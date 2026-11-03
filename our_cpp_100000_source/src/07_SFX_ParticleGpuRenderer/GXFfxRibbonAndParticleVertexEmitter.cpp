#include <cstdint>
#include <cstring>
#include <cmath>
#include <immintrin.h>

extern "C" {
void sub_28eaf20(uint32_t* pFirst, uint32_t* pLast, uint64_t count);
}

namespace {

// Helper: Convert 4 non-negative float lanes in [0, 2^31-1] to 4 uint32_t lanes via SIMD cvttps_epi32
static inline __m128 ffxCvttPsToU32ViaSi64(__m128 v) {
    return _mm_castsi128_ps(_mm_cvttps_epi32(v));
}

// Helper: Clamp (R, G, B, A) in [0, 1] and pack into 32-bit RGBA per lane via direct SIMD shifts
static inline __m128 ffxPackRgbaSimd(__m128 r, __m128 g, __m128 b, __m128 a) {
    const __m128 vZero = _mm_setzero_ps();
    const __m128 vOne  = _mm_set1_ps(1.0f);
    const __m128 v255  = _mm_set1_ps(255.0f);

    __m128i ri = _mm_cvttps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(r, vZero), vOne), v255));
    __m128i gi = _mm_slli_epi32(_mm_cvttps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(g, vZero), vOne), v255)), 8);
    __m128i bi = _mm_slli_epi32(_mm_cvttps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(b, vZero), vOne), v255)), 16);
    __m128i ai = _mm_slli_epi32(_mm_cvttps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(a, vZero), vOne), v255)), 24);

    return _mm_castsi128_ps(_mm_or_si128(_mm_or_si128(ri, gi), _mm_or_si128(bi, ai)));
}

// Helper: 2-iteration Newton-Raphson sqrt via vrsqrtps with zero-guard matching eboot.bin
static inline __m128 ffxSimdSqrtNR2(__m128 lenSq) {
    const __m128 vZero = _mm_setzero_ps();
    const __m128 vHalf = _mm_set1_ps(0.5f);
    const __m128 vThree = _mm_set1_ps(3.0f);

    __m128 y0 = _mm_rsqrt_ps(lenSq);
    __m128 zeroMask = _mm_or_ps(_mm_cmpeq_ps(lenSq, vZero), _mm_cmpeq_ps(y0, vZero));

    __m128 halfY0 = _mm_mul_ps(y0, vHalf);
    __m128 term0  = _mm_sub_ps(vThree, _mm_mul_ps(y0, _mm_mul_ps(lenSq, y0)));
    __m128 y1     = _mm_mul_ps(halfY0, term0);

    __m128 halfY1 = _mm_mul_ps(y1, vHalf);
    __m128 term1  = _mm_sub_ps(vThree, _mm_mul_ps(y1, _mm_mul_ps(lenSq, y1)));
    __m128 y2     = _mm_mul_ps(halfY1, term1);

    __m128 sq = _mm_mul_ps(lenSq, y2);
    return _mm_blendv_ps(sq, lenSq, zeroMask);
}

// Helper: Safe reciprocal of ffxSimdSqrtNR2 (1.0f / sqrt(lenSq), flushing +/-Inf to 0.0f)
static inline __m128 ffxSimdSafeRcpFromSqrt(__m128 sq) {
    const __m128 vOne = _mm_set1_ps(1.0f);
    const __m128 vAbsMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFFu));
    const __m128i vInfBits = _mm_set1_epi32(0x7F800000u);

    __m128 rcp = _mm_div_ps(vOne, sq);
    __m128 isInf = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(_mm_and_ps(rcp, vAbsMask)), vInfBits));
    return _mm_andnot_ps(isInf, rcp);
}

} // namespace

extern "C" {

// =====================================================================
// Function sub_28d21c0 (VMA: 0x28d21c0, Size: 1011 bytes)
// GXFfx Emit Point/Sprite Particle Vertices (20 bytes per vertex, 4 per loop)
// =====================================================================
void sub_28d21c0(
    uint8_t* pOutVerts,
    const float* const* ppStreams,
    uint32_t quadCount,
    const float* pPosX,
    const float* pPosY,
    const float* pPosZ,
    const __m128* pWorldMat3x4)
{
    __m128 row0 = pWorldMat3x4[0];
    __m128 row1 = pWorldMat3x4[1];
    __m128 row2 = pWorldMat3x4[2];

    __m128 m00 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0x00));
    __m128 m01 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0x55));
    __m128 m02 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0xAA));
    __m128 m03 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0xFF));

    __m128 m10 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x00));
    __m128 m11 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x55));
    __m128 m12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0xAA));
    __m128 m13 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0xFF));

    __m128 m20 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0x00));
    __m128 m21 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0x55));
    __m128 m22 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0xAA));
    __m128 m23 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0xFF));

    uint32_t byteOff = (quadCount << 4) - 0x10u;
    uint64_t vertCount = static_cast<uint64_t>(quadCount << 2);
    uint8_t* pCursor = pOutVerts + vertCount * 20u - 0x28;

    do {
        uint64_t off = byteOff;
        __m128 px = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + off));
        __m128 py = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + off));
        __m128 pz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + off));

        __m128 bVal = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[1]) + off));
        __m128 gVal = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[2]) + off));
        __m128 rVal = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[3]) + off));
        __m128 aVal = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[4]) + off));

        __m128 wx = _mm_add_ps(_mm_mul_ps(m02, pz), _mm_add_ps(_mm_add_ps(m03, _mm_mul_ps(m00, px)), _mm_mul_ps(m01, py)));
        __m128 wy = _mm_add_ps(_mm_mul_ps(m12, pz), _mm_add_ps(_mm_add_ps(m13, _mm_mul_ps(m10, px)), _mm_mul_ps(m11, py)));
        __m128 wz = _mm_add_ps(_mm_mul_ps(m22, pz), _mm_add_ps(_mm_add_ps(m23, _mm_mul_ps(m20, px)), _mm_mul_ps(m21, py)));

        __m128 rgba = ffxPackRgbaSimd(rVal, gVal, bVal, aVal);
        __m128 sVal = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[0]) + off));

        __m128 xyLo = _mm_movelh_ps(wx, wy);
        __m128 xyHi = _mm_movehl_ps(wy, wx);
        __m128 zHi  = _mm_movehl_ps(wz, wz);

        __m128 pos0 = _mm_shuffle_ps(xyLo, wz, 0x88);
        __m128 pos1 = _mm_shuffle_ps(xyLo, wz, 0xDD);
        __m128 pos2 = _mm_shuffle_ps(xyHi, zHi, 0x88);
        __m128 pos3 = _mm_shuffle_ps(xyHi, zHi, 0xDD);

        __m128 scLo = _mm_movelh_ps(sVal, rgba);
        __m128 scHi = _mm_movehl_ps(rgba, sVal);

        __m128 sc0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(scLo), 0x08));
        __m128 sc1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(scLo), 0x0D));
        __m128 sc2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(scHi), 0x08));
        __m128 sc3 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(scHi), 0x0D));

        std::memcpy(pCursor - 0x28, &pos0, 12);
        std::memcpy(pCursor - 0x14, &pos1, 12);
        std::memcpy(pCursor + 0x00, &pos2, 12);
        std::memcpy(pCursor + 0x14, &pos3, 12);

        std::memcpy(pCursor - 0x1C, &sc0, 8);
        std::memcpy(pCursor - 0x08, &sc1, 8);
        std::memcpy(pCursor + 0x0C, &sc2, 8);
        std::memcpy(pCursor + 0x20, &sc3, 8);

        byteOff -= 0x10u;
        pCursor -= 0x50;
    } while (byteOff != 0xFFFFFFF0u);
}

// =====================================================================
// Function sub_28d17d0 (VMA: 0x28d17d0, Size: 2540 bytes)
// GXFfx Emit Velocity-Aligned 3-Vertex Triangle Billboards (192B per 4 particles)
// =====================================================================
void sub_28d17d0(
    uint8_t* pOutVerts,
    const float* const* ppStreams,
    uint32_t quadCount,
    const float* pPosX,
    const float* pPosY,
    const float* pPosZ,
    float camPosX,
    float camPosY,
    float camPosZ,
    const float* pVelX,
    const float* pVelY,
    const float* pVelZ,
    const __m128* pWorldMat3x4)
{
    __m128 row0 = pWorldMat3x4[0];
    __m128 row1 = pWorldMat3x4[1];
    __m128 row2 = pWorldMat3x4[2];

    __m128 m00 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0x00));
    __m128 m01 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0x55));
    __m128 m02 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0xAA));
    __m128 m03 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0xFF));

    __m128 m10 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x00));
    __m128 m11 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x55));
    __m128 m12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0xAA));
    __m128 m13 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0xFF));

    __m128 m20 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0x00));
    __m128 m21 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0x55));
    __m128 m22 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0xAA));
    __m128 m23 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0xFF));

    __m128 vCamX = _mm_set1_ps(camPosX);
    __m128 vCamY = _mm_set1_ps(camPosY);
    __m128 vCamZ = _mm_set1_ps(camPosZ);

    uint32_t byteOff = (quadCount << 4) - 0x10u;
    uint64_t totalVertBytes = static_cast<uint64_t>(quadCount << 2) * 48u;
    uint8_t* pCursor = pOutVerts + totalVertBytes - 0x10;

    do {
        uint64_t off = byteOff;
        __m128 px = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + off));
        __m128 py = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + off));
        __m128 pz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + off));

        __m128 vx = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelX) + off));
        __m128 vy = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelY) + off));
        __m128 vz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelZ) + off));

        __m128 b0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[2]) + off));
        __m128 g0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[3]) + off));
        __m128 r0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[4]) + off));
        __m128 a0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[5]) + off));

        __m128 wx = _mm_add_ps(_mm_mul_ps(m02, pz), _mm_add_ps(_mm_add_ps(m03, _mm_mul_ps(m00, px)), _mm_mul_ps(m01, py)));
        __m128 wy = _mm_add_ps(_mm_mul_ps(m12, pz), _mm_add_ps(_mm_add_ps(m13, _mm_mul_ps(m10, px)), _mm_mul_ps(m11, py)));
        __m128 wz = _mm_add_ps(_mm_mul_ps(m22, pz), _mm_add_ps(_mm_add_ps(m23, _mm_mul_ps(m20, px)), _mm_mul_ps(m21, py)));

        // Normalize velocity (vx, vy, vz)
        __m128 vLenSq = _mm_add_ps(_mm_mul_ps(vz, vz), _mm_add_ps(_mm_mul_ps(vx, vx), _mm_mul_ps(vy, vy)));
        __m128 vInvLen = ffxSimdSafeRcpFromSqrt(ffxSimdSqrtNR2(vLenSq));
        __m128 dirX = _mm_mul_ps(vx, vInvLen);
        __m128 dirY = _mm_mul_ps(vy, vInvLen);
        __m128 dirZ = _mm_mul_ps(vz, vInvLen);

        // Eye vector = CamPos - WorldPos
        __m128 eyeX = _mm_sub_ps(vCamX, wx);
        __m128 eyeY = _mm_sub_ps(vCamY, wy);
        __m128 eyeZ = _mm_sub_ps(vCamZ, wz);
        __m128 eyeLenSq = _mm_add_ps(_mm_mul_ps(eyeZ, eyeZ), _mm_add_ps(_mm_mul_ps(eyeX, eyeX), _mm_mul_ps(eyeY, eyeY)));
        __m128 eyeInvLen = ffxSimdSafeRcpFromSqrt(ffxSimdSqrtNR2(eyeLenSq));
        eyeX = _mm_mul_ps(eyeX, eyeInvLen);
        eyeY = _mm_mul_ps(eyeY, eyeInvLen);
        eyeZ = _mm_mul_ps(eyeZ, eyeInvLen);

        // Cross product: side = dir x eye
        __m128 sideX = _mm_sub_ps(_mm_mul_ps(dirY, eyeZ), _mm_mul_ps(dirZ, eyeY));
        __m128 sideY = _mm_sub_ps(_mm_mul_ps(dirZ, eyeX), _mm_mul_ps(dirX, eyeZ));
        __m128 sideZ = _mm_sub_ps(_mm_mul_ps(dirX, eyeY), _mm_mul_ps(dirY, eyeX));
        __m128 sideLenSq = _mm_add_ps(_mm_mul_ps(sideZ, sideZ), _mm_add_ps(_mm_mul_ps(sideX, sideX), _mm_mul_ps(sideY, sideY)));
        __m128 sideSqrt = ffxSimdSqrtNR2(sideLenSq);

        __m128 color0 = ffxPackRgbaSimd(r0, g0, b0, a0);

        __m128 b1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[6]) + off));
        __m128 g1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[7]) + off));
        __m128 r1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[8]) + off));
        __m128 a1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[9]) + off));
        __m128 color1 = ffxPackRgbaSimd(r1, g1, b1, a1);

        __m128 lenScale = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[1]) + off));
        __m128 v1x = _mm_mul_ps(lenScale, dirX);
        __m128 v1y = _mm_mul_ps(lenScale, dirY);
        __m128 v1z = _mm_mul_ps(lenScale, dirZ);

        __m128 sideInvLen = ffxSimdSafeRcpFromSqrt(sideSqrt);
        __m128 widScale = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[0]) + off));
        __m128 v2x = _mm_mul_ps(widScale, _mm_mul_ps(sideX, sideInvLen));
        __m128 v2y = _mm_mul_ps(widScale, _mm_mul_ps(sideY, sideInvLen));
        __m128 v2z = _mm_mul_ps(widScale, _mm_mul_ps(sideZ, sideInvLen));

        // Transpose (wx, wy, wz, 0.0f) for 4 particles
        __m128 wxyLo = _mm_movelh_ps(wx, wy);
        __m128 wxyHi = _mm_movehl_ps(wy, wx);
        __m128 wzLo  = _mm_castsi128_ps(_mm_move_epi64(_mm_castps_si128(wz)));
        __m128 wzHi  = _mm_movehl_ps(_mm_setzero_ps(), wz);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0xB0), _mm_shuffle_ps(wxyLo, wzLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x80), _mm_shuffle_ps(wxyLo, wzLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x50), _mm_shuffle_ps(wxyHi, wzHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x20), _mm_shuffle_ps(wxyHi, wzHi, 0xDD));

        // Transpose (color0, v1x, v1y, v1z) for 4 particles
        __m128 c0v1xLo = _mm_movelh_ps(color0, v1x);
        __m128 c0v1xHi = _mm_movehl_ps(v1x, color0);
        __m128 v1yzLo  = _mm_movelh_ps(v1y, v1z);
        __m128 v1yzHi  = _mm_movehl_ps(v1z, v1y);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0xA0), _mm_shuffle_ps(c0v1xLo, v1yzLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x70), _mm_shuffle_ps(c0v1xLo, v1yzLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x40), _mm_shuffle_ps(c0v1xHi, v1yzHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x10), _mm_shuffle_ps(c0v1xHi, v1yzHi, 0xDD));

        // Transpose (color1, v2x, v2y, v2z) for 4 particles
        __m128 c1v2xLo = _mm_movelh_ps(color1, v2x);
        __m128 c1v2xHi = _mm_movehl_ps(v2x, color1);
        __m128 v2yzLo  = _mm_movelh_ps(v2y, v2z);
        __m128 v2yzHi  = _mm_movehl_ps(v2z, v2y);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x90), _mm_shuffle_ps(c1v2xLo, v2yzLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x60), _mm_shuffle_ps(c1v2xLo, v2yzLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x30), _mm_shuffle_ps(c1v2xHi, v2yzHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor + 0x00), _mm_shuffle_ps(c1v2xHi, v2yzHi, 0xDD));

        byteOff -= 0x10u;
        pCursor -= 0xC0;
    } while (byteOff != 0xFFFFFFF0u);
}

// =====================================================================
// Function sub_28d0db0 (VMA: 0x28d0db0, Size: 2581 bytes)
// GXFfx Emit Velocity-Aligned 4-Corner Quad Billboards (256B per 4 particles)
// =====================================================================
void sub_28d0db0(
    uint8_t* pOutVerts,
    const float* const* ppStreams,
    uint32_t quadCount,
    const float* pPosX,
    const float* pPosY,
    const float* pPosZ,
    float camPosX,
    float camPosY,
    float camPosZ,
    const float* pVelX,
    const float* pVelY,
    const float* pVelZ,
    const __m128* pWorldMat3x4)
{
    __m128 row0 = pWorldMat3x4[0];
    __m128 row1 = pWorldMat3x4[1];
    __m128 row2 = pWorldMat3x4[2];

    __m128 m00 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0x00));
    __m128 m01 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0x55));
    __m128 m02 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0xAA));
    __m128 m03 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0xFF));

    __m128 m10 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x00));
    __m128 m11 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x55));
    __m128 m12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0xAA));
    __m128 m13 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0xFF));

    __m128 m20 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0x00));
    __m128 m21 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0x55));
    __m128 m22 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0xAA));
    __m128 m23 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0xFF));

    __m128 vCamX = _mm_set1_ps(camPosX);
    __m128 vCamY = _mm_set1_ps(camPosY);
    __m128 vCamZ = _mm_set1_ps(camPosZ);

    uint32_t scaledQuad = quadCount << 4;
    uint32_t byteOff = scaledQuad - 0x10u;
    uint8_t* pCursor = pOutVerts + (static_cast<uint64_t>(scaledQuad) << 4) - 0x10;

    do {
        uint64_t off = byteOff;
        __m128 px = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + off));
        __m128 py = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + off));
        __m128 pz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + off));

        __m128 vx = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelX) + off));
        __m128 vy = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelY) + off));
        __m128 vz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelZ) + off));

        __m128 b0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[2]) + off));
        __m128 g0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[3]) + off));
        __m128 r0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[4]) + off));
        __m128 a0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[5]) + off));

        __m128 b1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[6]) + off));
        __m128 g1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[7]) + off));
        __m128 r1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[8]) + off));
        __m128 a1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[9]) + off));

        __m128 wx = _mm_add_ps(_mm_mul_ps(m02, pz), _mm_add_ps(_mm_add_ps(m03, _mm_mul_ps(m00, px)), _mm_mul_ps(m01, py)));
        __m128 wy = _mm_add_ps(_mm_mul_ps(m12, pz), _mm_add_ps(_mm_add_ps(m13, _mm_mul_ps(m10, px)), _mm_mul_ps(m11, py)));
        __m128 wz = _mm_add_ps(_mm_mul_ps(m22, pz), _mm_add_ps(_mm_add_ps(m23, _mm_mul_ps(m20, px)), _mm_mul_ps(m21, py)));

        __m128 color0 = ffxPackRgbaSimd(r0, g0, b0, a0);
        __m128 color1 = ffxPackRgbaSimd(r1, g1, b1, a1);

        // Normalize velocity
        __m128 vLenSq = _mm_add_ps(_mm_mul_ps(vz, vz), _mm_add_ps(_mm_mul_ps(vx, vx), _mm_mul_ps(vy, vy)));
        __m128 vInvLen = ffxSimdSafeRcpFromSqrt(ffxSimdSqrtNR2(vLenSq));
        __m128 dirX = _mm_mul_ps(vx, vInvLen);
        __m128 dirY = _mm_mul_ps(vy, vInvLen);
        __m128 dirZ = _mm_mul_ps(vz, vInvLen);

        // Eye vector = CamPos - WorldPos
        __m128 eyeX = _mm_sub_ps(vCamX, wx);
        __m128 eyeY = _mm_sub_ps(vCamY, wy);
        __m128 eyeZ = _mm_sub_ps(vCamZ, wz);
        __m128 eyeLenSq = _mm_add_ps(_mm_mul_ps(eyeZ, eyeZ), _mm_add_ps(_mm_mul_ps(eyeX, eyeX), _mm_mul_ps(eyeY, eyeY)));
        __m128 lenScale = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[1]) + off));
        __m128 eyeSqrt  = ffxSimdSqrtNR2(eyeLenSq);

        __m128 stepX = _mm_mul_ps(lenScale, dirX);
        __m128 stepY = _mm_mul_ps(lenScale, dirY);
        __m128 stepZ = _mm_mul_ps(lenScale, dirZ);

        __m128 eyeInvLen = ffxSimdSafeRcpFromSqrt(eyeSqrt);
        eyeX = _mm_mul_ps(eyeX, eyeInvLen);
        eyeY = _mm_mul_ps(eyeY, eyeInvLen);
        eyeZ = _mm_mul_ps(eyeZ, eyeInvLen);

        // Cross product: side = dir x eye
        __m128 sideX = _mm_sub_ps(_mm_mul_ps(dirY, eyeZ), _mm_mul_ps(dirZ, eyeY));
        __m128 sideY = _mm_sub_ps(_mm_mul_ps(dirZ, eyeX), _mm_mul_ps(dirX, eyeZ));
        __m128 sideZ = _mm_sub_ps(_mm_mul_ps(dirX, eyeY), _mm_mul_ps(dirY, eyeX));
        __m128 sideLenSq = _mm_add_ps(_mm_mul_ps(sideZ, sideZ), _mm_add_ps(_mm_mul_ps(sideX, sideX), _mm_mul_ps(sideY, sideY)));
        __m128 widScale  = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[0]) + off));
        __m128 sideInvLen = ffxSimdSafeRcpFromSqrt(ffxSimdSqrtNR2(sideLenSq));

        __m128 offX = _mm_mul_ps(widScale, _mm_mul_ps(sideX, sideInvLen));
        __m128 offY = _mm_mul_ps(widScale, _mm_mul_ps(sideY, sideInvLen));
        __m128 offZ = _mm_mul_ps(widScale, _mm_mul_ps(sideZ, sideInvLen));

        // Vertex 0: (wx, wy, wz, color0)
        __m128 v0xyLo = _mm_movelh_ps(wx, wy);
        __m128 v0xyHi = _mm_movehl_ps(wy, wx);
        __m128 v0zcLo = _mm_movelh_ps(wz, color0);
        __m128 v0zcHi = _mm_movehl_ps(color0, wz);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0xF0), _mm_shuffle_ps(v0xyLo, v0zcLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0xB0), _mm_shuffle_ps(v0xyLo, v0zcLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x70), _mm_shuffle_ps(v0xyHi, v0zcHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x30), _mm_shuffle_ps(v0xyHi, v0zcHi, 0xDD));

        // Tail base = w - step
        __m128 tailX = _mm_sub_ps(wx, stepX);
        __m128 tailY = _mm_sub_ps(wy, stepY);
        __m128 tailZ = _mm_sub_ps(wz, stepZ);

        // Vertex 1: (wx + offX, wy + offY, wz + offZ, color0)
        __m128 v1x = _mm_add_ps(wx, offX);
        __m128 v1y = _mm_add_ps(wy, offY);
        __m128 v1z = _mm_add_ps(wz, offZ);
        __m128 v1xyLo = _mm_movelh_ps(v1x, v1y);
        __m128 v1xyHi = _mm_movehl_ps(v1y, v1x);
        __m128 v1zcLo = _mm_movelh_ps(v1z, color0);
        __m128 v1zcHi = _mm_movehl_ps(color0, v1z);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0xE0), _mm_shuffle_ps(v1xyLo, v1zcLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0xA0), _mm_shuffle_ps(v1xyLo, v1zcLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x60), _mm_shuffle_ps(v1xyHi, v1zcHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x20), _mm_shuffle_ps(v1xyHi, v1zcHi, 0xDD));

        // Vertex 2: (tailX + offX, tailY + offY, tailZ + offZ, color1)
        __m128 v2x = _mm_add_ps(tailX, offX);
        __m128 v2y = _mm_add_ps(tailY, offY);
        __m128 v2z = _mm_add_ps(tailZ, offZ);
        __m128 v2xyLo = _mm_movelh_ps(v2x, v2y);
        __m128 v2xyHi = _mm_movehl_ps(v2y, v2x);
        __m128 v2zcLo = _mm_movelh_ps(v2z, color1);
        __m128 v2zcHi = _mm_movehl_ps(color1, v2z);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0xD0), _mm_shuffle_ps(v2xyLo, v2zcLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x90), _mm_shuffle_ps(v2xyLo, v2zcLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x50), _mm_shuffle_ps(v2xyHi, v2zcHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x10), _mm_shuffle_ps(v2xyHi, v2zcHi, 0xDD));

        // Vertex 3: (tailX, tailY, tailZ, color1)
        __m128 v3xyLo = _mm_movelh_ps(tailX, tailY);
        __m128 v3xyHi = _mm_movehl_ps(tailY, tailX);
        __m128 v3zcLo = _mm_movelh_ps(tailZ, color1);
        __m128 v3zcHi = _mm_movehl_ps(color1, tailZ);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0xC0), _mm_shuffle_ps(v3xyLo, v3zcLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x80), _mm_shuffle_ps(v3xyLo, v3zcLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x40), _mm_shuffle_ps(v3xyHi, v3zcHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor + 0x00), _mm_shuffle_ps(v3xyHi, v3zcHi, 0xDD));

        byteOff -= 0x10u;
        pCursor -= 0x100;
    } while (byteOff != 0xFFFFFFF0u);
}

// =====================================================================
// Function sub_28d0650 (VMA: 0x28d0650, Size: 1878 bytes)
// GXFfx Emit Velocity-Aligned 2-Endpoint Line Segments (128B per 4 particles)
// =====================================================================
void sub_28d0650(
    uint8_t* pOutVerts,
    const float* const* ppStreams,
    uint32_t quadCount,
    const float* pPosX,
    const float* pPosY,
    const float* pPosZ,
    const float* pVelX,
    const float* pVelY,
    const float* pVelZ,
    const __m128* pWorldMat3x4)
{
    __m128 row0 = pWorldMat3x4[0];
    __m128 row1 = pWorldMat3x4[1];
    __m128 row2 = pWorldMat3x4[2];

    __m128 m00 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0x00));
    __m128 m01 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0x55));
    __m128 m02 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0xAA));
    __m128 m03 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row0), 0xFF));

    __m128 m10 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x00));
    __m128 m11 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0x55));
    __m128 m12 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0xAA));
    __m128 m13 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row1), 0xFF));

    __m128 m20 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0x00));
    __m128 m21 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0x55));
    __m128 m22 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0xAA));
    __m128 m23 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row2), 0xFF));

    uint32_t byteOff = (quadCount << 4) - 0x10u;
    uint8_t* pCursor = pOutVerts + (static_cast<uint64_t>(quadCount << 3) << 4) - 0x10;

    do {
        uint64_t off = byteOff;
        __m128 px = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + off));
        __m128 py = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + off));
        __m128 pz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + off));

        __m128 vx = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelX) + off));
        __m128 vy = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelY) + off));
        __m128 vz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVelZ) + off));

        __m128 b0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[1]) + off));
        __m128 g0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[2]) + off));
        __m128 r0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[3]) + off));
        __m128 a0 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[4]) + off));

        __m128 wx = _mm_add_ps(_mm_mul_ps(m02, pz), _mm_add_ps(_mm_add_ps(m03, _mm_mul_ps(m00, px)), _mm_mul_ps(m01, py)));
        __m128 wy = _mm_add_ps(_mm_mul_ps(m12, pz), _mm_add_ps(_mm_add_ps(m13, _mm_mul_ps(m10, px)), _mm_mul_ps(m11, py)));
        __m128 wz = _mm_add_ps(_mm_mul_ps(m22, pz), _mm_add_ps(_mm_add_ps(m23, _mm_mul_ps(m20, px)), _mm_mul_ps(m21, py)));

        __m128 vLenSq = _mm_add_ps(_mm_mul_ps(vz, vz), _mm_add_ps(_mm_mul_ps(vx, vx), _mm_mul_ps(vy, vy)));
        __m128 vSqrt  = ffxSimdSqrtNR2(vLenSq);

        __m128 color0 = ffxPackRgbaSimd(r0, g0, b0, a0);

        __m128 b1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[5]) + off));
        __m128 g1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[6]) + off));
        __m128 r1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[7]) + off));
        __m128 a1 = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[8]) + off));
        __m128 color1 = ffxPackRgbaSimd(r1, g1, b1, a1);

        __m128 vInvLen  = ffxSimdSafeRcpFromSqrt(vSqrt);
        __m128 lenScale = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(ppStreams[0]) + off));

        __m128 tailX = _mm_sub_ps(wx, _mm_mul_ps(lenScale, _mm_mul_ps(vx, vInvLen)));
        __m128 tailY = _mm_sub_ps(wy, _mm_mul_ps(lenScale, _mm_mul_ps(vy, vInvLen)));
        __m128 tailZ = _mm_sub_ps(wz, _mm_mul_ps(lenScale, _mm_mul_ps(vz, vInvLen)));

        // Head vertex: (wx, wy, wz, color0)
        __m128 hxyLo = _mm_movelh_ps(wx, wy);
        __m128 hxyHi = _mm_movehl_ps(wy, wx);
        __m128 hzcLo = _mm_movelh_ps(wz, color0);
        __m128 hzcHi = _mm_movehl_ps(color0, wz);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x70), _mm_shuffle_ps(hxyLo, hzcLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x50), _mm_shuffle_ps(hxyLo, hzcLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x30), _mm_shuffle_ps(hxyHi, hzcHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x10), _mm_shuffle_ps(hxyHi, hzcHi, 0xDD));

        // Tail vertex: (tailX, tailY, tailZ, color1)
        __m128 txyLo = _mm_movelh_ps(tailX, tailY);
        __m128 txyHi = _mm_movehl_ps(tailY, tailX);
        __m128 tzcLo = _mm_movelh_ps(tailZ, color1);
        __m128 tzcHi = _mm_movehl_ps(color1, tailZ);

        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x60), _mm_shuffle_ps(txyLo, tzcLo, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x40), _mm_shuffle_ps(txyLo, tzcLo, 0xDD));
        _mm_store_ps(reinterpret_cast<float*>(pCursor - 0x20), _mm_shuffle_ps(txyHi, tzcHi, 0x88));
        _mm_store_ps(reinterpret_cast<float*>(pCursor + 0x00), _mm_shuffle_ps(txyHi, tzcHi, 0xDD));

        byteOff -= 0x10u;
        pCursor -= 0x80;
    } while (byteOff != 0xFFFFFFF0u);
}

// =====================================================================
// Function sub_28d0480 (VMA: 0x28d0480, Size: 462 bytes)
// GXFfx Compute SoA Particle Positions AABB (Min/Max with w = 1.0f)
// =====================================================================
void sub_28d0480(
    __m128* pOutMin,
    __m128* pOutMax,
    uint32_t count,
    const float* pPosX,
    const float* pPosY,
    const float* pPosZ)
{
    uint32_t quadCount = count >> 2;
    uint32_t alignedCount = quadCount << 2;

    const float fltMax = __builtin_bit_cast(float, 0x7F7FFFFFu);
    const float fltMin = __builtin_bit_cast(float, 0xFF7FFFFFu);

    __m128 minX = _mm_set_ss(fltMax);
    __m128 minY = minX;
    __m128 minZ = minX;
    __m128 maxX = _mm_set_ss(fltMin);
    __m128 maxY = maxX;
    __m128 maxZ = maxX;

    if (alignedCount < count) {
        uint32_t base = count & ~3u;
        const float* px = pPosX + base;
        const float* py = pPosY + base;
        const float* pz = pPosZ + base;
        uint32_t rem = count - base;
        do {
            __m128 vx = _mm_load_ss(px++);
            __m128 vy = _mm_load_ss(py++);
            __m128 vz = _mm_load_ss(pz++);
            minX = _mm_min_ss(minX, vx);
            minY = _mm_min_ss(minY, vy);
            minZ = _mm_min_ss(minZ, vz);
            maxX = _mm_max_ss(maxX, vx);
            maxY = _mm_max_ss(maxY, vy);
            maxZ = _mm_max_ss(maxZ, vz);
            --rem;
        } while (rem != 0);
    }

    if (quadCount != 0) {
        __m128 vMinX = _mm_insert_ps(_mm_insert_ps(_mm_insert_ps(minX, minX, 0x10), minX, 0x20), minX, 0x30);
        __m128 vMinY = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(minY), 0x00));
        __m128 vMinZ = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(minZ), 0x00));
        __m128 vMaxX = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(maxX), 0x00));
        __m128 vMaxY = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(maxY), 0x00));
        __m128 vMaxZ = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(maxZ), 0x00));

        uint32_t byteOff = ((count & 0x3FFFFFFCu) << 2) - 0x10u;
        do {
            uint64_t off = byteOff;
            __m128 vx = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + off));
            __m128 vy = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + off));
            __m128 vz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + off));
            vMinX = _mm_min_ps(vMinX, vx);
            vMinY = _mm_min_ps(vMinY, vy);
            vMinZ = _mm_min_ps(vMinZ, vz);
            vMaxX = _mm_max_ps(vMaxX, vx);
            vMaxY = _mm_max_ps(vMaxY, vy);
            vMaxZ = _mm_max_ps(vMaxZ, vz);
            byteOff -= 0x10u;
        } while (byteOff != 0xFFFFFFF0u);

        const __m128 vOne = _mm_set1_ps(1.0f);
        __m128 minXYLo = _mm_movelh_ps(vMinX, vMinY);
        __m128 minXYHi = _mm_movehl_ps(vMinY, vMinX);
        __m128 minZWLo = _mm_movelh_ps(vMinZ, vOne);
        __m128 minZWHi = _mm_movehl_ps(vOne, vMinZ);

        __m128 min0 = _mm_shuffle_ps(minXYLo, minZWLo, 0x88);
        __m128 min1 = _mm_shuffle_ps(minXYLo, minZWLo, 0xDD);
        __m128 min2 = _mm_shuffle_ps(minXYHi, minZWHi, 0x88);
        __m128 min3 = _mm_shuffle_ps(minXYHi, minZWHi, 0xDD);
        *pOutMin = _mm_min_ps(_mm_min_ps(min0, min1), _mm_min_ps(min2, min3));

        __m128 maxXYLo = _mm_movelh_ps(vMaxX, vMaxY);
        __m128 maxXYHi = _mm_movehl_ps(vMaxY, vMaxX);
        __m128 maxZWLo = _mm_movelh_ps(vMaxZ, vOne);
        __m128 maxZWHi = _mm_movehl_ps(vOne, vMaxZ);

        __m128 max0 = _mm_shuffle_ps(maxXYLo, maxZWLo, 0x88);
        __m128 max1 = _mm_shuffle_ps(maxXYLo, maxZWLo, 0xDD);
        __m128 max2 = _mm_shuffle_ps(maxXYHi, maxZWHi, 0x88);
        __m128 max3 = _mm_shuffle_ps(maxXYHi, maxZWHi, 0xDD);
        *pOutMax = _mm_max_ps(_mm_max_ps(max0, max1), _mm_max_ps(max2, max3));
    } else {
        const __m128 sOne = _mm_set_ss(1.0f);
        __m128 outMin = _mm_insert_ps(_mm_insert_ps(_mm_insert_ps(minX, minY, 0x10), minZ, 0x20), sOne, 0x30);
        *pOutMin = outMin;
        __m128 outMax = _mm_insert_ps(_mm_insert_ps(_mm_insert_ps(maxX, maxY, 0x10), maxZ, 0x20), sOne, 0x30);
        *pOutMax = outMax;
    }
}

// =====================================================================
// Function sub_28d02c0 (VMA: 0x28d02c0, Size: 436 bytes)
// GXFfx Build 16-bit View-Depth Sort Keys & Introsort Particle Indices
// =====================================================================
void sub_28d02c0(
    uint32_t* pSortBuf,
    uint32_t count,
    const float* pPosZ,
    const float* pPosX,
    const float* pPosY,
    const uint8_t* pViewMat,
    const __m128* pBoundsZ)
{
    uint32_t paddedCount = count + 3u;
    uint32_t quadCount = paddedCount >> 2;

    __m128 m20 = _mm_set1_ps(*reinterpret_cast<const float*>(pViewMat + 0x20));
    __m128 m21 = _mm_set1_ps(*reinterpret_cast<const float*>(pViewMat + 0x24));
    __m128 m22 = _mm_set1_ps(*reinterpret_cast<const float*>(pViewMat + 0x28));
    __m128 m23 = _mm_set1_ps(*reinterpret_cast<const float*>(pViewMat + 0x2C));

    __m128 minB = pBoundsZ[0];
    __m128 maxB = pBoundsZ[1];
    __m128 minZ = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(minB), 0xAA));
    __m128 span = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_mm_sub_ps(maxB, minB)), 0xAA));

    const __m128 vOne = _mm_set1_ps(1.0f);
    const __m128 v65535 = _mm_set1_ps(65535.0f);
    const __m128 vEps = _mm_set1_ps(1.0e-7f);
    span = _mm_max_ps(span, vEps);
    __m128 scale = _mm_mul_ps(_mm_div_ps(vOne, span), v65535);
    __m128 bias  = _mm_sub_ps(_mm_setzero_ps(), _mm_mul_ps(minZ, scale));

    uint32_t baseIdx0 = quadCount << 18;
    uint32_t baseIdx1 = baseIdx0 + 0x10000u;
    uint32_t baseIdx2 = baseIdx0 | 0x20000u;
    uint32_t baseIdx3 = (paddedCount | 3u) << 16;

    __m128 idxVec = _mm_insert_ps(
        _mm_insert_ps(
            _mm_insert_ps(
                _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int32_t>(baseIdx0))),
                _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int32_t>(baseIdx1))),
                0x10),
            _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int32_t>(baseIdx2))),
            0x20),
        _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int32_t>(baseIdx3))),
        0x30);

    uint32_t byteOff = (quadCount << 4) - 0x10u;
    const __m128 mLow16  = _mm_castsi128_ps(_mm_set1_epi32(0x0000FFFFu));
    const __m128i mHigh16 = _mm_set1_epi32(static_cast<int32_t>(0xFFFF0000u));

    const __m128i vIdxStep = _mm_set1_epi32(static_cast<int32_t>(0xFFFC0000u));
    do {
        __m128i iv = _mm_add_epi32(_mm_castps_si128(idxVec), vIdxStep);
        idxVec = _mm_castsi128_ps(iv);

        uint64_t off = byteOff;
        __m128 vz = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + off));
        __m128 vx = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + off));
        __m128 vy = _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + off));

        __m128 viewZ = _mm_add_ps(_mm_mul_ps(m22, vy), _mm_add_ps(_mm_mul_ps(m21, vx), _mm_add_ps(m23, _mm_mul_ps(m20, vz))));
        __m128 keyF  = _mm_min_ps(_mm_max_ps(_mm_add_ps(bias, _mm_mul_ps(scale, viewZ)), _mm_setzero_ps()), v65535);
        __m128 keyI  = ffxCvttPsToU32ViaSi64(keyF);

        __m128 packed = _mm_or_ps(_mm_and_ps(keyI, mLow16), _mm_castsi128_ps(_mm_and_si128(iv, mHigh16)));
        _mm_store_ps(reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(pSortBuf) + off), packed);

        byteOff -= 0x10u;
    } while (byteOff != 0xFFFFFFF0u);

    sub_28eaf20(pSortBuf, pSortBuf + count, count);
}

// =====================================================================
// Function sub_28d0160 (VMA: 0x28d0160, Size: 345 bytes)
// GXFfx Apply Camera Near/Far Distance Fade to Vertex Alpha Bytes
// =====================================================================
void sub_28d0160(
    uint8_t* pColorBytes,
    const uint8_t* pVertPos,
    uint32_t count,
    uint32_t stride,
    const float* pFadeParams,
    const uint8_t* pViewMat)
{
    if (count == 0) {
        return;
    }

    __m128 camVec = _mm_load_ps(reinterpret_cast<const float*>(pViewMat + 0x30));
    float camX = _mm_cvtss_f32(camVec);
    float camY = _mm_cvtss_f32(_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(camVec), 1)));
    float camZ = _mm_cvtss_f32(_mm_movehl_ps(camVec, camVec));

    float nearStart = pFadeParams[0];
    float nearRange = pFadeParams[1];
    float nearEnd   = nearStart + nearRange;
    float invNearRange = 1.0f / nearRange;

    float farStart  = pFadeParams[2];
    float farRange  = pFadeParams[3];
    float invFarRange = 1.0f / farRange;
    float farEnd    = farStart + farRange;

    uint64_t step = static_cast<uint64_t>(stride & ~3u);
    bool hasNearStart = (nearStart > 0.0f);
    bool hasNearRange = (nearRange > 0.0f);
    bool noNearRange  = (nearRange <= 0.0f);
    bool checkNear    = hasNearStart || hasNearRange;

    const float kOneZeroTable[2] = { 1.0f, 0.0f };
    const float farStartSq  = (farStart >= 0.0f) ? (farStart * farStart) : -1.0f;
    const float nearStartSq = (nearStart > 0.0f) ? (nearStart * nearStart) : 0.0f;
    const float nearEndSq   = (nearEnd > 0.0f) ? (nearEnd * nearEnd) : 0.0f;
    uint64_t byteOff = 0;

    do {
        const float* pos = reinterpret_cast<const float*>(pVertPos + byteOff);
        float dx = pos[0] - camX;
        float dy = pos[1] - camY;
        float dz = pos[2] - camZ;
        float distSq = (dx * dx + dy * dy) + dz * dz;

        if (distSq <= farStartSq && (!checkNear || (!noNearRange && distSq >= nearEndSq && distSq >= nearStartSq))) {
            byteOff += step;
            --count;
            continue;
        }

        float dist = std::sqrt(distSq);

        float factor;
        if (dist > farStart) {
            factor = 0.0f;
            if (dist <= farEnd) {
                factor = 1.0f - invFarRange * (dist - farStart);
            }
        } else {
            factor = 1.0f;
            if (checkNear) {
                bool belowNearStart = (nearStart > dist);
                uint32_t tblIdx = belowNearStart ? 1u : 0u;
                if (belowNearStart || noNearRange) {
                    factor = kOneZeroTable[tblIdx];
                } else {
                    factor = 1.0f;
                    if (nearEnd > dist) {
                        factor = invNearRange * (dist - nearStart);
                    }
                }
            }
        }

        uint32_t origAlpha = pColorBytes[byteOff + 3];
        float scaledAlpha = factor * static_cast<float>(origAlpha);
        uint32_t newAlpha = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss(scaledAlpha)));
        uint32_t shifted  = newAlpha << 24;
        if (newAlpha > 0xFFu) {
            shifted = 0xFF000000u;
        }
        pColorBytes[byteOff + 3] = static_cast<uint8_t>(shifted >> 24);

        byteOff += step;
        --count;
    } while (count != 0);
}

// =====================================================================
// Function sub_28cfba0 (VMA: 0x28cfba0, Size: 1470 bytes)
// GXFfx Emit Custom-Up-Vector Ribbon Trail Control Points (64B per point)
// =====================================================================
void sub_28cfba0(
    uint8_t* pOutVerts,
    uint32_t count,
    const float* pTime,
    const uint32_t* pPosX,
    const uint32_t* pPosY,
    const uint32_t* pPosZ,
    float curTime,
    float lifeTime,
    float param0,
    float param1,
    float vOffsetBase,
    float vOffsetDelta,
    float colorB,
    float colorG,
    const uint32_t* pUCoord,
    const float* pVCoordOpt,
    const float* pUpZ,
    const float* pUpX,
    const float* pUpY,
    uint32_t upStride,
    float colorR,
    float colorA,
    uint32_t startIdxOffset,
    uint32_t totalRibbonPoints,
    float fadeInCount,
    float fadeOutCount)
{
    float invLife = (0.0f < lifeTime) ? (1.0f / lifeTime) : -1.0f;
    uint32_t totalMinus1Int = totalRibbonPoints - 1u;
    float invFadeIn  = (fadeInCount > 0.0f) ? (1.0f / fadeInCount) : 1.0f;
    float invFadeOut = (fadeOutCount > 0.0f) ? (1.0f / fadeOutCount) : 1.0f;

    uint32_t param0Bits = __builtin_bit_cast(uint32_t, param0);
    uint32_t param1Bits = __builtin_bit_cast(uint32_t, param1);
    float vOffsetSum = vOffsetBase + vOffsetDelta;
    float totalMinus1F = static_cast<float>(static_cast<int64_t>(totalMinus1Int));

    static const float kZeroVCoord[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float* pVCoord = (pVCoordOpt != nullptr) ? pVCoordOpt : kZeroVCoord;
    uint32_t vCoordStepBytes = (pVCoordOpt != nullptr) ? 4u : 0u;

    uint32_t idxSumInt = startIdxOffset + count;
    float curPointIdxF = static_cast<float>(static_cast<int64_t>(idxSumInt));

    uint32_t bBits = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((colorB * 255.0f) * 65536.0f))) & 0x00FF0000u;
    if (colorB > 1.0f) {
        bBits = 0x00FF0000u;
    }
    uint32_t gBits = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((colorG * 255.0f) * 256.0f))) & 0x0000FF00u;
    if (colorG > 1.0f) {
        gBits = 0x0000FF00u;
    }
    uint32_t rBits = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(colorR * 255.0f)));
    if (colorR > 1.0f) {
        rBits = 0x000000FFu;
    }

    uint32_t posByteOff = count * 4u - 4u;
    uint32_t upByteOff  = upStride * posByteOff;
    uint32_t upStepBytes = upStride << 2;
    uint32_t vByteOff   = (count - 1u) * vCoordStepBytes;

    uint8_t* pCursor = pOutVerts + (static_cast<uint64_t>(count) << 6) - 4;

    do {
        uint64_t off = posByteOff;
        uint32_t pxBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pPosX) + off);
        uint32_t pyBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pPosY) + off);
        uint32_t pzBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pPosZ) + off);
        uint32_t uBits  = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pUCoord) + off);

        float vVal = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVCoord) + vByteOff);
        uint64_t upOff = upByteOff;
        uint32_t upZBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pUpZ) + upOff);
        uint32_t upXBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pUpX) + upOff);
        uint32_t upYBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pUpY) + upOff);

        float dt = curTime - *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pTime) + off);

        uint64_t nextOff = static_cast<uint32_t>(posByteOff + 4u);
        float nextX = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + nextOff);
        float nextY = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + nextOff);
        float nextZ = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + nextOff);

        float dx = nextX - __builtin_bit_cast(float, pxBits);
        float dy = nextY - __builtin_bit_cast(float, pyBits);
        float dz = nextZ - __builtin_bit_cast(float, pzBits);

        float dLenSq = dz * dz + (dx * dx + dy * dy);
        float dInvLen = 1.0f / std::sqrt(dLenSq);
        if ((__builtin_bit_cast(uint32_t, dInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            dInvLen = 0.0f;
        }

        curPointIdxF += -1.0f;
        float tanX = dx * dInvLen;
        float tanY = dy * dInvLen;
        float tanZ = dz * dInvLen;

        float upX = __builtin_bit_cast(float, upXBits);
        float upY = __builtin_bit_cast(float, upYBits);
        float upZ = __builtin_bit_cast(float, upZBits);

        float cx = upX * tanZ - upY * tanY;
        float cy = upY * tanX - upZ * tanZ;
        float cz = upZ * tanY - upX * tanX;

        float cLenSq = cz * cz + (cx * cx + cy * cy);
        float cInvLen = 1.0f / std::sqrt(cLenSq);
        if ((__builtin_bit_cast(uint32_t, cInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            cInvLen = 0.0f;
        }

        float sideX = cInvLen * cx;
        float sideY = cInvLen * cy;
        float sideZ = cInvLen * cz;

        float remPoints = totalMinus1F - curPointIdxF;
        float fadeFactor = (fadeOutCount > remPoints) ? (invFadeOut * remPoints) : 1.0f;
        if (fadeInCount > curPointIdxF) {
            fadeFactor = invFadeIn * curPointIdxF;
        }

        float lifePhase = invLife * dt + (-1.0f);
        float lifeFade  = 0.0f;
        if (!(lifePhase > 0.0f)) {
            lifeFade = (lifePhase < -1.0f) ? 1.0f : -lifePhase;
        }
        float alphaScale = fadeFactor * (lifeFade * lifeFade);

        uint32_t v0Bits = __builtin_bit_cast(uint32_t, vVal + vOffsetBase);
        uint32_t v1Bits = __builtin_bit_cast(uint32_t, vOffsetSum + vVal);

        uint32_t sxi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(sideX * (-127.0f) + 127.0f)));
        uint32_t syi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((sideY * (-127.0f) + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t szi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((sideZ * (-127.0f) + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedSide = szi | (sxi | syi);

        uint32_t txi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(tanX * 127.0f + 127.0f)));
        uint32_t tyi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((tanY * 127.0f + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t tzi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((tanZ * 127.0f + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedTan = tzi | (txi | tyi);

        float finalA = alphaScale * colorA;
        uint32_t rOut = (0.0f > colorR) ? 0u : rBits;
        uint32_t gOut = (0.0f > colorG) ? 0u : gBits;
        uint32_t bOut = (0.0f > colorB) ? 0u : bBits;
        uint32_t aOut = 0u;
        if (!(0.0f > finalA)) {
            aOut = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((finalA * 255.0f) * 16777216.0f))) & 0xFF000000u;
            if (finalA > 1.0f) {
                aOut = 0xFF000000u;
            }
        }
        uint32_t rgba = (aOut | bOut) | (gOut | rOut);

        *reinterpret_cast<uint32_t*>(pCursor - 0x3C) = pxBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x38) = pyBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x34) = pzBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x30) = rgba;
        *reinterpret_cast<uint32_t*>(pCursor - 0x2C) = uBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x28) = v0Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x24) = v1Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x20) = param1Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x1C) = packedSide;
        *reinterpret_cast<uint32_t*>(pCursor - 0x18) = packedTan;
        *reinterpret_cast<uint32_t*>(pCursor - 0x14) = 0u;
        *reinterpret_cast<uint32_t*>(pCursor - 0x10) = 0u;
        *reinterpret_cast<uint32_t*>(pCursor - 0x0C) = upZBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x08) = upXBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x04) = upYBits;
        *reinterpret_cast<uint32_t*>(pCursor + 0x00) = param0Bits;

        upByteOff  -= upStepBytes;
        vByteOff   -= vCoordStepBytes;
        posByteOff -= 4u;
        pCursor    -= 0x40;
    } while (posByteOff != 0xFFFFFFFCu);
}

// =====================================================================
// Function sub_28cf580 (VMA: 0x28cf580, Size: 1557 bytes)
// GXFfx Emit Camera-Facing Ribbon Trail Control Points (64B per point)
// =====================================================================
void sub_28cf580(
    uint8_t* pOutVerts,
    uint32_t count,
    const float* pTime,
    const uint32_t* pPosX,
    const uint32_t* pPosY,
    const uint32_t* pPosZ,
    float curTime,
    float camPosX,
    float camPosY,
    float camPosZ,
    float lifeTime,
    float param0,
    float param1,
    float vOffsetBase,
    const uint32_t* pUCoord,
    const float* pVCoordOpt,
    float vOffsetDelta,
    float colorB,
    float colorG,
    float colorR,
    float colorA,
    uint32_t startIdxOffset,
    uint32_t totalRibbonPoints,
    float fadeInCount,
    float fadeOutCount)
{
    float invLife = (0.0f < lifeTime) ? (1.0f / lifeTime) : -1.0f;
    uint32_t totalMinus1Int = totalRibbonPoints - 1u;
    float invFadeIn  = (fadeInCount > 0.0f) ? (1.0f / fadeInCount) : 1.0f;
    float invFadeOut = (fadeOutCount > 0.0f) ? (1.0f / fadeOutCount) : 1.0f;

    uint32_t param0Bits = __builtin_bit_cast(uint32_t, param0);
    uint32_t param1Bits = __builtin_bit_cast(uint32_t, param1);
    float vOffsetSum = vOffsetBase + vOffsetDelta;
    float totalMinus1F = static_cast<float>(static_cast<int64_t>(totalMinus1Int));

    uint32_t idxSumInt = startIdxOffset + count;
    float curPointIdxF = static_cast<float>(static_cast<int64_t>(idxSumInt));

    static const float kZeroVCoord[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float* pVCoord = (pVCoordOpt != nullptr) ? pVCoordOpt : kZeroVCoord;
    uint32_t vCoordStepBytes = (pVCoordOpt != nullptr) ? 4u : 0u;

    uint32_t bBits = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((colorB * 255.0f) * 65536.0f))) & 0x00FF0000u;
    if (colorB > 1.0f) {
        bBits = 0x00FF0000u;
    }
    uint32_t gBits = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((colorG * 255.0f) * 256.0f))) & 0x0000FF00u;
    if (colorG > 1.0f) {
        gBits = 0x0000FF00u;
    }
    uint32_t rBits = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(colorR * 255.0f)));
    if (colorR > 1.0f) {
        rBits = 0x000000FFu;
    }

    uint32_t vByteOff = (count - 1u) * vCoordStepBytes;
    uint32_t nextPosByteOff = count * 4u;
    uint8_t* pCursor = pOutVerts + (static_cast<uint64_t>(count) << 6) - 4;

    uint32_t curPosByteOff;
    do {
        curPosByteOff = nextPosByteOff - 4u;
        uint64_t off = curPosByteOff;

        uint32_t pxBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pPosX) + off);
        uint32_t pyBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pPosY) + off);
        uint32_t pzBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pPosZ) + off);
        uint32_t uBits  = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pUCoord) + off);

        float vVal = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVCoord) + vByteOff);
        float dt   = curTime - *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pTime) + off);

        float px = __builtin_bit_cast(float, pxBits);
        float py = __builtin_bit_cast(float, pyBits);
        float pz = __builtin_bit_cast(float, pzBits);

        float ex = camPosX - px;
        float ey = camPosY - py;
        float ez = camPosZ - pz;
        float eLenSq = ez * ez + (ex * ex + ey * ey);
        float eInvLen = 1.0f / std::sqrt(eLenSq);
        if ((__builtin_bit_cast(uint32_t, eInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            eInvLen = 0.0f;
        }
        float eyeX = ex * eInvLen;
        float eyeY = ey * eInvLen;
        float eyeZ = ez * eInvLen;

        uint64_t nextOff = nextPosByteOff;
        float dx = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + nextOff) - px;
        float dy = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + nextOff) - py;
        float dz = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + nextOff) - pz;
        float dLenSq = dz * dz + (dx * dx + dy * dy);
        float dInvLen = 1.0f / std::sqrt(dLenSq);
        if ((__builtin_bit_cast(uint32_t, dInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            dInvLen = 0.0f;
        }

        curPointIdxF += -1.0f;
        float tanX = dx * dInvLen;
        float tanY = dy * dInvLen;
        float tanZ = dz * dInvLen;

        float cx = eyeZ * tanY - eyeY * tanZ;
        float cy = eyeX * tanZ - eyeZ * tanX;
        float cz = eyeY * tanX - eyeX * tanY;
        float cLenSq = cz * cz + (cx * cx + cy * cy);
        float cInvLen = 1.0f / std::sqrt(cLenSq);
        if ((__builtin_bit_cast(uint32_t, cInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            cInvLen = 0.0f;
        }

        uint32_t sideXBits = __builtin_bit_cast(uint32_t, cInvLen * cx) ^ 0x80000000u;
        uint32_t sideYBits = __builtin_bit_cast(uint32_t, cInvLen * cy) ^ 0x80000000u;
        uint32_t sideZBits = __builtin_bit_cast(uint32_t, cInvLen * cz) ^ 0x80000000u;

        float remPoints = totalMinus1F - curPointIdxF;
        float fadeFactor = (fadeOutCount > remPoints) ? (invFadeOut * remPoints) : 1.0f;
        if (fadeInCount > curPointIdxF) {
            fadeFactor = invFadeIn * curPointIdxF;
        }

        float lifePhase = invLife * dt + (-1.0f);
        float lifeFade  = 0.0f;
        if (!(lifePhase > 0.0f)) {
            lifeFade = (lifePhase < -1.0f) ? 1.0f : -lifePhase;
        }
        float alphaScale = fadeFactor * (lifeFade * lifeFade);

        uint32_t v0Bits = __builtin_bit_cast(uint32_t, vVal + vOffsetBase);
        uint32_t v1Bits = __builtin_bit_cast(uint32_t, vOffsetSum + vVal);

        uint32_t exi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(eyeX * 127.0f + 127.0f)));
        uint32_t eyi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((eyeY * 127.0f + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t ezi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((eyeZ * 127.0f + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedEye = ezi | (exi | eyi);

        uint32_t txi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(tanX * 127.0f + 127.0f)));
        uint32_t tyi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((tanY * 127.0f + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t tzi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((tanZ * 127.0f + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedTan = tzi | (txi | tyi);

        float finalA = alphaScale * colorA;
        uint32_t rOut = (0.0f > colorR) ? 0u : rBits;
        uint32_t gOut = (0.0f > colorG) ? 0u : gBits;
        uint32_t bOut = (0.0f > colorB) ? 0u : bBits;
        uint32_t aOut = 0u;
        if (!(0.0f > finalA)) {
            aOut = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((finalA * 255.0f) * 16777216.0f))) & 0xFF000000u;
            if (finalA > 1.0f) {
                aOut = 0xFF000000u;
            }
        }
        uint32_t rgba = (aOut | bOut) | (gOut | rOut);

        *reinterpret_cast<uint32_t*>(pCursor - 0x3C) = pxBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x38) = pyBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x34) = pzBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x30) = rgba;
        *reinterpret_cast<uint32_t*>(pCursor - 0x2C) = uBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x28) = v0Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x24) = v1Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x20) = param1Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x1C) = packedEye;
        *reinterpret_cast<uint32_t*>(pCursor - 0x18) = packedTan;
        *reinterpret_cast<uint32_t*>(pCursor - 0x14) = 0u;
        *reinterpret_cast<uint32_t*>(pCursor - 0x10) = 0u;
        *reinterpret_cast<uint32_t*>(pCursor - 0x0C) = sideXBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x08) = sideYBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x04) = sideZBits;
        *reinterpret_cast<uint32_t*>(pCursor + 0x00) = param0Bits;

        vByteOff -= vCoordStepBytes;
        pCursor  -= 0x40;
        nextPosByteOff = curPosByteOff;
    } while (curPosByteOff != 0u);
}

// =====================================================================
// Function sub_28ceec0 (VMA: 0x28ceec0, Size: 1714 bytes)
// GXFfx Emit Custom-Up-Vector Ribbon Strip Vertex Pairs (2x56B = 112B per segment)
// =====================================================================
void sub_28ceec0(
    uint8_t* pOutVerts,
    uint32_t count,
    const float* pTime,
    const float* pPosX,
    const float* pPosY,
    const float* pPosZ,
    float curTime,
    float lifeTime,
    float uvParam,
    float halfWidth,
    float vOffsetBase,
    float vOffsetDelta,
    float colorB,
    float colorG,
    const uint32_t* pUCoord,
    const float* pVCoordOpt,
    const float* pUpZ,
    const float* pUpX,
    const float* pUpY,
    uint32_t upStride,
    float colorR,
    float colorA,
    uint32_t startIdxOffset,
    uint32_t totalRibbonPoints,
    float fadeInCount,
    float fadeOutCount)
{
    float invLife = (0.0f < lifeTime) ? (1.0f / lifeTime) : -1.0f;
    uint32_t totalMinus1Int = totalRibbonPoints - 1u;
    float invFadeIn  = (fadeInCount > 0.0f) ? (1.0f / fadeInCount) : 1.0f;
    float invFadeOut = (fadeOutCount > 0.0f) ? (1.0f / fadeOutCount) : 1.0f;

    uint32_t uvParamBits = __builtin_bit_cast(uint32_t, uvParam);
    float vOffsetSum = vOffsetBase + vOffsetDelta;
    float totalMinus1F = static_cast<float>(static_cast<int64_t>(totalMinus1Int));

    static const float kZeroVCoord[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float* pVCoord = (pVCoordOpt != nullptr) ? pVCoordOpt : kZeroVCoord;
    uint32_t vCoordStepBytes = (pVCoordOpt != nullptr) ? 4u : 0u;

    uint32_t idxSumInt = startIdxOffset + count;
    float curPointIdxF = static_cast<float>(static_cast<int64_t>(idxSumInt));

    uint64_t totalVertPairBytes = static_cast<uint64_t>(count * 2u) * 0x38ULL;
    uint8_t* pCursor = pOutVerts + totalVertPairBytes;
    uint64_t fixedNextByteOff = static_cast<uint32_t>(count * 4u + 4u);

    uint32_t bBits = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((colorB * 255.0f) * 65536.0f))) & 0x00FF0000u;
    if (colorB > 1.0f) {
        bBits = 0x00FF0000u;
    }
    uint32_t gBits = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((colorG * 255.0f) * 256.0f))) & 0x0000FF00u;
    if (colorG > 1.0f) {
        gBits = 0x0000FF00u;
    }
    uint32_t rBits = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(colorR * 255.0f)));
    if (colorR > 1.0f) {
        rBits = 0x000000FFu;
    }

    uint32_t posByteOff  = count * 4u - 4u;
    uint32_t upByteOff   = upStride * posByteOff;
    uint32_t upStepBytes = upStride << 2;
    uint32_t vByteOff    = (count - 1u) * vCoordStepBytes;

    do {
        uint64_t off = posByteOff;
        uint32_t uBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pUCoord) + off);
        float vVal     = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVCoord) + vByteOff);
        uint64_t upOff = upByteOff;

        float dt = curTime - *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pTime) + off);

        float px = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + off);
        float py = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + off);
        float pz = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + off);

        uint64_t nextOff = static_cast<uint32_t>(posByteOff + 4u);
        float nextX = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + nextOff);
        float nextY = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + nextOff);
        float nextZ = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + nextOff);

        float dx = nextX - px;
        float dy = nextY - py;
        float dz = nextZ - pz;

        float dLenSq = dz * dz + (dx * dx + dy * dy);
        float dInvLen = 1.0f / std::sqrt(dLenSq);
        if ((__builtin_bit_cast(uint32_t, dInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            dInvLen = 0.0f;
        }

        curPointIdxF += -1.0f;
        float tanX = dx * dInvLen;
        float tanY = dy * dInvLen;
        float tanZ = dz * dInvLen;

        float upY = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pUpY) + upOff);
        float upZ = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pUpZ) + upOff);
        float upX = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pUpX) + upOff);

        float cx = upX * tanZ - upY * tanY;
        float cy = upY * tanX - upZ * tanZ;
        float cz = upZ * tanY - upX * tanX;

        float cLenSq = cz * cz + (cx * cx + cy * cy);
        float cInvLen = 1.0f / std::sqrt(cLenSq);
        if ((__builtin_bit_cast(uint32_t, cInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            cInvLen = 0.0f;
        }

        float sideX = cInvLen * cx;
        float sideY = cInvLen * cy;
        float sideZ = cInvLen * cz;

        float remPoints = totalMinus1F - curPointIdxF;
        float fadeFactor = (fadeOutCount > remPoints) ? (invFadeOut * remPoints) : 1.0f;
        if (fadeInCount > curPointIdxF) {
            fadeFactor = invFadeIn * curPointIdxF;
        }

        float lifePhase = invLife * dt + (-1.0f);
        float lifeFade  = 0.0f;
        if (!(lifePhase > 0.0f)) {
            lifeFade = (lifePhase < -1.0f) ? 1.0f : -lifePhase;
        }
        float alphaScale = fadeFactor * (lifeFade * lifeFade);

        uint32_t v0Bits = __builtin_bit_cast(uint32_t, vVal + vOffsetBase);
        uint32_t v1Bits = __builtin_bit_cast(uint32_t, vOffsetSum + vVal);

        float offX = upZ * halfWidth;
        float offY = upX * halfWidth;
        float offZ = upY * halfWidth;

        uint32_t pos0XBits = __builtin_bit_cast(uint32_t, px + offX);
        uint32_t pos0YBits = __builtin_bit_cast(uint32_t, py + offY);
        uint32_t pos0ZBits = __builtin_bit_cast(uint32_t, pz + offZ);

        uint32_t pos1XBits = __builtin_bit_cast(uint32_t, offX - px) ^ 0x80000000u;
        uint32_t pos1YBits = __builtin_bit_cast(uint32_t, offY - py) ^ 0x80000000u;
        uint32_t pos1ZBits = __builtin_bit_cast(uint32_t, offZ - pz) ^ 0x80000000u;

        uint32_t sxi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(sideX * (-127.0f) + 127.0f)));
        uint32_t syi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((sideY * (-127.0f) + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t szi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((sideZ * (-127.0f) + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedSide = szi | (sxi | syi);

        uint32_t uxi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(upZ * 127.0f + 127.0f)));
        uint32_t uyi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((upX * 127.0f + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t uzi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((upY * 127.0f + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedUp = uzi | (uxi | uyi);

        uint32_t txi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(tanX * 127.0f + 127.0f)));
        uint32_t tyi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((tanY * 127.0f + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t tzi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((tanZ * 127.0f + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedTan = tzi | (txi | tyi);

        float finalA = alphaScale * colorA;
        uint32_t rOut = (0.0f > colorR) ? 0u : rBits;
        uint32_t gOut = (0.0f > colorG) ? 0u : gBits;
        uint32_t bOut = (0.0f > colorB) ? 0u : bBits;
        uint32_t aOut = 0u;
        if (!(0.0f > finalA)) {
            aOut = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((finalA * 255.0f) * 16777216.0f))) & 0xFF000000u;
            if (finalA > 1.0f) {
                aOut = 0xFF000000u;
            }
        }
        uint32_t rgba = (aOut | bOut) | (gOut | rOut);

        // First vertex (offset -0x70 .. -0x39)
        *reinterpret_cast<uint32_t*>(pCursor - 0x70) = pos0XBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x6C) = pos0YBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x68) = pos0ZBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x64) = packedSide;
        *reinterpret_cast<uint32_t*>(pCursor - 0x60) = packedUp;
        *reinterpret_cast<uint32_t*>(pCursor - 0x5C) = packedTan;
        *reinterpret_cast<uint32_t*>(pCursor - 0x58) = rgba;
        *reinterpret_cast<uint32_t*>(pCursor - 0x54) = uBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x50) = v0Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x4C) = uvParamBits;
        *reinterpret_cast<uint64_t*>(pCursor - 0x48) = 0ULL;
        *reinterpret_cast<uint64_t*>(pCursor - 0x40) = 0ULL;

        // Second vertex (offset -0x38 .. -0x01)
        *reinterpret_cast<uint32_t*>(pCursor - 0x38) = pos1XBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x34) = pos1YBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x30) = pos1ZBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x2C) = packedSide;
        *reinterpret_cast<uint32_t*>(pCursor - 0x28) = packedUp;
        *reinterpret_cast<uint32_t*>(pCursor - 0x24) = packedTan;
        *reinterpret_cast<uint32_t*>(pCursor - 0x20) = rgba;
        *reinterpret_cast<uint32_t*>(pCursor - 0x1C) = uBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x18) = v1Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x14) = uvParamBits;
        *reinterpret_cast<uint64_t*>(pCursor - 0x10) = 0ULL;
        *reinterpret_cast<uint64_t*>(pCursor - 0x08) = 0ULL;

        pCursor    -= 0x70;
        upByteOff  -= upStepBytes;
        vByteOff   -= vCoordStepBytes;
        posByteOff -= 4u;
    } while (posByteOff != 0xFFFFFFFCu);
}

// =====================================================================
// Function sub_28ce7b0 (VMA: 0x28ce7b0, Size: 1800 bytes)
// GXFfx Emit Camera-Facing Ribbon Strip Vertex Pairs (2x56B = 112B per segment)
// =====================================================================
void sub_28ce7b0(
    uint8_t* pOutVerts,
    uint32_t count,
    const float* pTime,
    const float* pPosX,
    const float* pPosY,
    const float* pPosZ,
    float curTime,
    float camPosX,
    float camPosY,
    float camPosZ,
    float lifeTime,
    float uvParam,
    float halfWidth,
    float vOffsetBase,
    const uint32_t* pUCoord,
    const float* pVCoordOpt,
    float vOffsetDelta,
    float colorB,
    float colorG,
    float colorR,
    float colorA,
    uint32_t startIdxOffset,
    uint32_t totalRibbonPoints,
    float fadeInCount,
    float fadeOutCount)
{
    float invLife = (0.0f < lifeTime) ? (1.0f / lifeTime) : -1.0f;
    uint32_t totalMinus1Int = totalRibbonPoints - 1u;
    float invFadeIn  = (fadeInCount > 0.0f) ? (1.0f / fadeInCount) : 1.0f;
    float invFadeOut = (fadeOutCount > 0.0f) ? (1.0f / fadeOutCount) : 1.0f;

    uint32_t uvParamBits = __builtin_bit_cast(uint32_t, uvParam);
    float vOffsetSum = vOffsetBase + vOffsetDelta;
    float totalMinus1F = static_cast<float>(static_cast<int64_t>(totalMinus1Int));

    uint32_t idxSumInt = startIdxOffset + count;
    float curPointIdxF = static_cast<float>(static_cast<int64_t>(idxSumInt));

    static const float kZeroVCoord[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float* pVCoord = (pVCoordOpt != nullptr) ? pVCoordOpt : kZeroVCoord;
    uint32_t vCoordStepBytes = (pVCoordOpt != nullptr) ? 4u : 0u;

    uint32_t bBits = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((colorB * 255.0f) * 65536.0f))) & 0x00FF0000u;
    if (colorB > 1.0f) {
        bBits = 0x00FF0000u;
    }
    uint32_t gBits = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((colorG * 255.0f) * 256.0f))) & 0x0000FF00u;
    if (colorG > 1.0f) {
        gBits = 0x0000FF00u;
    }
    uint32_t rBits = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(colorR * 255.0f)));
    if (colorR > 1.0f) {
        rBits = 0x000000FFu;
    }

    uint32_t posByteOff = count * 4u - 4u;
    uint32_t vByteOff   = (count - 1u) * vCoordStepBytes;
    uint64_t totalVertPairBytes = static_cast<uint64_t>(count * 2u) * 0x38ULL;
    uint8_t* pCursor = pOutVerts + totalVertPairBytes - 0x14;

    do {
        uint64_t off = posByteOff;
        uint32_t uBits = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pUCoord) + off);
        float vVal     = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pVCoord) + vByteOff);
        float dt       = curTime - *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pTime) + off);

        float px = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + off);
        float py = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + off);
        float pz = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + off);

        float ex = camPosX - px;
        float ey = camPosY - py;
        float ez = camPosZ - pz;
        float eLenSq = ez * ez + (ex * ex + ey * ey);
        float eInvLen = 1.0f / std::sqrt(eLenSq);
        if ((__builtin_bit_cast(uint32_t, eInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            eInvLen = 0.0f;
        }
        float eyeX = ex * eInvLen;
        float eyeY = ey * eInvLen;
        float eyeZ = ez * eInvLen;

        uint64_t nextOff = static_cast<uint32_t>(posByteOff + 4u);
        float dx = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosX) + nextOff) - px;
        float dy = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosY) + nextOff) - py;
        float dz = *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pPosZ) + nextOff) - pz;
        float dLenSq = dz * dz + (dx * dx + dy * dy);
        float dInvLen = 1.0f / std::sqrt(dLenSq);
        if ((__builtin_bit_cast(uint32_t, dInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            dInvLen = 0.0f;
        }

        curPointIdxF += -1.0f;
        float tanX = dx * dInvLen;
        float tanY = dy * dInvLen;
        float tanZ = dz * dInvLen;

        float cx = eyeZ * tanY - eyeY * tanZ;
        float cy = eyeX * tanZ - eyeZ * tanX;
        float cz = eyeY * tanX - eyeX * tanY;
        float cLenSq = cz * cz + (cx * cx + cy * cy);
        float cInvLen = 1.0f / std::sqrt(cLenSq);
        if ((__builtin_bit_cast(uint32_t, cInvLen) & 0x7FFFFFFFu) == 0x7F800000u) {
            cInvLen = 0.0f;
        }

        float sideX = cInvLen * cx;
        float sideY = cInvLen * cy;
        float sideZ = cInvLen * cz;

        float remPoints = totalMinus1F - curPointIdxF;
        float fadeFactor = (fadeOutCount > remPoints) ? (invFadeOut * remPoints) : 1.0f;
        if (fadeInCount > curPointIdxF) {
            fadeFactor = invFadeIn * curPointIdxF;
        }

        float lifePhase = invLife * dt + (-1.0f);
        float lifeFade  = 0.0f;
        if (!(lifePhase > 0.0f)) {
            lifeFade = (lifePhase < -1.0f) ? 1.0f : -lifePhase;
        }
        float alphaScale = fadeFactor * (lifeFade * lifeFade);

        uint32_t v0Bits = __builtin_bit_cast(uint32_t, vVal + vOffsetBase);
        uint32_t v1Bits = __builtin_bit_cast(uint32_t, vOffsetSum + vVal);

        float offX = sideX * halfWidth;
        float offY = sideY * halfWidth;
        float offZ = sideZ * halfWidth;

        uint32_t pos0XBits = __builtin_bit_cast(uint32_t, px - offX);
        uint32_t pos0YBits = __builtin_bit_cast(uint32_t, py - offY);
        uint32_t pos0ZBits = __builtin_bit_cast(uint32_t, pz - offZ);

        uint32_t pos1XBits = __builtin_bit_cast(uint32_t, (-offX) - px) ^ 0x80000000u;
        uint32_t pos1YBits = __builtin_bit_cast(uint32_t, (-offY) - py) ^ 0x80000000u;
        uint32_t pos1ZBits = __builtin_bit_cast(uint32_t, (-offZ) - pz) ^ 0x80000000u;

        uint32_t exi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(eyeX * 127.0f + 127.0f)));
        uint32_t eyi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((eyeY * 127.0f + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t ezi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((eyeZ * 127.0f + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedEye = ezi | (exi | eyi);

        uint32_t sxi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(sideX * (-127.0f) + 127.0f)));
        uint32_t syi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((sideY * (-127.0f) + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t szi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((sideZ * (-127.0f) + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedSide = szi | (sxi | syi);

        uint32_t txi = static_cast<uint8_t>(_mm_cvttss_si64(_mm_set_ss(tanX * 127.0f + 127.0f)));
        uint32_t tyi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((tanY * 127.0f + 127.0f) * 256.0f))) & 0x0000FF00u;
        uint32_t tzi = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((tanZ * 127.0f + 127.0f) * 65536.0f))) & 0xFFFF0000u;
        uint32_t packedTan = tzi | (txi | tyi);

        float finalA = alphaScale * colorA;
        uint32_t rOut = (0.0f > colorR) ? 0u : rBits;
        uint32_t gOut = (0.0f > colorG) ? 0u : gBits;
        uint32_t bOut = (0.0f > colorB) ? 0u : bBits;
        uint32_t aOut = 0u;
        if (!(0.0f > finalA)) {
            aOut = static_cast<uint32_t>(_mm_cvttss_si64(_mm_set_ss((finalA * 255.0f) * 16777216.0f))) & 0xFF000000u;
            if (finalA > 1.0f) {
                aOut = 0xFF000000u;
            }
        }
        uint32_t rgba = (aOut | bOut) | (gOut | rOut);

        // First vertex (pCursor - 0x5C .. pCursor - 0x25)
        *reinterpret_cast<uint32_t*>(pCursor - 0x5C) = pos0XBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x58) = pos0YBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x54) = pos0ZBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x50) = packedEye;
        *reinterpret_cast<uint32_t*>(pCursor - 0x4C) = packedSide;
        *reinterpret_cast<uint32_t*>(pCursor - 0x48) = packedTan;
        *reinterpret_cast<uint32_t*>(pCursor - 0x44) = rgba;
        *reinterpret_cast<uint32_t*>(pCursor - 0x40) = uBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x3C) = v0Bits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x38) = uvParamBits;
        *reinterpret_cast<uint64_t*>(pCursor - 0x34) = 0ULL;
        *reinterpret_cast<uint64_t*>(pCursor - 0x2C) = 0ULL;

        // Second vertex (pCursor - 0x24 .. pCursor + 0x13)
        *reinterpret_cast<uint32_t*>(pCursor - 0x24) = pos1XBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x20) = pos1YBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x1C) = pos1ZBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x18) = packedEye;
        *reinterpret_cast<uint32_t*>(pCursor - 0x14) = packedSide;
        *reinterpret_cast<uint32_t*>(pCursor - 0x10) = packedTan;
        *reinterpret_cast<uint32_t*>(pCursor - 0x0C) = rgba;
        *reinterpret_cast<uint32_t*>(pCursor - 0x08) = uBits;
        *reinterpret_cast<uint32_t*>(pCursor - 0x04) = v1Bits;
        *reinterpret_cast<uint32_t*>(pCursor + 0x00) = uvParamBits;
        *reinterpret_cast<uint64_t*>(pCursor + 0x04) = 0ULL;
        *reinterpret_cast<uint64_t*>(pCursor + 0x0C) = 0ULL;

        posByteOff -= 4u;
        vByteOff   -= vCoordStepBytes;
        pCursor    -= 0x70;
    } while (posByteOff != 0xFFFFFFFCu);
}

} // extern "C"
