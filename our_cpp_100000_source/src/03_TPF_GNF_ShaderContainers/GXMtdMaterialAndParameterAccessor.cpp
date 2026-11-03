// ============================================================================
// Bloodborne (CUSA03173 v1.09) - Tier 9/10 Container & Material Parsers
// Module: 03_TPF_GNF_ShaderContainers
// File:   GXMtdMaterialAndParameterAccessor.cpp
// Range:  0x26dc380 .. 0x26dfe50 (65 functions, 14,921 bytes, 100% complete)
//
// Subsystem Overview:
//   Implements Dantelion2's binary MTD ("MTD ") Material Definition container,
//   typed shader parameter accessors (bool/bool2/bool3/bool4, int/int2/int3/int4,
//   float/float2/float3/float4), texture sampler slot descriptors, copy-on-write
//   (COW) chunk unsharing hooks, and file/memory stream serialization.
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace FromSoftware {
namespace GX {

// ============================================================================
// Forward Declarations & Core Dantelion2 Runtime Structures
// ============================================================================

struct DLAllocator {
    struct VTable {
        void*    (*dtor)(DLAllocator* self, uint32_t flags);                       // +0x00
        void*    (*dtorDeleting)(DLAllocator* self);                               // +0x08
        uint32_t (*getAllocatorId)(const DLAllocator* self);                       // +0x10
        uint32_t (*getAllocatorTag)(const DLAllocator* self);                      // +0x18
        void     (*getCapabilities)(uint32_t* outCaps,
                                    DLAllocator* selfArg, uint32_t flags);         // +0x20
        void*    pad28[5];                                                         // +0x28..+0x48
        void*    (*allocate)(DLAllocator* self, size_t bytes);                     // +0x50
        void*    (*allocateAligned)(DLAllocator* self, size_t bytes, size_t align);// +0x58
        void*    pad60[2];                                                         // +0x60..+0x68
        void     (*deallocate)(DLAllocator* self, void* ptr);                      // +0x70
    };
    VTable* vptr;
};

struct DLStringA {
    void*        aux00;        // +0x00
    union {
        char     sso[16];
        char*    ptr;
    }            buf;          // +0x08
    uint64_t     length;       // +0x18
    uint64_t     capacity;     // +0x20
    DLAllocator* allocator;    // +0x28
    char         fillChar;     // +0x30
    uint8_t      pad31[7];     // +0x31

    char* c_str() {
        return (capacity >= 16) ? buf.ptr : buf.sso;
    }
    const char* c_str() const {
        return (capacity >= 16) ? buf.ptr : buf.sso;
    }
};

struct DLStringW {
    void*        aux00;        // +0x00
    union {
        char16_t  sso[8];
        char16_t* ptr;
    }            buf;          // +0x08
    uint64_t     length;       // +0x18
    uint64_t     capacity;     // +0x20
    DLAllocator* allocator;    // +0x28
    char         fillChar;     // +0x30
    uint8_t      pad31[7];     // +0x31

    char16_t* c_str() {
        return (capacity >= 8) ? buf.ptr : buf.sso;
    }
    const char16_t* c_str() const {
        return (capacity >= 8) ? buf.ptr : buf.sso;
    }
};

// Copy-on-write (COW) serialized block field inside an MTD chunk (+0x68 or +0xA0)
struct GXMtdBlockField {
    struct VTable {
        void* pad00[15];
        void  (*onResizeOrReset)(GXMtdBlockField* self, uint32_t flags);           // +0x78
        void* pad80[3];
        void  (*assignRawBytes)(GXMtdBlockField* self, const void* src, uint32_t len); // +0x98
        void* padA0;
        void  (*commitLayout)(GXMtdBlockField* self);                              // +0xA8
    };
    VTable*   vptr;            // +0x00
    uint8_t   isInitialized;   // +0x08
    uint8_t   pad09[7];        // +0x09
    uint8_t*  dataPtr;         // +0x10
    uint64_t  reserved18;      // +0x18
    int32_t   elementCount;    // +0x20 (at +0x88 when field is at +0x68)
    uint32_t  pad24;           // +0x24
    void*     ownerChunk;      // +0x28 (at +0x90 when field is at +0x68; +0xC8 when at +0xA0)
    void*     ownerNode;       // +0x30 (at +0x98 when field is at +0x68; +0xD0 when at +0xA0)
};

// Dynamic block pointer array inside an MTD block (+0xD8, +0xA0, or +0xD0)
struct GXMtdBlockArray {
    void*     vptr;            // +0x00
    uint64_t  reserved08;      // +0x08
    void**    elements;        // +0x10
    uint32_t  capacity;        // +0x18
    int32_t   count;           // +0x1C
    uint8_t   pad20[0x10];     // +0x20..+0x2F
};

// Base polymorphic MTD block node
struct GXMtdBlockObject {
    struct VTable {
        void  (*dtor)(GXMtdBlockObject* self);                                     // +0x00
        void* (*deletingDtor)(GXMtdBlockObject* self);                             // +0x08
        bool  (*deserializeFromStream)(GXMtdBlockObject* self, void* stream);      // +0x10
        bool  (*isSharedChunk)(GXMtdBlockObject* self);                            // +0x18
        void* pad20[4];                                                            // +0x20..+0x38
        void  (*initializeDefaults)(GXMtdBlockObject* self);                       // +0x40
    };
    VTable* vptr;              // +0x00
};

// 0xA0-byte typed parameter value block (bool[1..4], int[1..4], or float[1..4])
struct GXMtdValueBlock : GXMtdBlockObject {
    uint8_t          basePad[0x60]; // +0x08..+0x67
    GXMtdBlockField  valueField;    // +0x68..+0x9F (+0x78 = dataPtr, +0x88 = elementCount, +0x90 = ownerChunk, +0x98 = ownerNode)
};

// 0x138-byte MTD shader parameter node (constructed via 0x26e26b0)
struct GXMtdParameterNode : GXMtdBlockObject {
    uint8_t         basePad[0x60];  // +0x08..+0x67
    GXMtdBlockField nameField;      // +0x68..+0x9F (+0x78 = name char*, +0x90 = ownerChunk, +0x98 = ownerNode)
    GXMtdBlockField typeField;      // +0xA0..+0xD7 (+0xB0 = type char*, +0xC8 = ownerChunk, +0xD0 = ownerNode)
    GXMtdBlockArray valueBlocks;    // +0xD8..+0x107 (holds 1 GXMtdValueBlock*)
    uint8_t         tailPad[0x30];  // +0x108..+0x137
};

// 0xD0-byte MTD texture sampler slot node (constructed via 0x26e38b0)
struct GXMtdTextureSlotNode : GXMtdBlockObject {
    uint8_t         basePad[0x60];  // +0x08..+0x67
    GXMtdBlockField typeNameField;  // +0x68..+0x9F (+0x78 = char* samplerName, +0x90 = ownerChunk, +0x98 = ownerNode)
    uint8_t         padA0[0x10];    // +0xA0..+0xAF
    uint32_t        uvIndex;        // +0xB0
    uint8_t         padB4[0x14];    // +0xB4..+0xC7
    uint32_t        shaderDataIdx;  // +0xC8
    uint32_t        padCC;          // +0xCC
};

// 0x130-byte MTD parameter & texture lists block (constructed via 0x26e0950)
struct GXMtdListsBlock : GXMtdBlockObject {
    uint8_t         basePad[0x98];  // +0x08..+0x9F
    GXMtdBlockArray paramNodes;     // +0xA0..+0xCF (elements of GXMtdParameterNode*)
    GXMtdBlockArray textureSlots;   // +0xD0..+0xFF (elements of GXMtdTextureSlotNode*)
    uint8_t         tailPad[0x30];  // +0x100..+0x12F
};

// MTD data block at rootBlock + 0x70
struct GXMtdDataBlock : GXMtdBlockObject {
    uint8_t         basePad[0x60];    // +0x08..+0x67
    GXMtdBlockField shaderPathField;  // +0x68..+0x9F (+0x78 = char* shaderPath)
    GXMtdBlockField descriptionField; // +0xA0..+0xD7 (+0xB0 = char* description)
    GXMtdBlockArray listsBlockArray;  // +0xD8..+0x107 (holds 1 GXMtdListsBlock* at index 0)
};

// MTD header block at rootBlock + 0x68
struct GXMtdHeaderBlock : GXMtdBlockObject {
    uint8_t         basePad[0x60];    // +0x08..+0x67
    GXMtdBlockField magicField;       // +0x68..+0x9F (+0x78 = char[4] "MTD ")
    uint8_t         padA0[0x10];      // +0xA0..+0xAF
    uint32_t        version;          // +0xB0
};

// 0x108-byte MTD root container block (constructed via 0x26e2cd0)
struct GXMtdRootBlock : GXMtdBlockObject {
    uint8_t           pad08[0x24];    // +0x08..+0x2B
    char              magic[4];       // +0x2C..+0x2F ("MTD ")
    uint8_t           pad30[0x38];    // +0x30..+0x67
    GXMtdHeaderBlock* headerBlock;    // +0x68
    GXMtdDataBlock*   dataBlock;      // +0x70
    uint8_t           pad78[0x90];    // +0x78..+0x107
};

// High-level MTD Material Container wrapper (0x10 bytes)
struct GXMtdMaterialContainer {
    GXMtdRootBlock* rootBlock;        // +0x00
    uint32_t        lastError;        // +0x08
    uint32_t        pad0C;            // +0x0C
};

// Accessor handle for an MTD shader parameter (0x18 bytes)
struct GXMtdParameterAccessor {
    void*               vptr;            // +0x00
    GXMtdParameterNode* paramNode;       // +0x08
    int32_t             cachedTypeIndex; // +0x10 (-1 = uncached, 0..3 = bool1..4, 4..7 = int1..4, 8..11 = float1..4)
    uint32_t            pad14;           // +0x14
};

// Accessor handle for an MTD texture sampler slot (0x10 bytes)
struct GXMtdTextureSlotAccessor {
    void*                 vptr;          // +0x00
    GXMtdTextureSlotNode* slotNode;      // +0x08
};

// 16-byte sync state pair used by GXMtdMaterialContainer_SyncParametersFromDescriptorList
struct GXMtdParamSyncPair {
    uint32_t            syncState;       // +0x00 (0 = missing, 1 = type mismatch/convert, 2 = exact match, 3 = default)
    uint32_t            pad04;           // +0x04
    GXMtdParameterNode* existingNode;    // +0x08
};

// Dantelion2 vector<GXMtdParamSyncPair> (0x28 bytes)
struct GXMtdParamSyncPairVector {
    void*               aux00;           // +0x00
    GXMtdParamSyncPair* begin;           // +0x08
    GXMtdParamSyncPair* end;             // +0x10
    GXMtdParamSyncPair* capacityEnd;     // +0x18
    DLAllocator*        allocator;       // +0x20
};

// Dantelion2 vector<GXMtdParameterNode*> (0x28 bytes)
struct GXMtdNodePtrVector {
    void*                aux00;          // +0x00
    GXMtdParameterNode** begin;          // +0x08
    GXMtdParameterNode** end;            // +0x10
    GXMtdParameterNode** capacityEnd;    // +0x18
    DLAllocator*         allocator;      // +0x20
};

// 0x40-byte Texture Slot Descriptor element in input vector
struct GXMtdTextureSlotDescriptor {
    DLStringW samplerTypeName;           // +0x00..+0x37
    uint32_t  uvIndex;                   // +0x38
    uint32_t  shaderDataIndex;           // +0x3C
};

struct GXMtdTextureSlotDescriptorVector {
    void*                       aux00;       // +0x00
    GXMtdTextureSlotDescriptor* begin;       // +0x08
    GXMtdTextureSlotDescriptor* end;         // +0x10
    GXMtdTextureSlotDescriptor* capacityEnd; // +0x18
    DLAllocator*                allocator;   // +0x20
};

// 0x50-byte Parameter Descriptor element in input vector
struct GXMtdParameterDescriptor {
    DLStringW paramName;                 // +0x00..+0x37
    uint32_t  paramType;                 // +0x38 (0..11)
    uint32_t  valueData[4];              // +0x3C..+0x4B
    uint32_t  pad4C;                     // +0x4C
};

struct GXMtdParameterDescriptorVector {
    void*                     aux00;       // +0x00
    GXMtdParameterDescriptor* begin;       // +0x08
    GXMtdParameterDescriptor* end;         // +0x10
    GXMtdParameterDescriptor* capacityEnd; // +0x18
    DLAllocator*              allocator;   // +0x20
};

struct GXMtdConvertedValue16 {
    uint64_t lo;
    uint64_t hi;
};

} // namespace GX
} // namespace FromSoftware

// ============================================================================
// External Engine & Dantelion2 Helper Declarations
// ============================================================================
extern "C" {

// COW chunk unshare / dirty notification (0x26eb940, 0x26ebdc0)
bool     GXMtdChunk_PrepareWrite(void* chunk, void* fieldObj, void* ownerNode, int32_t byteDelta);
void     GXMtdChunk_CommitWrite(void* chunk, void* fieldObj, void* ownerNode);

// Block array operations (0x26ea090, 0x26ea3e0, 0x26ea380, 0x26e9ee0)
int32_t  GXMtdBlockArray_GetCount(const FromSoftware::GX::GXMtdBlockArray* arr);
void*    GXMtdBlockArray_GetAt(const FromSoftware::GX::GXMtdBlockArray* arr, int32_t index);
void     GXMtdBlockArray_Resize(FromSoftware::GX::GXMtdBlockArray* arr, int32_t newCount);
void     GXMtdBlockArray_SetAt(FromSoftware::GX::GXMtdBlockArray* arr, int32_t index, void* element);

// Block field string assignment (0x26e7d60, 0x26e7f40, 0x26e80c0)
bool     GXMtdBlockField_SetStringWide(FromSoftware::GX::GXMtdBlockField* field, const FromSoftware::GX::DLStringW* wideStr);
bool     GXMtdBlockField_SetStringAscii(FromSoftware::GX::GXMtdBlockField* field, const FromSoftware::GX::DLStringA* asciiStr);
bool     GXMtdBlockField_SetCStringAscii(FromSoftware::GX::GXMtdBlockField* field, const char* cstr);

// Parameter type string lookup & parsing (0x26e0280, 0x26e02c0)
const char* GXMtdParamType_GetTypeString(uint32_t typeIndex);
int32_t     GXMtdParamType_ParseFromString(const char* typeStr);

// Block constructors & clone (0x26e0950, 0x26e26b0, 0x26e2cd0, 0x26e38b0, 0x26e41b0, 0x26e5650, 0x26e69c0, 0x26edf60, 0x26ecc50)
void     GXMtdListsBlock_Ctor(void* self);
void     GXMtdParameterNode_Ctor(void* self);
void     GXMtdRootBlock_Ctor(void* self);
void     GXMtdTextureSlotNode_Ctor(void* self);
void     GXMtdFloatValueBlock_Ctor(void* self);
void     GXMtdIntValueBlock_Ctor(void* self);
void     GXMtdBoolValueBlock_Ctor(void* self);
void*    GXMtdBlockObject_Clone(const void* srcBlock);
bool     GXMtdRootBlock_SerializeToStream(void* rootBlock, void* fileStream);

// TextureSlotAccessor lifecycle (0x26e0410, 0x26e0460, 0x26e0480)
void     GXMtdTextureSlotAccessor_CtorConst(FromSoftware::GX::GXMtdTextureSlotAccessor* self);
void     GXMtdTextureSlotAccessor_Dtor(FromSoftware::GX::GXMtdTextureSlotAccessor* self);
void     GXMtdTextureSlotAccessor_BindNode(FromSoftware::GX::GXMtdTextureSlotAccessor* self, FromSoftware::GX::GXMtdTextureSlotNode* node);

// Parameter value conversion helper (0x26dbed0, in Batch 2)
FromSoftware::GX::GXMtdConvertedValue16 GXMtdParamValue_ConvertBetweenTypes(
    FromSoftware::GX::GXMtdMaterialContainer* container,
    const void* srcVal4,
    uint32_t srcType,
    const void* defaultVal4,
    uint32_t dstType);

// File & Memory Stream I/O (0x270bc00..0x270c900)
void     DLBufferedFileStream_Ctor(void* stream, uint32_t bufferSize);
void     DLBufferedFileStream_Dtor(void* stream);
bool     DLBufferedFileStream_OpenRead(void* stream, const char* filePath);
bool     DLBufferedFileStream_OpenWrite(void* stream, const char* filePath);
void     DLBufferedFileStream_Close(void* stream);
bool     DLBufferedFileStream_IsOpen(const void* stream);
void     DLMemoryReadStream_Ctor(void* stream);
void     DLMemoryReadStream_Dtor(void* stream);
bool     DLMemoryReadStream_OpenBuffer(void* stream, const void* buffer, uint32_t byteSize);
void     DLMemoryReadStream_Close(void* stream);
bool     DLMemoryReadStream_IsOpen(const void* stream);

// String & Allocator utilities
void*    GXMtdDefaultContainerHeap_GetHolder();                                    // 0x26e7bf0
FromSoftware::GX::DLAllocator* GXMtdContainerHeapHolder_GetAllocator(void* holder);// 0x26e7bd0
FromSoftware::GX::DLAllocator* DLSystem_GetDefaultStringAllocator();               // 0x2ba2b10
static inline void DLDebug_LogContainerHeapError(uint64_t fileOrZero, uint32_t lineNo, const char* guestMsg) {
    reinterpret_cast<void (*)(uint64_t, uint32_t, const char*)>(GX_VMA(0x20b55b0ULL))(fileOrZero, lineNo, guestMsg);
}
size_t   DLCharTraitsW_Length(const char16_t* str);                                // 0x2bbf228
size_t   DLCharTraitsA_Length(const char* str);                                    // 0x2bbe608
int      DLCharTraitsA_Compare(const char* s1, const char* s2);                    // 0x2bbe568
void     DLOperator_Delete(void* ptr);                                             // 0x2bbe588
void     DLStd_XlengthError(const char* msg);                                      // 0x2bbf1b8
void     DLTlsAllocator_RecordOutOfMemory(void* tlsSlot, uint32_t flag);           // 0x2bbe978

// Wide <-> MultiByte conversion & DLString resize helpers
uint64_t DLCharset_MeasureWideToMultiByte(const char16_t* wideStr, uint64_t wideLen, const char* fillChar); // 0x20e2dc0
void     DLCharset_ConvertWideToMultiByte(const char* fillChar, char* dstBuf, uint64_t dstLen,
                                          const char16_t* srcWide, uint64_t* ioWideLen);                    // 0x20e2b90
uint64_t DLCharset_MeasureMultiByteToWide(const char* mbStr, uint64_t mbLen, const char* fillChar);         // 0x20e2ff0
void     DLCharset_ConvertMultiByteToWide(const char* fillChar, char16_t* dstWide, uint64_t dstWideLen,
                                          const char* srcMb, uint64_t* ioMbLen);                            // 0x20e2a80
void     DLStringA_ResizeWithChar(FromSoftware::GX::DLStringA* str, uint64_t oldLen, uint64_t newLen, char ch); // 0x2670750
void     DLStringA_AssignBuffer(FromSoftware::GX::DLStringA* str, const char* buf, uint64_t len);           // 0x262d8e0
void     DLStringW_ResizeWithChar(FromSoftware::GX::DLStringW* str, uint64_t oldLen, uint64_t newLen, char16_t ch); // 0x262d7a0

} // extern "C"

namespace FromSoftware {
namespace GX {

// ============================================================================
// Engine Globals & Static Tables Referenced by Batch 1
// ============================================================================
namespace {

// 0x48716a0: Number of scalar components per GXMtdParamType (0..11)
// 0..3: bool, bool2, bool3, bool4 -> 1, 2, 3, 4
// 4..7: int,  int2,  int3,  int4  -> 1, 2, 3, 4
// 8..11: float, float2, float3, float4 -> 1, 2, 3, 4
constexpr uint32_t kMtdParamTypeComponentCounts[12] = {
    1, 2, 3, 4,
    1, 2, 3, 4,
    1, 2, 3, 4
};

using BlockAllocFn = void* (*)(size_t bytes);
using BlockFreeFn  = void  (*)(void* ptr);

// 0x52c45c8 / 0x52c45d0: Global Dantelion2 block allocator/deallocator function pointers
#define g_pfnMtdBlockAllocate   (*reinterpret_cast<BlockAllocFn*>(GX_VMA(0x52c45c8ULL)))
#define g_pfnMtdBlockDeallocate (*reinterpret_cast<BlockFreeFn*>(GX_VMA(0x52c45d0ULL)))

// 0x5540670: Cached default string allocator singleton
#define g_pCachedDefaultStringAllocator (*reinterpret_cast<DLAllocator**>(GX_VMA(0x5540670ULL)))

// 0x47383f8 / 0x47383f5: Default conversion fill characters
#define g_defaultLocaleFillChar  (*reinterpret_cast<char*>(GX_VMA(0x47383f8ULL)))
#define g_sentinelLocaleFillChar (*reinterpret_cast<char*>(GX_VMA(0x47383f5ULL)))

// 0x52bbf00, 0x52bbf20, 0x52bbf40: Accessor vtables
#define g_vtableMtdTextureSlotAccessorMutable (reinterpret_cast<void**>(GX_VMA(0x52bbf00ULL)))
#define g_vtableMtdParameterAccessorConst     (reinterpret_cast<void**>(GX_VMA(0x52bbf20ULL)))
#define g_vtableMtdParameterAccessorMutable   (reinterpret_cast<void**>(GX_VMA(0x52bbf40ULL)))

// Thread-local OOM state helper matching fs:0x0 + [0x53e4318 / 0x53e4320]
#define g_tlsTransactionSuppressLog (*reinterpret_cast<int32_t*>(gx_guest_tls_slot(0x53e4318ULL)))
#define g_tlsTransactionErrorSlot   (reinterpret_cast<uint8_t*>(gx_guest_tls_slot(0x53e4320ULL)))

inline void UnshareAndCommitBlockField(GXMtdBlockField* field) {
    if (field->ownerChunk != nullptr) {
        GXMtdChunk_PrepareWrite(field->ownerChunk, field, field->ownerNode, 0);
        if (field->ownerChunk != nullptr) {
            GXMtdChunk_CommitWrite(field->ownerChunk, field, field->ownerNode);
        }
    }
}

inline void UnshareBlockFieldForRead(GXMtdBlockField* field) {
    if (field->ownerChunk != nullptr) {
        GXMtdChunk_PrepareWrite(field->ownerChunk, field, field->ownerNode, 0);
    }
}

inline void EnsureParameterTypeResolved(GXMtdParameterAccessor* self, GXMtdParameterNode* paramNode) {
    if (self->cachedTypeIndex == -1) {
        UnshareAndCommitBlockField(&paramNode->typeField);
        self->cachedTypeIndex = GXMtdParamType_ParseFromString(
            reinterpret_cast<const char*>(paramNode->typeField.dataPtr));
    }
}

} // anonymous namespace

extern "C" {

// Forward declarations of internal Batch 1 functions called before definition
bool GXMtdParameterAccessor_SetFloatComponents(
    GXMtdParameterAccessor* self, float c0, float c1, float c2, float c3);
bool GXMtdParameterAccessor_GetFloatComponents(
    GXMtdParameterAccessor* self, float* out0, float* out1, float* out2, float* out3);
DLStringA* GXMtdStringConvert_WideToMultiByteString(
    DLStringA* outAscii, const DLStringW* srcWide, const char* pFillChar);
void GXMtdParameterAccessor_CtorMutable(GXMtdParameterAccessor* self);
void GXMtdParameterAccessor_CtorConst(GXMtdParameterAccessor* self);
void GXMtdParameterAccessor_Dtor(GXMtdParameterAccessor* self);
void GXMtdParameterAccessor_BindNode(GXMtdParameterAccessor* self, GXMtdParameterNode* node);
GXMtdParameterAccessor* GXMtdParameterAccessor_CopyAssign(
    GXMtdParameterAccessor* self, const GXMtdParameterAccessor* rhs);
int32_t GXMtdParameterAccessor_GetResolvedType(GXMtdParameterAccessor* self);
bool GXMtdParameterAccessor_GetNameAscii(GXMtdParameterAccessor* self, DLStringA* outName);
bool GXMtdParameterAccessor_SetNameWide(GXMtdParameterAccessor* self, const DLStringW* wideName);
bool GXMtdParameterAccessor_SetTypeAndAllocateValueBlock(GXMtdParameterAccessor* self, uint32_t typeIndex);
bool GXMtdParameterAccessor_SetValueFromBuffer4(GXMtdParameterAccessor* self, const void* pValues4);
bool GXMtdParameterAccessor_GetValueToBuffer4(GXMtdParameterAccessor* self, void* outValues4);
int32_t GXMtdMaterialContainer_FindParameterIndexByNameAscii(
    const GXMtdMaterialContainer* self, const DLStringA* paramName);
bool GXMtdMaterialContainer_SaveToFileAscii(
    GXMtdMaterialContainer* self, const DLStringA* filePath);
bool GXMtdMaterialContainer_LoadFromFileAscii(
    GXMtdMaterialContainer* self, const DLStringA* filePath);
void GXMtdNodePtrVector_Reallocate(GXMtdNodePtrVector* vec, uint64_t newCapacity);
void GXMtdVector_ThrowLengthError_NodePtr(void* vec);
void GXMtdVector_ThrowLengthError_ParamSync(void* vec);

// ============================================================================
// 1. VMA 0x26dfe50 (459 B) — GXMtdParameterAccessor_SetBoolComponents
//    Writes up to 4 boolean (uint8_t) components (c0..c3) into the parameter's
//    value block after unsharing its COW chunk.
// ============================================================================
bool GXMtdParameterAccessor_SetBoolComponents(
    GXMtdParameterAccessor* self,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2,
    uint8_t c3)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }

    EnsureParameterTypeResolved(self, paramNode);

    GXMtdBlockArray* valArr = &paramNode->valueBlocks;
    if (GXMtdBlockArray_GetCount(valArr) <= 0) {
        return false;
    }

    GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
    if (valBlock == nullptr) {
        return false;
    }

    int32_t compCount = valBlock->valueField.elementCount;
    if (compCount <= 0) {
        return true;
    }

    GXMtdBlockField* vf = &valBlock->valueField;
    UnshareAndCommitBlockField(vf);
    vf->dataPtr[0] = c0;

    if (compCount >= 2) {
        vf->dataPtr[1] = c1;
    }
    if (compCount >= 3) {
        vf->dataPtr[2] = c2;
    }
    if (compCount >= 4) {
        vf->dataPtr[3] = c3;
    }
    return true;
}

// ============================================================================
// 2. VMA 0x26dfc70 (478 B) — GXMtdParameterAccessor_SetFloatComponents
//    Writes up to 4 float32 components (c0..c3) into the parameter's value block.
// ============================================================================
bool GXMtdParameterAccessor_SetFloatComponents(
    GXMtdParameterAccessor* self,
    float c0,
    float c1,
    float c2,
    float c3)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }

    EnsureParameterTypeResolved(self, paramNode);

    GXMtdBlockArray* valArr = &paramNode->valueBlocks;
    if (GXMtdBlockArray_GetCount(valArr) <= 0) {
        return false;
    }

    GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
    if (valBlock == nullptr) {
        return false;
    }

    int32_t compCount = valBlock->valueField.elementCount;
    if (compCount <= 0) {
        return true;
    }

    GXMtdBlockField* vf = &valBlock->valueField;
    UnshareAndCommitBlockField(vf);
    float* dst = reinterpret_cast<float*>(vf->dataPtr);
    dst[0] = c0;

    if (compCount >= 2) {
        dst[1] = c1;
    }
    if (compCount >= 3) {
        dst[2] = c2;
    }
    if (compCount >= 4) {
        dst[3] = c3;
    }
    return true;
}

// ============================================================================
// 3. VMA 0x26df8b0 (947 B) — GXMtdParameterAccessor_SetValueFromBuffer4
//    Writes a 4-component value buffer (pValues4) into the parameter node,
//    dispatching by cachedTypeIndex (0..3 bool, 4..7 int, 8..11 float).
// ============================================================================
bool GXMtdParameterAccessor_SetValueFromBuffer4(
    GXMtdParameterAccessor* self,
    const void* pValues4)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr || pValues4 == nullptr) {
        return false;
    }

    EnsureParameterTypeResolved(self, paramNode);
    uint32_t typeIdx = static_cast<uint32_t>(self->cachedTypeIndex);
    if (typeIdx > 11u) {
        return false;
    }

    const uint32_t* srcWords = static_cast<const uint32_t*>(pValues4);

    // Types 0..3: bool, bool2, bool3, bool4 (bitmask 0x00F)
    if ((0x00Fu & (1u << typeIdx)) != 0) {
        GXMtdParameterNode* node = self->paramNode;
        if (node == nullptr) {
            return false;
        }
        uint32_t v0 = srcWords[0];
        uint32_t v1 = srcWords[1];
        uint32_t v2 = srcWords[2];
        uint32_t v3 = srcWords[3];

        GXMtdBlockArray* valArr = &node->valueBlocks;
        if (GXMtdBlockArray_GetCount(valArr) <= 0) {
            return false;
        }
        GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
        if (valBlock == nullptr) {
            return false;
        }

        int32_t compCount = valBlock->valueField.elementCount;
        if (compCount > 0) {
            GXMtdBlockField* vf = &valBlock->valueField;
            UnshareAndCommitBlockField(vf);
            vf->dataPtr[0] = static_cast<uint8_t>(v0);
            if (compCount >= 2) {
                vf->dataPtr[1] = static_cast<uint8_t>(v1);
            }
            if (compCount >= 3) {
                vf->dataPtr[2] = static_cast<uint8_t>(v2);
            }
            if (compCount >= 4) {
                vf->dataPtr[3] = static_cast<uint8_t>(v3);
            }
        }
        return true;
    }

    // Types 4..7: int, int2, int3, int4 (bitmask 0x0F0)
    if ((0x0F0u & (1u << typeIdx)) != 0) {
        GXMtdParameterNode* node = self->paramNode;
        if (node == nullptr) {
            return false;
        }
        uint32_t v0 = srcWords[0];
        uint32_t v1 = srcWords[1];
        uint32_t v2 = srcWords[2];
        uint32_t v3 = srcWords[3];

        GXMtdBlockArray* valArr = &node->valueBlocks;
        if (GXMtdBlockArray_GetCount(valArr) <= 0) {
            return false;
        }
        GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
        if (valBlock == nullptr) {
            return false;
        }

        int32_t compCount = valBlock->valueField.elementCount;
        if (compCount > 0) {
            GXMtdBlockField* vf = &valBlock->valueField;
            UnshareAndCommitBlockField(vf);
            uint32_t* dst = reinterpret_cast<uint32_t*>(vf->dataPtr);
            dst[0] = v0;
            if (compCount >= 2) {
                dst[1] = v1;
            }
            if (compCount >= 3) {
                dst[2] = v2;
            }
            if (compCount >= 4) {
                dst[3] = v3;
            }
        }
        return true;
    }

    // Types 8..11: float, float2, float3, float4 (bitmask 0xF00)
    if ((0xF00u & (1u << typeIdx)) != 0) {
        const float* srcFloats = static_cast<const float*>(pValues4);
        return GXMtdParameterAccessor_SetFloatComponents(
            self, srcFloats[0], srcFloats[1], srcFloats[2], srcFloats[3]);
    }

    return false;
}

// ============================================================================
// 4. VMA 0x26df670 (570 B) — GXMtdParameterAccessor_SetTypeAndAllocateValueBlock
//    Sets the parameter type string, clears existing value blocks, allocates
//    and initializes a new 0xA0-byte GXMtdValueBlock (bool/int/float), and
//    attaches it to paramNode->valueBlocks[0].
// ============================================================================
bool GXMtdParameterAccessor_SetTypeAndAllocateValueBlock(
    GXMtdParameterAccessor* self,
    uint32_t typeIndex)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }

    const char* typeNameStr = GXMtdParamType_GetTypeString(typeIndex);
    GXMtdBlockField_SetCStringAscii(&paramNode->typeField, typeNameStr);
    GXMtdBlockArray_Resize(&paramNode->valueBlocks, 0);

    if (typeIndex > 11u) {
        return false;
    }

    GXMtdValueBlock* newValBlock = nullptr;

    // 0..3: bool1..4 (1 byte per component)
    if ((0x00Fu & (1u << typeIndex)) != 0) {
        GXMtdValueBlock* blk = static_cast<GXMtdValueBlock*>(g_pfnMtdBlockAllocate(0xA0));
        if (blk != nullptr) {
            uint32_t compCount = kMtdParamTypeComponentCounts[typeIndex];
            GXMtdBoolValueBlock_Ctor(blk);
            blk->vptr->initializeDefaults(blk);

            GXMtdBlockField* vf = &blk->valueField;
            if (vf->ownerChunk != nullptr) {
                int32_t byteDelta = static_cast<int32_t>(compCount) - vf->elementCount;
                GXMtdChunk_PrepareWrite(vf->ownerChunk, vf, vf->ownerNode, byteDelta);
            }
            vf->vptr->onResizeOrReset(vf, 0);
            vf->elementCount = static_cast<int32_t>(compCount);
            vf->vptr->commitLayout(vf);
            vf->isInitialized = 1;
            if (vf->ownerChunk != nullptr) {
                GXMtdChunk_CommitWrite(vf->ownerChunk, vf, vf->ownerNode);
            }
            newValBlock = blk;
        }
    }
    // 4..7: int1..4 (4 bytes per component)
    else if ((0x0F0u & (1u << typeIndex)) != 0) {
        GXMtdValueBlock* blk = static_cast<GXMtdValueBlock*>(g_pfnMtdBlockAllocate(0xA0));
        if (blk != nullptr) {
            uint32_t compCount = kMtdParamTypeComponentCounts[typeIndex];
            GXMtdIntValueBlock_Ctor(blk);
            blk->vptr->initializeDefaults(blk);

            GXMtdBlockField* vf = &blk->valueField;
            if (vf->ownerChunk != nullptr) {
                int32_t byteDelta = static_cast<int32_t>(compCount * 4) - (vf->elementCount << 2);
                GXMtdChunk_PrepareWrite(vf->ownerChunk, vf, vf->ownerNode, byteDelta);
            }
            vf->vptr->onResizeOrReset(vf, 0);
            vf->elementCount = static_cast<int32_t>(compCount);
            vf->vptr->commitLayout(vf);
            vf->isInitialized = 1;
            if (vf->ownerChunk != nullptr) {
                GXMtdChunk_CommitWrite(vf->ownerChunk, vf, vf->ownerNode);
            }
            newValBlock = blk;
        }
    }
    // 8..11: float1..4 (4 bytes per component)
    else if ((0xF00u & (1u << typeIndex)) != 0) {
        GXMtdValueBlock* blk = static_cast<GXMtdValueBlock*>(g_pfnMtdBlockAllocate(0xA0));
        if (blk != nullptr) {
            uint32_t compCount = kMtdParamTypeComponentCounts[typeIndex];
            GXMtdFloatValueBlock_Ctor(blk);
            blk->vptr->initializeDefaults(blk);

            GXMtdBlockField* vf = &blk->valueField;
            if (vf->ownerChunk != nullptr) {
                int32_t byteDelta = static_cast<int32_t>(compCount * 4) - (vf->elementCount << 2);
                GXMtdChunk_PrepareWrite(vf->ownerChunk, vf, vf->ownerNode, byteDelta);
            }
            vf->vptr->onResizeOrReset(vf, 0);
            vf->elementCount = static_cast<int32_t>(compCount);
            vf->vptr->commitLayout(vf);
            vf->isInitialized = 1;
            if (vf->ownerChunk != nullptr) {
                GXMtdChunk_CommitWrite(vf->ownerChunk, vf, vf->ownerNode);
            }
            newValBlock = blk;
        }
    } else {
        return false;
    }

    if (newValBlock == nullptr) {
        return false;
    }

    GXMtdBlockArray_Resize(&self->paramNode->valueBlocks, 1);
    GXMtdBlockArray_SetAt(&self->paramNode->valueBlocks, 0, newValBlock);
    self->cachedTypeIndex = static_cast<int32_t>(typeIndex);
    return true;
}

// ============================================================================
// 5. VMA 0x26df650 (30 B) — GXMtdParameterAccessor_SetNameAscii
// ============================================================================
bool GXMtdParameterAccessor_SetNameAscii(
    GXMtdParameterAccessor* self,
    const DLStringA* asciiName)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }
    GXMtdBlockField_SetStringAscii(&paramNode->nameField, asciiName);
    return true;
}

// ============================================================================
// 6. VMA 0x26df4d0 (384 B) — GXMtdParameterAccessor_SetNameWide
//    Converts wide string (DLStringW) to multi-byte DLStringA and assigns it
//    to paramNode->nameField.
// ============================================================================
bool GXMtdParameterAccessor_SetNameWide(
    GXMtdParameterAccessor* self,
    const DLStringW* wideName)
{
    char fillCh = g_defaultLocaleFillChar;
    const char16_t* widePtr = wideName->c_str();
    uint64_t wideLen = wideName->length;

    DLAllocator* alloc = g_pCachedDefaultStringAllocator;
    if (alloc == nullptr) {
        alloc = DLSystem_GetDefaultStringAllocator();
        g_pCachedDefaultStringAllocator = alloc;
    }

    uint32_t caps[8]{};
    alloc->vptr->getCapabilities(caps, alloc, 0);
    if ((caps[0] & 0x20u) == 0) {
        DLDebug_LogContainerHeapError(0, 0x3E, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    DLStringA tempAscii{};
    tempAscii.allocator = alloc;
    tempAscii.capacity  = 15;
    tempAscii.length    = 0;
    tempAscii.buf.sso[0] = '\0';
    tempAscii.fillChar  = fillCh;

    if (wideLen == ~0ULL) {
        wideLen = DLCharTraitsW_Length(widePtr);
    }
    if (wideLen != 0) {
        if (fillCh == g_sentinelLocaleFillChar) {
            fillCh = tempAscii.fillChar;
        }
        uint64_t mbLen = DLCharset_MeasureWideToMultiByte(widePtr, wideLen, &fillCh);
        DLStringA_ResizeWithChar(&tempAscii, 0, mbLen, '\0');
        char* dstPtr = tempAscii.c_str();
        uint64_t wideLenCopy = wideLen;
        DLCharset_ConvertWideToMultiByte(&fillCh, dstPtr, mbLen, widePtr, &wideLenCopy);
    }

    bool ok = false;
    if (self->paramNode != nullptr) {
        GXMtdBlockField_SetStringAscii(&self->paramNode->nameField, &tempAscii);
        ok = true;
    }

    if (tempAscii.capacity >= 16) {
        tempAscii.allocator->vptr->deallocate(tempAscii.allocator, tempAscii.buf.ptr);
    }
    return ok;
}

// ============================================================================
// 7. VMA 0x26df4c0 (5 B) — GXMtdParameterAccessor_DeletingDtorMutable
// ============================================================================
void GXMtdParameterAccessor_DeletingDtorMutable(GXMtdParameterAccessor* self) {
    DLOperator_Delete(self);
}

// ============================================================================
// 8. VMA 0x26df4a0 (30 B) — GXMtdParameterAccessor_CtorMutable
// ============================================================================
void GXMtdParameterAccessor_CtorMutable(GXMtdParameterAccessor* self) {
    self->paramNode       = nullptr;
    self->cachedTypeIndex = -1;
    self->vptr            = &g_vtableMtdParameterAccessorMutable[2];
}

// ============================================================================
// 9. VMA 0x26df2f0 (428 B) — GXMtdParameterAccessor_GetIntComponents
//    Reads up to 4 int32_t components from the parameter's value block.
// ============================================================================
bool GXMtdParameterAccessor_GetIntComponents(
    GXMtdParameterAccessor* self,
    int32_t* out0,
    int32_t* out1,
    int32_t* out2,
    int32_t* out3)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }

    EnsureParameterTypeResolved(self, paramNode);

    GXMtdBlockArray* valArr = &paramNode->valueBlocks;
    if (GXMtdBlockArray_GetCount(valArr) <= 0) {
        return false;
    }

    GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
    if (valBlock == nullptr) {
        return false;
    }

    int32_t compCount = valBlock->valueField.elementCount;
    GXMtdBlockField* vf = &valBlock->valueField;
    if (compCount > 0 && (out0 != nullptr || out1 != nullptr || out2 != nullptr || out3 != nullptr)) {
        UnshareBlockFieldForRead(vf);
        const int32_t* src = reinterpret_cast<const int32_t*>(vf->dataPtr);
        if (out0 != nullptr) {
            *out0 = src[0];
        }
        if (out1 != nullptr && compCount >= 2) {
            *out1 = src[1];
        }
        if (out2 != nullptr && compCount >= 3) {
            *out2 = src[2];
        }
        if (out3 != nullptr && compCount >= 4) {
            *out3 = src[3];
        }
    }
    return true;
}

// ============================================================================
// 10. VMA 0x26df130 (436 B) — GXMtdParameterAccessor_GetBoolComponents
//     Reads up to 4 boolean (uint8_t != 0) components from the value block.
// ============================================================================
bool GXMtdParameterAccessor_GetBoolComponents(
    GXMtdParameterAccessor* self,
    bool* out0,
    bool* out1,
    bool* out2,
    bool* out3)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }

    EnsureParameterTypeResolved(self, paramNode);

    GXMtdBlockArray* valArr = &paramNode->valueBlocks;
    if (GXMtdBlockArray_GetCount(valArr) <= 0) {
        return false;
    }

    GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
    if (valBlock == nullptr) {
        return false;
    }

    int32_t compCount = valBlock->valueField.elementCount;
    GXMtdBlockField* vf = &valBlock->valueField;
    if (compCount > 0 && (out0 != nullptr || out1 != nullptr || out2 != nullptr || out3 != nullptr)) {
        UnshareBlockFieldForRead(vf);
        if (out0 != nullptr) {
            *out0 = (vf->dataPtr[0] != 0);
        }
        if (out1 != nullptr && compCount >= 2) {
            *out1 = (vf->dataPtr[1] != 0);
        }
        if (out2 != nullptr && compCount >= 3) {
            *out2 = (vf->dataPtr[2] != 0);
        }
        if (out3 != nullptr && compCount >= 4) {
            *out3 = (vf->dataPtr[3] != 0);
        }
    }
    return true;
}

// ============================================================================
// 11. VMA 0x26def70 (444 B) — GXMtdParameterAccessor_GetFloatComponents
//     Reads up to 4 float32 components from the parameter's value block.
// ============================================================================
bool GXMtdParameterAccessor_GetFloatComponents(
    GXMtdParameterAccessor* self,
    float* out0,
    float* out1,
    float* out2,
    float* out3)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }

    EnsureParameterTypeResolved(self, paramNode);

    GXMtdBlockArray* valArr = &paramNode->valueBlocks;
    if (GXMtdBlockArray_GetCount(valArr) <= 0) {
        return false;
    }

    GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
    if (valBlock == nullptr) {
        return false;
    }

    int32_t compCount = valBlock->valueField.elementCount;
    GXMtdBlockField* vf = &valBlock->valueField;
    if (compCount > 0 && (out0 != nullptr || out1 != nullptr || out2 != nullptr || out3 != nullptr)) {
        UnshareBlockFieldForRead(vf);
        const float* src = reinterpret_cast<const float*>(vf->dataPtr);
        if (out0 != nullptr) {
            *out0 = src[0];
        }
        if (out1 != nullptr && compCount >= 2) {
            *out1 = src[1];
        }
        if (out2 != nullptr && compCount >= 3) {
            *out2 = src[2];
        }
        if (out3 != nullptr && compCount >= 4) {
            *out3 = src[3];
        }
    }
    return true;
}

// ============================================================================
// 12. VMA 0x26dec90 (722 B) — GXMtdParameterAccessor_GetValueToBuffer4
//     Reads up to 4 scalar components into a 16-byte buffer (outValues4)
//     according to cachedTypeIndex (0..3 bool->uint32_t, 4..7 int32, 8..11 float).
// ============================================================================
bool GXMtdParameterAccessor_GetValueToBuffer4(
    GXMtdParameterAccessor* self,
    void* outValues4)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr || outValues4 == nullptr) {
        return false;
    }

    EnsureParameterTypeResolved(self, paramNode);
    uint32_t typeIdx = static_cast<uint32_t>(self->cachedTypeIndex);
    if (typeIdx > 11u) {
        return false;
    }

    uint32_t* dstWords = static_cast<uint32_t*>(outValues4);

    // Types 0..3: bool1..4 zero-extended to uint32_t[4]
    if ((0x00Fu & (1u << typeIdx)) != 0) {
        GXMtdParameterNode* node = self->paramNode;
        if (node == nullptr) {
            return false;
        }
        GXMtdBlockArray* valArr = &node->valueBlocks;
        if (GXMtdBlockArray_GetCount(valArr) <= 0) {
            return false;
        }
        GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
        if (valBlock == nullptr) {
            return false;
        }

        int32_t compCount = valBlock->valueField.elementCount;
        GXMtdBlockField* vf = &valBlock->valueField;
        if (compCount > 0) {
            UnshareBlockFieldForRead(vf);
            dstWords[0] = static_cast<uint32_t>(vf->dataPtr[0]);
            if (compCount >= 2) {
                dstWords[1] = static_cast<uint32_t>(vf->dataPtr[1]);
            }
            if (compCount >= 3) {
                dstWords[2] = static_cast<uint32_t>(vf->dataPtr[2]);
            }
            if (compCount >= 4) {
                dstWords[3] = static_cast<uint32_t>(vf->dataPtr[3]);
            }
        }
        return true;
    }

    // Types 4..7: int1..4 copied as uint32_t[4]
    if ((0x0F0u & (1u << typeIdx)) != 0) {
        GXMtdParameterNode* node = self->paramNode;
        if (node == nullptr) {
            return false;
        }
        GXMtdBlockArray* valArr = &node->valueBlocks;
        if (GXMtdBlockArray_GetCount(valArr) <= 0) {
            return false;
        }
        GXMtdValueBlock* valBlock = static_cast<GXMtdValueBlock*>(GXMtdBlockArray_GetAt(valArr, 0));
        if (valBlock == nullptr) {
            return false;
        }

        int32_t compCount = valBlock->valueField.elementCount;
        GXMtdBlockField* vf = &valBlock->valueField;
        if (compCount > 0) {
            UnshareBlockFieldForRead(vf);
            const uint32_t* src = reinterpret_cast<const uint32_t*>(vf->dataPtr);
            dstWords[0] = src[0];
            if (compCount >= 2) {
                dstWords[1] = src[1];
            }
            if (compCount >= 3) {
                dstWords[2] = src[2];
            }
            if (compCount >= 4) {
                dstWords[3] = src[3];
            }
        }
        return true;
    }

    // Types 8..11: float1..4
    if ((0xF00u & (1u << typeIdx)) != 0) {
        float* dstFloats = static_cast<float*>(outValues4);
        return GXMtdParameterAccessor_GetFloatComponents(
            self, &dstFloats[0], &dstFloats[1], &dstFloats[2], &dstFloats[3]);
    }

    return false;
}

// ============================================================================
// 13. VMA 0x26dec10 (126 B) — GXMtdParameterAccessor_GetResolvedType
//     Resolves (if -1) and returns cachedTypeIndex (0..11, or -1 if invalid).
// ============================================================================
int32_t GXMtdParameterAccessor_GetResolvedType(GXMtdParameterAccessor* self) {
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return -1;
    }
    EnsureParameterTypeResolved(self, paramNode);
    return self->cachedTypeIndex;
}

// ============================================================================
// 14. VMA 0x26deb80 (132 B) — GXMtdParameterAccessor_GetNameAscii
// ============================================================================
bool GXMtdParameterAccessor_GetNameAscii(
    GXMtdParameterAccessor* self,
    DLStringA* outName)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }
    UnshareAndCommitBlockField(&paramNode->nameField);
    const char* nameStr = reinterpret_cast<const char*>(paramNode->nameField.dataPtr);
    size_t len = (nameStr[0] != '\0') ? DLCharTraitsA_Length(nameStr) : 0;
    DLStringA_AssignBuffer(outName, nameStr, len);
    return true;
}

// ============================================================================
// 15. VMA 0x26dea30 (328 B) — GXMtdParameterAccessor_GetNameWide
// ============================================================================
bool GXMtdParameterAccessor_GetNameWide(
    GXMtdParameterAccessor* self,
    DLStringW* outWideName)
{
    GXMtdParameterNode* paramNode = self->paramNode;
    if (paramNode == nullptr) {
        return false;
    }

    UnshareAndCommitBlockField(&paramNode->nameField);
    const char* nameStr = reinterpret_cast<const char*>(paramNode->nameField.dataPtr);
    char fillCh = g_defaultLocaleFillChar;

    outWideName->length = 0;
    char16_t* dstStart = outWideName->c_str();
    dstStart[0] = u'\0';

    uint64_t oldLen = outWideName->length;
    uint64_t mbLen  = DLCharTraitsA_Length(nameStr);
    if (mbLen != 0) {
        if (fillCh == g_sentinelLocaleFillChar) {
            fillCh = outWideName->fillChar;
        }
        uint64_t wideCount = DLCharset_MeasureMultiByteToWide(nameStr, mbLen, &fillCh);
        DLStringW_ResizeWithChar(outWideName, oldLen, wideCount, u'\0');
        char16_t* wideBuf = outWideName->c_str() + oldLen;
        DLCharset_ConvertMultiByteToWide(&fillCh, wideBuf, wideCount, nameStr, &mbLen);
    }
    return true;
}

// ============================================================================
// 16. VMA 0x26dea20 (8 B) — GXMtdParameterAccessor_InvalidateCachedType
// ============================================================================
void GXMtdParameterAccessor_InvalidateCachedType(GXMtdParameterAccessor* self) {
    self->cachedTypeIndex = -1;
}

// ============================================================================
// 17. VMA 0x26dea10 (12 B) — GXMtdParameterAccessor_BindNode
// ============================================================================
void GXMtdParameterAccessor_BindNode(
    GXMtdParameterAccessor* self,
    GXMtdParameterNode* node)
{
    self->cachedTypeIndex = -1;
    self->paramNode       = node;
}

// ============================================================================
// 18. VMA 0x26dea00 (16 B) — GXMtdParameterAccessor_ClearNode
// ============================================================================
void GXMtdParameterAccessor_ClearNode(GXMtdParameterAccessor* self) {
    self->cachedTypeIndex = -1;
    self->paramNode       = nullptr;
}

// ============================================================================
// 19. VMA 0x26de9e0 (30 B) — GXMtdParameterAccessor_Dtor
// ============================================================================
void GXMtdParameterAccessor_Dtor(GXMtdParameterAccessor* self) {
    self->vptr            = &g_vtableMtdParameterAccessorConst[2];
    self->cachedTypeIndex = -1;
    self->paramNode       = nullptr;
}

// ============================================================================
// 20. VMA 0x26de9d0 (5 B) — GXMtdParameterAccessor_DeletingDtorConst
// ============================================================================
void GXMtdParameterAccessor_DeletingDtorConst(GXMtdParameterAccessor* self) {
    DLOperator_Delete(self);
}

// ============================================================================
// 21. VMA 0x26de9b0 (23 B) — GXMtdParameterAccessor_CopyAssign
// ============================================================================
GXMtdParameterAccessor* GXMtdParameterAccessor_CopyAssign(
    GXMtdParameterAccessor* self,
    const GXMtdParameterAccessor* rhs)
{
    if (self != rhs) {
        self->paramNode       = rhs->paramNode;
        self->cachedTypeIndex = rhs->cachedTypeIndex;
    }
    return self;
}

// ============================================================================
// 22. VMA 0x26de990 (29 B) — GXMtdParameterAccessor_CopyCtor
// ============================================================================
void GXMtdParameterAccessor_CopyCtor(
    GXMtdParameterAccessor* self,
    const GXMtdParameterAccessor* rhs)
{
    self->vptr            = &g_vtableMtdParameterAccessorConst[2];
    self->paramNode       = rhs->paramNode;
    self->cachedTypeIndex = rhs->cachedTypeIndex;
}

// ============================================================================
// 23. VMA 0x26de970 (30 B) — GXMtdParameterAccessor_CtorConst
// ============================================================================
void GXMtdParameterAccessor_CtorConst(GXMtdParameterAccessor* self) {
    self->vptr            = &g_vtableMtdParameterAccessorConst[2];
    self->paramNode       = nullptr;
    self->cachedTypeIndex = -1;
}

// ============================================================================
// 24. VMA 0x26de950 (28 B) — GXMtdTextureSlotAccessor_DeletingDtor
// ============================================================================
void GXMtdTextureSlotAccessor_DeletingDtor(GXMtdTextureSlotAccessor* self) {
    GXMtdTextureSlotAccessor_Dtor(self);
    DLOperator_Delete(self);
}

// ============================================================================
// 25. VMA 0x26de940 (16 B) — GXMtdVector_ThrowLengthError_ParamSync
// ============================================================================
void GXMtdVector_ThrowLengthError_ParamSync(void* vec) {
    if (vec != nullptr) {
        DLStd_XlengthError(reinterpret_cast<const char*>(GX_VMA(0x48715c5ULL)));
    } else {
        DLStd_XlengthError(reinterpret_cast<const char*>(GX_VMA(0x48715c5ULL)));
    }
}

// ============================================================================
// 26. VMA 0x26de840 (251 B) — GXMtdNodePtrVector_Reallocate
//     Allocates a new 8-byte aligned array of GXMtdParameterNode* pointers,
//     copies existing elements, frees the old array, and updates pointers.
// ============================================================================
void GXMtdNodePtrVector_Reallocate(
    GXMtdNodePtrVector* vec,
    uint64_t newCapacity)
{
    DLAllocator* alloc = vec->allocator;
    GXMtdParameterNode** newBuf = static_cast<GXMtdParameterNode**>(
        alloc->vptr->allocateAligned(alloc, newCapacity * sizeof(void*), 8));

    if (newBuf == nullptr) {
        if (g_tlsTransactionSuppressLog != 0) {
            DLTlsAllocator_RecordOutOfMemory(g_tlsTransactionErrorSlot, 1);
            return;
        }
        DLDebug_LogContainerHeapError(0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
        return;
    }

    GXMtdParameterNode** oldBegin = vec->begin;
    GXMtdParameterNode** oldEnd   = vec->end;
    if (oldBegin != oldEnd) {
        GXMtdParameterNode** dst = newBuf;
        for (GXMtdParameterNode** src = oldBegin; src != oldEnd; ++src, ++dst) {
            if (dst != nullptr) {
                *dst = *src;
            }
        }
        oldBegin = vec->begin;
        oldEnd   = vec->end;
    }

    int64_t count = oldEnd - oldBegin;
    if (oldBegin != nullptr) {
        vec->allocator->vptr->deallocate(vec->allocator, oldBegin);
    }

    vec->capacityEnd = newBuf + newCapacity;
    vec->end         = newBuf + count;
    vec->begin       = newBuf;
}

// ============================================================================
// 27. VMA 0x26de830 (16 B) — GXMtdVector_ThrowLengthError_NodePtr
// ============================================================================
void GXMtdVector_ThrowLengthError_NodePtr(void* vec) {
    if (vec != nullptr) {
        DLStd_XlengthError(reinterpret_cast<const char*>(GX_VMA(0x48715c5ULL)));
    } else {
        DLStd_XlengthError(reinterpret_cast<const char*>(GX_VMA(0x48715c5ULL)));
    }
}

// ============================================================================
// 28. VMA 0x26de780 (29 B) — GXMtdRootBlock_HasValidMagicHeader
//     Checks if rootBlock + 0x2C..+0x2F equals 'M', 'T', 'D', ' '.
// ============================================================================
bool GXMtdRootBlock_HasValidMagicHeader(const GXMtdRootBlock* rootBlock) {
    return rootBlock->magic[0] == 'M' &&
           rootBlock->magic[1] == 'T' &&
           rootBlock->magic[2] == 'D' &&
           rootBlock->magic[3] == ' ';
}

// ============================================================================
// 29. VMA 0x26de770 (4 B) — GXMtdMaterialContainer_GetLastError
// ============================================================================
uint32_t GXMtdMaterialContainer_GetLastError(const GXMtdMaterialContainer* self) {
    return self->lastError;
}

// ============================================================================
// 30. VMA 0x26de470 (762 B) — GXMtdMaterialContainer_SyncTextureSlotsFromDescriptorList
//     Synchronizes the MTD container's texture sampler slots (+0xD0 array)
//     against an input vector of 0x40-byte GXMtdTextureSlotDescriptor entries,
//     setting *outModified = true if slot count, uvIndex, shaderDataIndex, or
//     samplerTypeName changed.
// ============================================================================
bool GXMtdMaterialContainer_SyncTextureSlotsFromDescriptorList(
    GXMtdMaterialContainer* self,
    const GXMtdTextureSlotDescriptorVector* descList,
    bool* outModified)
{
    GXMtdRootBlock* root = self->rootBlock;
    if (root == nullptr) {
        *outModified = false;
        self->lastError = 3;
        return false;
    }

    GXMtdBlockArray* listsArr = &root->dataBlock->listsBlockArray;
    if (GXMtdBlockArray_GetCount(listsArr) == 0) {
        *outModified = false;
        self->lastError = 3;
        return false;
    }

    GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
    *outModified = false;
    if (listsBlock == nullptr) {
        self->lastError = 3;
        return false;
    }

    uint64_t newSlotCount = static_cast<uint64_t>(
        reinterpret_cast<const uint8_t*>(descList->end) -
        reinterpret_cast<const uint8_t*>(descList->begin)) >> 6; // / 0x40

    GXMtdBlockArray* texSlotsArr = &listsBlock->textureSlots;
    if (static_cast<uint32_t>(newSlotCount) != static_cast<uint32_t>(GXMtdBlockArray_GetCount(texSlotsArr))) {
        GXMtdBlockArray_Resize(texSlotsArr, static_cast<int32_t>(newSlotCount));
        for (uint32_t i = 0; i < static_cast<uint32_t>(newSlotCount); ++i) {
            GXMtdTextureSlotNode* newSlot = static_cast<GXMtdTextureSlotNode*>(g_pfnMtdBlockAllocate(0xD0));
            if (newSlot == nullptr) {
                break;
            }
            GXMtdTextureSlotNode_Ctor(newSlot);
            newSlot->vptr->initializeDefaults(newSlot);
            GXMtdBlockArray_SetAt(texSlotsArr, static_cast<int32_t>(i), newSlot);
        }
        *outModified = true;
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(newSlotCount); ++i) {
        GXMtdTextureSlotNode* slotNode = static_cast<GXMtdTextureSlotNode*>(
            GXMtdBlockArray_GetAt(texSlotsArr, static_cast<int32_t>(i)));
        if (slotNode == nullptr) {
            continue;
        }

        const GXMtdTextureSlotDescriptor& desc = descList->begin[i];
        bool uvDiff  = (slotNode->uvIndex != desc.uvIndex);
        bool idxDiff = (slotNode->shaderDataIdx != desc.shaderDataIndex);

        GXMtdBlockField* tf = &slotNode->typeNameField;
        UnshareAndCommitBlockField(tf);

        const char* curTypeName = reinterpret_cast<const char*>(GX_VMA(0x48715c4ULL));
        if (tf->dataPtr != nullptr) {
            curTypeName = reinterpret_cast<const char*>(tf->dataPtr);
        }

        uint32_t changedMask = (uvDiff || idxDiff) ? 1u : 0u;
        char fillCh = g_defaultLocaleFillChar;
        DLStringA convertedName{};
        GXMtdStringConvert_WideToMultiByteString(&convertedName, &desc.samplerTypeName, &fillCh);

        const char* newNameCStr = convertedName.c_str();
        if (DLCharTraitsA_Compare(curTypeName, newNameCStr) != 0) {
            changedMask |= 1u;
        }

        if (convertedName.capacity >= 16) {
            convertedName.allocator->vptr->deallocate(convertedName.allocator, convertedName.buf.ptr);
        }

        if (changedMask != 0) {
            slotNode->uvIndex       = desc.uvIndex;
            slotNode->shaderDataIdx = desc.shaderDataIndex;
            GXMtdBlockField_SetStringWide(tf, &desc.samplerTypeName);
            *outModified = true;
        }
    }

    return true;
}

// ============================================================================
// 31. VMA 0x26de350 (277 B) — GXMtdNodePtrVector_ResizeZeroed
//     Resizes a GXMtdNodePtrVector to newCount, zero-filling new slots.
// ============================================================================
void GXMtdNodePtrVector_ResizeZeroed(
    GXMtdNodePtrVector* vec,
    uint64_t newCount)
{
    uint64_t curCount = static_cast<uint64_t>(vec->end - vec->begin);
    if (curCount > newCount) {
        if (newCount == 0) {
            vec->end = vec->begin;
        } else if (vec->begin + newCount != vec->end) {
            vec->end = vec->begin + newCount;
        }
        return;
    }

    if (curCount < newCount) {
        uint64_t extraNeeded = newCount - curCount;
        uint64_t availCap    = static_cast<uint64_t>(vec->capacityEnd - vec->end);
        if (availCap < extraNeeded) {
            constexpr uint64_t kMaxPtrCount = 0x1FFFFFFFFFFFFFFFULL;
            if (kMaxPtrCount - curCount < extraNeeded) {
                GXMtdVector_ThrowLengthError_NodePtr(vec);
                return;
            }
            uint64_t oldCap = static_cast<uint64_t>(vec->capacityEnd - vec->begin);
            uint64_t halfCap = oldCap >> 1;
            uint64_t grownCap = (kMaxPtrCount - halfCap >= oldCap) ? (oldCap + halfCap) : 0;
            if (grownCap < newCount) {
                grownCap = newCount;
            }
            GXMtdNodePtrVector_Reallocate(vec, grownCap);
        }

        uint64_t countAfterRealloc = static_cast<uint64_t>(vec->end - vec->begin);
        for (uint64_t i = countAfterRealloc; i < newCount; ++i) {
            GXMtdParameterNode** slot = vec->begin + i;
            if (slot != nullptr) {
                *slot = nullptr;
            }
        }
        vec->end = vec->begin + newCount;
    }
}

// ============================================================================
// 32. VMA 0x26de160 (496 B) — GXMtdParamSyncPairVector_ResizeDefault
//     Resizes a GXMtdParamSyncPairVector to newCount, initializing new 16-byte
//     entries to { syncState = 3, existingNode = nullptr }.
// ============================================================================
void GXMtdParamSyncPairVector_ResizeDefault(
    GXMtdParamSyncPairVector* vec,
    uint64_t newCount)
{
    uint64_t curCount = static_cast<uint64_t>(vec->end - vec->begin);
    if (curCount > newCount) {
        if (newCount == 0) {
            vec->end = vec->begin;
        } else if (vec->begin + newCount != vec->end) {
            vec->end = vec->begin + newCount;
        }
        return;
    }

    if (curCount < newCount) {
        uint64_t extraNeeded = newCount - curCount;
        uint64_t availCap    = static_cast<uint64_t>(vec->capacityEnd - vec->end);
        if (availCap < extraNeeded) {
            constexpr uint64_t kMaxPairCount = 0x0FFFFFFFFFFFFFFFULL;
            if (kMaxPairCount - curCount < extraNeeded) {
                GXMtdVector_ThrowLengthError_ParamSync(vec);
                return;
            }
            uint64_t oldCap   = static_cast<uint64_t>(vec->capacityEnd - vec->begin);
            uint64_t halfCap  = oldCap >> 1;
            uint64_t grownCap = (kMaxPairCount - halfCap >= oldCap) ? (oldCap + halfCap) : 0;
            if (grownCap < newCount) {
                grownCap = newCount;
            }

            DLAllocator* alloc = vec->allocator;
            GXMtdParamSyncPair* newBuf = static_cast<GXMtdParamSyncPair*>(
                alloc->vptr->allocateAligned(alloc, grownCap * sizeof(GXMtdParamSyncPair), 8));
            if (newBuf == nullptr) {
                if (g_tlsTransactionSuppressLog != 0) {
                    DLTlsAllocator_RecordOutOfMemory(g_tlsTransactionErrorSlot, 1);
                    return;
                }
                DLDebug_LogContainerHeapError(0, 0x109, reinterpret_cast<const char*>(GX_VMA(0x4737d80ULL)));
                return;
            }

            GXMtdParamSyncPair* oldBegin = vec->begin;
            GXMtdParamSyncPair* oldEnd   = vec->end;
            if (oldBegin != oldEnd) {
                GXMtdParamSyncPair* dst = newBuf;
                for (GXMtdParamSyncPair* src = oldBegin; src != oldEnd; ++src, ++dst) {
                    if (dst != nullptr) {
                        *dst = *src;
                    }
                }
                oldBegin = vec->begin;
                oldEnd   = vec->end;
            }

            int64_t count = oldEnd - oldBegin;
            if (oldBegin != nullptr) {
                vec->allocator->vptr->deallocate(vec->allocator, oldBegin);
            }
            vec->capacityEnd = newBuf + grownCap;
            vec->end         = newBuf + count;
            vec->begin       = newBuf;
        }

        uint64_t countAfterRealloc = static_cast<uint64_t>(vec->end - vec->begin);
        for (uint64_t i = countAfterRealloc; i < newCount; ++i) {
            GXMtdParamSyncPair* slot = vec->begin + i;
            if (slot != nullptr) {
                slot->syncState    = 3;
                slot->existingNode = nullptr;
            }
        }
        vec->end = vec->begin + newCount;
    }
}

// ============================================================================
// 33. VMA 0x26dd9f0 (1,900 B) — GXMtdMaterialContainer_SyncParametersFromDescriptorList
//     Synchronizes the MTD container's shader parameters (+0xA0 array) against
//     an input vector of 0x50-byte GXMtdParameterDescriptor entries:
//       - Matches existing parameters by name (0x26dd500)
//       - Classifies each parameter as state 0 (new), 1 (type changed, convert
//         existing value via 0x26dbed0), or 2 (exact type match, clone via 0x26edf60)
//       - Rebuilds the parameter list in exact descriptor order.
// ============================================================================
bool GXMtdMaterialContainer_SyncParametersFromDescriptorList(
    GXMtdMaterialContainer* self,
    const GXMtdParameterDescriptorVector* descList,
    bool* outModified)
{
    GXMtdRootBlock* root = self->rootBlock;
    if (root == nullptr) {
        *outModified = false;
        self->lastError = 3;
        return false;
    }

    GXMtdBlockArray* listsArr = &root->dataBlock->listsBlockArray;
    if (GXMtdBlockArray_GetCount(listsArr) == 0) {
        *outModified = false;
        self->lastError = 3;
        return false;
    }

    GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
    *outModified = false;
    if (listsBlock == nullptr) {
        self->lastError = 3;
        return false;
    }

    DLAllocator* heapAlloc = GXMtdContainerHeapHolder_GetAllocator(GXMtdDefaultContainerHeap_GetHolder());
    uint32_t caps[8]{};
    heapAlloc->vptr->getCapabilities(caps, heapAlloc, 0);
    if ((caps[0] & 0x20u) == 0) {
        DLDebug_LogContainerHeapError(0, 0x3E, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    GXMtdParamSyncPairVector syncPairs{};
    syncPairs.allocator = heapAlloc;

    uint64_t newParamCount = static_cast<uint64_t>(
        reinterpret_cast<const uint8_t*>(descList->end) -
        reinterpret_cast<const uint8_t*>(descList->begin)) / sizeof(GXMtdParameterDescriptor);

    GXMtdParamSyncPairVector_ResizeDefault(&syncPairs, static_cast<uint32_t>(newParamCount));

    GXMtdParameterAccessor tempAcc{};
    GXMtdParameterAccessor_CtorMutable(&tempAcc);

    GXMtdBlockArray* paramNodesArr = &listsBlock->paramNodes;
    bool anyDiff = (static_cast<uint32_t>(newParamCount) != static_cast<uint32_t>(GXMtdBlockArray_GetCount(paramNodesArr)));

    for (uint32_t i = 0; i < static_cast<uint32_t>(newParamCount); ++i) {
        const GXMtdParameterDescriptor& desc = descList->begin[i];
        char fillCh = g_defaultLocaleFillChar;
        DLStringA asciiName{};
        GXMtdStringConvert_WideToMultiByteString(&asciiName, &desc.paramName, &fillCh);
        int32_t foundIdx = GXMtdMaterialContainer_FindParameterIndexByNameAscii(self, &asciiName);
        if (asciiName.capacity >= 16) {
            asciiName.allocator->vptr->deallocate(asciiName.allocator, asciiName.buf.ptr);
        }

        if (foundIdx == -1) {
            syncPairs.begin[i].syncState = 0;
            anyDiff = true;
            continue;
        }

        GXMtdParameterAccessor foundAcc{};
        GXMtdParameterAccessor_CtorMutable(&foundAcc);
        GXMtdParameterNode* foundNode = nullptr;
        if (self->rootBlock != nullptr) {
            GXMtdBlockArray* lbArr = &self->rootBlock->dataBlock->listsBlockArray;
            if (GXMtdBlockArray_GetCount(lbArr) != 0) {
                GXMtdListsBlock* lb = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(lbArr, 0));
                if (lb != nullptr && static_cast<uint32_t>(foundIdx) < static_cast<uint32_t>(GXMtdBlockArray_GetCount(&lb->paramNodes))) {
                    foundNode = static_cast<GXMtdParameterNode*>(GXMtdBlockArray_GetAt(&lb->paramNodes, foundIdx));
                }
            }
        }
        GXMtdParameterAccessor_BindNode(&foundAcc, foundNode);
        GXMtdParameterAccessor_CopyAssign(&tempAcc, &foundAcc);
        GXMtdParameterAccessor_Dtor(&foundAcc);

        syncPairs.begin[i].existingNode = static_cast<GXMtdParameterNode*>(
            GXMtdBlockArray_GetAt(paramNodesArr, foundIdx));

        int32_t existingType = GXMtdParameterAccessor_GetResolvedType(&tempAcc);
        if (static_cast<uint32_t>(existingType) == desc.paramType) {
            syncPairs.begin[i].syncState = 2;
        } else {
            syncPairs.begin[i].syncState = 1;
            anyDiff = true;
        }

        if (static_cast<int32_t>(i) != foundIdx) {
            anyDiff = true;
        }
    }

    bool resultOk = true;
    if (anyDiff) {
        DLAllocator* ptrVecAlloc = GXMtdContainerHeapHolder_GetAllocator(GXMtdDefaultContainerHeap_GetHolder());
        uint32_t ptrCaps[8]{};
        ptrVecAlloc->vptr->getCapabilities(ptrCaps, ptrVecAlloc, 0);
        if ((ptrCaps[0] & 0x20u) == 0) {
            DLDebug_LogContainerHeapError(0, 0x3E, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
        }

        GXMtdNodePtrVector clonedNodes{};
        clonedNodes.allocator = ptrVecAlloc;
        GXMtdNodePtrVector_ResizeZeroed(&clonedNodes, static_cast<uint32_t>(newParamCount));

        for (uint32_t i = 0; i < static_cast<uint32_t>(newParamCount); ++i) {
            uint32_t st = syncPairs.begin[i].syncState;
            if ((st - 1u) <= 1u && syncPairs.begin[i].existingNode != nullptr) {
                clonedNodes.begin[i] = static_cast<GXMtdParameterNode*>(
                    GXMtdBlockObject_Clone(syncPairs.begin[i].existingNode));
            } else {
                clonedNodes.begin[i] = nullptr;
            }
        }

        GXMtdBlockArray_Resize(paramNodesArr, static_cast<int32_t>(newParamCount));
        for (GXMtdParamSyncPair* p = syncPairs.begin; p != syncPairs.end; ++p) {
            p->existingNode = nullptr;
        }

        bool allocFailed = false;
        for (uint32_t i = 0; i < static_cast<uint32_t>(newParamCount); ++i) {
            GXMtdParameterNode* finalNode = clonedNodes.begin[i];
            uint32_t st = syncPairs.begin[i].syncState;

            if (st != 2u) {
                GXMtdParameterNode* newNode = static_cast<GXMtdParameterNode*>(g_pfnMtdBlockAllocate(0x138));
                if (newNode == nullptr) {
                    allocFailed = true;
                } else {
                    GXMtdParameterNode_Ctor(newNode);
                    newNode->vptr->initializeDefaults(newNode);
                    GXMtdParameterAccessor_BindNode(&tempAcc, newNode);

                    const GXMtdParameterDescriptor& desc = descList->begin[i];
                    GXMtdParameterAccessor_SetNameWide(&tempAcc, &desc.paramName);
                    GXMtdParameterAccessor_SetTypeAndAllocateValueBlock(&tempAcc, desc.paramType);
                    GXMtdParameterAccessor_SetValueFromBuffer4(&tempAcc, desc.valueData);
                    finalNode = newNode;
                }

                if (st == 1u && clonedNodes.begin[i] != nullptr) {
                    GXMtdParameterNode* oldCloned = clonedNodes.begin[i];
                    GXMtdParameterAccessor oldAcc{};
                    GXMtdParameterAccessor_CtorMutable(&oldAcc);
                    GXMtdParameterAccessor_BindNode(&oldAcc, oldCloned);

                    uint32_t oldValues[4] = {};
                    if (GXMtdParameterAccessor_GetValueToBuffer4(&oldAcc, oldValues)) {
                        int32_t oldType = GXMtdParameterAccessor_GetResolvedType(&oldAcc);
                        int32_t newType = GXMtdParameterAccessor_GetResolvedType(&tempAcc);
                        GXMtdConvertedValue16 converted = GXMtdParamValue_ConvertBetweenTypes(
                            self,
                            oldValues,
                            static_cast<uint32_t>(oldType),
                            descList->begin[i].valueData,
                            static_cast<uint32_t>(newType));
                        GXMtdParameterAccessor_SetValueFromBuffer4(&tempAcc, &converted);
                    }

                    oldCloned->vptr->dtor(oldCloned);
                    g_pfnMtdBlockDeallocate(oldCloned);
                    clonedNodes.begin[i] = nullptr;
                    GXMtdParameterAccessor_Dtor(&oldAcc);
                }
            }

            GXMtdBlockArray_SetAt(paramNodesArr, static_cast<int32_t>(i), finalNode);
        }

        if (allocFailed) {
            self->lastError = 2;
            resultOk = false;
        } else {
            *outModified = true;
            resultOk = true;
        }

        if (clonedNodes.begin != nullptr) {
            clonedNodes.allocator->vptr->deallocate(clonedNodes.allocator, clonedNodes.begin);
            clonedNodes.capacityEnd = nullptr;
            clonedNodes.end         = nullptr;
            clonedNodes.begin       = nullptr;
        }
    }

    GXMtdParameterAccessor_Dtor(&tempAcc);
    if (syncPairs.begin != nullptr) {
        syncPairs.allocator->vptr->deallocate(syncPairs.allocator, syncPairs.begin);
        syncPairs.capacityEnd = nullptr;
        syncPairs.end         = nullptr;
        syncPairs.begin       = nullptr;
    }

    return resultOk;
}

// ============================================================================
// 34. VMA 0x26dd9e0 (5 B) — GXMtdTextureSlotAccessor_DtorThunk
// ============================================================================
void GXMtdTextureSlotAccessor_DtorThunk(GXMtdTextureSlotAccessor* self) {
    GXMtdTextureSlotAccessor_Dtor(self);
}

// ============================================================================
// 35. VMA 0x26dd930 (166 B) — GXMtdMaterialContainer_GetMutableTextureSlotAccessor
// ============================================================================
GXMtdTextureSlotAccessor* GXMtdMaterialContainer_GetMutableTextureSlotAccessor(
    GXMtdTextureSlotAccessor* outAccessor,
    GXMtdMaterialContainer* self,
    uint32_t slotIndex)
{
    GXMtdTextureSlotAccessor_CtorConst(outAccessor);
    outAccessor->vptr = &g_vtableMtdTextureSlotAccessorMutable[2];

    GXMtdTextureSlotNode* slotNode = nullptr;
    if (self->rootBlock != nullptr) {
        GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
        if (GXMtdBlockArray_GetCount(listsArr) != 0) {
            GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
            if (listsBlock != nullptr &&
                slotIndex < static_cast<uint32_t>(GXMtdBlockArray_GetCount(&listsBlock->textureSlots))) {
                slotNode = static_cast<GXMtdTextureSlotNode*>(
                    GXMtdBlockArray_GetAt(&listsBlock->textureSlots, static_cast<int32_t>(slotIndex)));
            }
        }
    }
    GXMtdTextureSlotAccessor_BindNode(outAccessor, slotNode);
    return outAccessor;
}

// ============================================================================
// 36. VMA 0x26dd890 (152 B) — GXMtdMaterialContainer_GetConstTextureSlotAccessor
// ============================================================================
GXMtdTextureSlotAccessor* GXMtdMaterialContainer_GetConstTextureSlotAccessor(
    GXMtdTextureSlotAccessor* outAccessor,
    const GXMtdMaterialContainer* self,
    uint32_t slotIndex)
{
    GXMtdTextureSlotAccessor_CtorConst(outAccessor);

    GXMtdTextureSlotNode* slotNode = nullptr;
    if (self->rootBlock != nullptr) {
        GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
        if (GXMtdBlockArray_GetCount(listsArr) != 0) {
            GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
            if (listsBlock != nullptr &&
                slotIndex < static_cast<uint32_t>(GXMtdBlockArray_GetCount(&listsBlock->textureSlots))) {
                slotNode = static_cast<GXMtdTextureSlotNode*>(
                    GXMtdBlockArray_GetAt(&listsBlock->textureSlots, static_cast<int32_t>(slotIndex)));
            }
        }
    }
    GXMtdTextureSlotAccessor_BindNode(outAccessor, slotNode);
    return outAccessor;
}

// ============================================================================
// 37. VMA 0x26dd7f0 (152 B) — GXMtdMaterialContainer_GetMutableParameterAccessor
// ============================================================================
GXMtdParameterAccessor* GXMtdMaterialContainer_GetMutableParameterAccessor(
    GXMtdParameterAccessor* outAccessor,
    GXMtdMaterialContainer* self,
    uint32_t paramIndex)
{
    GXMtdParameterAccessor_CtorMutable(outAccessor);

    GXMtdParameterNode* paramNode = nullptr;
    if (self->rootBlock != nullptr) {
        GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
        if (GXMtdBlockArray_GetCount(listsArr) != 0) {
            GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
            if (listsBlock != nullptr &&
                paramIndex < static_cast<uint32_t>(GXMtdBlockArray_GetCount(&listsBlock->paramNodes))) {
                paramNode = static_cast<GXMtdParameterNode*>(
                    GXMtdBlockArray_GetAt(&listsBlock->paramNodes, static_cast<int32_t>(paramIndex)));
            }
        }
    }
    GXMtdParameterAccessor_BindNode(outAccessor, paramNode);
    return outAccessor;
}

// ============================================================================
// 38. VMA 0x26dd750 (152 B) — GXMtdMaterialContainer_GetConstParameterAccessor
// ============================================================================
GXMtdParameterAccessor* GXMtdMaterialContainer_GetConstParameterAccessor(
    GXMtdParameterAccessor* outAccessor,
    const GXMtdMaterialContainer* self,
    uint32_t paramIndex)
{
    GXMtdParameterAccessor_CtorConst(outAccessor);

    GXMtdParameterNode* paramNode = nullptr;
    if (self->rootBlock != nullptr) {
        GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
        if (GXMtdBlockArray_GetCount(listsArr) != 0) {
            GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
            if (listsBlock != nullptr &&
                paramIndex < static_cast<uint32_t>(GXMtdBlockArray_GetCount(&listsBlock->paramNodes))) {
                paramNode = static_cast<GXMtdParameterNode*>(
                    GXMtdBlockArray_GetAt(&listsBlock->paramNodes, static_cast<int32_t>(paramIndex)));
            }
        }
    }
    GXMtdParameterAccessor_BindNode(outAccessor, paramNode);
    return outAccessor;
}

// ============================================================================
// 39. VMA 0x26dd500 (587 B) — GXMtdMaterialContainer_FindParameterIndexByNameAscii
//     Searches all parameters in the MTD container for an exact ASCII name
//     match, returning the 0-based parameter index or -1 if not found.
// ============================================================================
int32_t GXMtdMaterialContainer_FindParameterIndexByNameAscii(
    const GXMtdMaterialContainer* self,
    const DLStringA* paramName)
{
    if (self == nullptr || paramName == nullptr || self->rootBlock == nullptr) {
        return -1;
    }

    GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
    if (GXMtdBlockArray_GetCount(listsArr) == 0) {
        return -1;
    }

    GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
    if (listsBlock == nullptr) {
        return -1;
    }

    uint32_t paramCount = static_cast<uint32_t>(GXMtdBlockArray_GetCount(&listsBlock->paramNodes));
    const char* targetName = paramName->c_str();

    for (uint32_t i = 0; i < paramCount; ++i) {
        GXMtdParameterNode* node = static_cast<GXMtdParameterNode*>(
            GXMtdBlockArray_GetAt(&listsBlock->paramNodes, static_cast<int32_t>(i)));
        if (node != nullptr) {
            UnshareAndCommitBlockField(&node->nameField);
            const char* nameStr = reinterpret_cast<const char*>(node->nameField.dataPtr);
            if (nameStr != nullptr && DLCharTraitsA_Compare(nameStr, targetName) == 0) {
                return static_cast<int32_t>(i);
            }
        }
    }
    return -1;
}

// ============================================================================
// 40. VMA 0x26dd480 (120 B) — GXMtdMaterialContainer_FindParameterIndexByNameWide
// ============================================================================
int32_t GXMtdMaterialContainer_FindParameterIndexByNameWide(
    const GXMtdMaterialContainer* self,
    const DLStringW* wideParamName)
{
    char fillCh = g_defaultLocaleFillChar;
    DLStringA asciiName{};
    GXMtdStringConvert_WideToMultiByteString(&asciiName, wideParamName, &fillCh);
    int32_t idx = GXMtdMaterialContainer_FindParameterIndexByNameAscii(self, &asciiName);
    if (asciiName.capacity >= 16) {
        asciiName.allocator->vptr->deallocate(asciiName.allocator, asciiName.buf.ptr);
    }
    return idx;
}

// ============================================================================
// 41. VMA 0x26dd420 (81 B) — GXMtdMaterialContainer_GetTextureSlotCount
// ============================================================================
int32_t GXMtdMaterialContainer_GetTextureSlotCount(const GXMtdMaterialContainer* self) {
    if (self->rootBlock == nullptr) {
        return 0;
    }
    GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
    if (GXMtdBlockArray_GetCount(listsArr) == 0) {
        return 0;
    }
    GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
    if (listsBlock == nullptr) {
        return 0;
    }
    return GXMtdBlockArray_GetCount(&listsBlock->textureSlots);
}

// ============================================================================
// 42. VMA 0x26dd2f0 (297 B) — GXMtdMaterialContainer_AllocateTextureSlots
//     Ensures the GXMtdListsBlock exists at dataBlock->listsBlockArray[0] and
//     allocates slotCount GXMtdTextureSlotNode (0xD0 B) objects.
// ============================================================================
bool GXMtdMaterialContainer_AllocateTextureSlots(
    GXMtdMaterialContainer* self,
    uint32_t slotCount)
{
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return false;
    }

    GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
    if (GXMtdBlockArray_GetCount(listsArr) == 0) {
        GXMtdBlockArray_Resize(listsArr, 1);
        GXMtdListsBlock* newLists = static_cast<GXMtdListsBlock*>(g_pfnMtdBlockAllocate(0x130));
        if (newLists == nullptr) {
            self->lastError = 2;
            return false;
        }
        GXMtdListsBlock_Ctor(newLists);
        newLists->vptr->initializeDefaults(newLists);
        GXMtdBlockArray_SetAt(listsArr, 0, newLists);
    }

    GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
    if (listsBlock == nullptr) {
        self->lastError = 3;
        return false;
    }

    GXMtdBlockArray* texSlotsArr = &listsBlock->textureSlots;
    GXMtdBlockArray_Resize(texSlotsArr, static_cast<int32_t>(slotCount));
    for (uint32_t i = 0; i < slotCount; ++i) {
        GXMtdTextureSlotNode* newSlot = static_cast<GXMtdTextureSlotNode*>(g_pfnMtdBlockAllocate(0xD0));
        if (newSlot == nullptr) {
            self->lastError = 2;
            return false;
        }
        GXMtdTextureSlotNode_Ctor(newSlot);
        newSlot->vptr->initializeDefaults(newSlot);
        GXMtdBlockArray_SetAt(texSlotsArr, static_cast<int32_t>(i), newSlot);
    }
    return true;
}

// ============================================================================
// 43. VMA 0x26dd290 (81 B) — GXMtdMaterialContainer_GetParameterCount
// ============================================================================
int32_t GXMtdMaterialContainer_GetParameterCount(const GXMtdMaterialContainer* self) {
    if (self->rootBlock == nullptr) {
        return 0;
    }
    GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
    if (GXMtdBlockArray_GetCount(listsArr) == 0) {
        return 0;
    }
    GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
    if (listsBlock == nullptr) {
        return 0;
    }
    return GXMtdBlockArray_GetCount(&listsBlock->paramNodes);
}

// ============================================================================
// 44. VMA 0x26dd160 (297 B) — GXMtdMaterialContainer_AllocateParameters
//     Ensures GXMtdListsBlock exists and allocates paramCount GXMtdParameterNode
//     (0x138 B) objects.
// ============================================================================
bool GXMtdMaterialContainer_AllocateParameters(
    GXMtdMaterialContainer* self,
    uint32_t paramCount)
{
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return false;
    }

    GXMtdBlockArray* listsArr = &self->rootBlock->dataBlock->listsBlockArray;
    if (GXMtdBlockArray_GetCount(listsArr) == 0) {
        GXMtdBlockArray_Resize(listsArr, 1);
        GXMtdListsBlock* newLists = static_cast<GXMtdListsBlock*>(g_pfnMtdBlockAllocate(0x130));
        if (newLists == nullptr) {
            self->lastError = 2;
            return false;
        }
        GXMtdListsBlock_Ctor(newLists);
        newLists->vptr->initializeDefaults(newLists);
        GXMtdBlockArray_SetAt(listsArr, 0, newLists);
    }

    GXMtdListsBlock* listsBlock = static_cast<GXMtdListsBlock*>(GXMtdBlockArray_GetAt(listsArr, 0));
    if (listsBlock == nullptr) {
        self->lastError = 3;
        return false;
    }

    GXMtdBlockArray* paramArr = &listsBlock->paramNodes;
    GXMtdBlockArray_Resize(paramArr, static_cast<int32_t>(paramCount));
    for (uint32_t i = 0; i < paramCount; ++i) {
        GXMtdParameterNode* newParam = static_cast<GXMtdParameterNode*>(g_pfnMtdBlockAllocate(0x138));
        if (newParam == nullptr) {
            self->lastError = 2;
            return false;
        }
        GXMtdParameterNode_Ctor(newParam);
        newParam->vptr->initializeDefaults(newParam);
        GXMtdBlockArray_SetAt(paramArr, static_cast<int32_t>(i), newParam);
    }
    return true;
}

// ============================================================================
// 45. VMA 0x26dd0c0 (148 B) — GXMtdMaterialContainer_GetDescriptionAscii
// ============================================================================
bool GXMtdMaterialContainer_GetDescriptionAscii(
    GXMtdMaterialContainer* self,
    DLStringA* outDesc)
{
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return false;
    }
    GXMtdDataBlock* dataBlock = self->rootBlock->dataBlock;
    UnshareAndCommitBlockField(&dataBlock->descriptionField);
    const char* descStr = reinterpret_cast<const char*>(dataBlock->descriptionField.dataPtr);
    size_t len = (descStr[0] != '\0') ? DLCharTraitsA_Length(descStr) : 0;
    DLStringA_AssignBuffer(outDesc, descStr, len);
    return true;
}

// ============================================================================
// 46. VMA 0x26dcf60 (344 B) — GXMtdMaterialContainer_GetDescriptionWide
// ============================================================================
bool GXMtdMaterialContainer_GetDescriptionWide(
    GXMtdMaterialContainer* self,
    DLStringW* outWideDesc)
{
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return false;
    }

    GXMtdDataBlock* dataBlock = self->rootBlock->dataBlock;
    UnshareAndCommitBlockField(&dataBlock->descriptionField);
    const char* descStr = reinterpret_cast<const char*>(dataBlock->descriptionField.dataPtr);
    char fillCh = g_defaultLocaleFillChar;

    outWideDesc->length = 0;
    outWideDesc->c_str()[0] = u'\0';

    uint64_t oldLen = outWideDesc->length;
    uint64_t mbLen  = DLCharTraitsA_Length(descStr);
    if (mbLen != 0) {
        if (fillCh == g_sentinelLocaleFillChar) {
            fillCh = outWideDesc->fillChar;
        }
        uint64_t wideCount = DLCharset_MeasureMultiByteToWide(descStr, mbLen, &fillCh);
        DLStringW_ResizeWithChar(outWideDesc, oldLen, wideCount, u'\0');
        char16_t* wideBuf = outWideDesc->c_str() + oldLen;
        DLCharset_ConvertMultiByteToWide(&fillCh, wideBuf, wideCount, descStr, &mbLen);
    }
    return true;
}

// ============================================================================
// 47. VMA 0x26dcf30 (34 B) — GXMtdMaterialContainer_SetDescriptionAscii
// ============================================================================
bool GXMtdMaterialContainer_SetDescriptionAscii(
    GXMtdMaterialContainer* self,
    const DLStringA* asciiDesc)
{
    if (self->rootBlock == nullptr) {
        return false;
    }
    return GXMtdBlockField_SetStringAscii(&self->rootBlock->dataBlock->descriptionField, asciiDesc);
}

// ============================================================================
// 48. VMA 0x26dcea0 (134 B) — GXMtdMaterialContainer_SetDescriptionWide
// ============================================================================
bool GXMtdMaterialContainer_SetDescriptionWide(
    GXMtdMaterialContainer* self,
    const DLStringW* wideDesc)
{
    char fillCh = g_defaultLocaleFillChar;
    DLStringA asciiDesc{};
    GXMtdStringConvert_WideToMultiByteString(&asciiDesc, wideDesc, &fillCh);

    bool ok = false;
    if (self->rootBlock != nullptr) {
        ok = GXMtdBlockField_SetStringAscii(&self->rootBlock->dataBlock->descriptionField, &asciiDesc);
    }
    if (asciiDesc.capacity >= 16) {
        asciiDesc.allocator->vptr->deallocate(asciiDesc.allocator, asciiDesc.buf.ptr);
    }
    return ok;
}

// ============================================================================
// 49. VMA 0x26dce70 (40 B) — GXMtdMaterialContainer_SetShaderPathAscii
// ============================================================================
bool GXMtdMaterialContainer_SetShaderPathAscii(
    GXMtdMaterialContainer* self,
    const DLStringA* asciiShaderPath)
{
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return false;
    }
    GXMtdBlockField_SetStringAscii(&self->rootBlock->dataBlock->shaderPathField, asciiShaderPath);
    return true;
}

// ============================================================================
// 50. VMA 0x26dcde0 (138 B) — GXMtdMaterialContainer_SetShaderPathWide
// ============================================================================
bool GXMtdMaterialContainer_SetShaderPathWide(
    GXMtdMaterialContainer* self,
    const DLStringW* wideShaderPath)
{
    char fillCh = g_defaultLocaleFillChar;
    DLStringA asciiPath{};
    GXMtdStringConvert_WideToMultiByteString(&asciiPath, wideShaderPath, &fillCh);

    bool ok = false;
    if (self->rootBlock != nullptr) {
        GXMtdBlockField_SetStringAscii(&self->rootBlock->dataBlock->shaderPathField, &asciiPath);
        ok = true;
    } else {
        self->lastError = 3;
    }

    if (asciiPath.capacity >= 16) {
        asciiPath.allocator->vptr->deallocate(asciiPath.allocator, asciiPath.buf.ptr);
    }
    return ok;
}

// ============================================================================
// 51. VMA 0x26dcd50 (142 B) — GXMtdMaterialContainer_GetShaderPathAscii
// ============================================================================
bool GXMtdMaterialContainer_GetShaderPathAscii(
    GXMtdMaterialContainer* self,
    DLStringA* outShaderPath)
{
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return false;
    }
    GXMtdDataBlock* dataBlock = self->rootBlock->dataBlock;
    UnshareAndCommitBlockField(&dataBlock->shaderPathField);
    const char* pathStr = reinterpret_cast<const char*>(dataBlock->shaderPathField.dataPtr);
    size_t len = (pathStr[0] != '\0') ? DLCharTraitsA_Length(pathStr) : 0;
    DLStringA_AssignBuffer(outShaderPath, pathStr, len);
    return true;
}

// ============================================================================
// 52. VMA 0x26dcbf0 (338 B) — GXMtdMaterialContainer_GetShaderPathWide
// ============================================================================
bool GXMtdMaterialContainer_GetShaderPathWide(
    GXMtdMaterialContainer* self,
    DLStringW* outWideShaderPath)
{
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return false;
    }

    GXMtdDataBlock* dataBlock = self->rootBlock->dataBlock;
    UnshareAndCommitBlockField(&dataBlock->shaderPathField);
    const char* pathStr = reinterpret_cast<const char*>(dataBlock->shaderPathField.dataPtr);
    char fillCh = g_defaultLocaleFillChar;

    outWideShaderPath->length = 0;
    outWideShaderPath->c_str()[0] = u'\0';

    uint64_t oldLen = outWideShaderPath->length;
    uint64_t mbLen  = DLCharTraitsA_Length(pathStr);
    if (mbLen != 0) {
        if (fillCh == g_sentinelLocaleFillChar) {
            fillCh = outWideShaderPath->fillChar;
        }
        uint64_t wideCount = DLCharset_MeasureMultiByteToWide(pathStr, mbLen, &fillCh);
        DLStringW_ResizeWithChar(outWideShaderPath, oldLen, wideCount, u'\0');
        char16_t* wideBuf = outWideShaderPath->c_str() + oldLen;
        DLCharset_ConvertMultiByteToWide(&fillCh, wideBuf, wideCount, pathStr, &mbLen);
    }
    return true;
}

// ============================================================================
// 53. VMA 0x26dcbd0 (31 B) — GXMtdMaterialContainer_SetVersion
// ============================================================================
bool GXMtdMaterialContainer_SetVersion(GXMtdMaterialContainer* self, uint32_t version) {
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return false;
    }
    self->rootBlock->headerBlock->version = version;
    return true;
}

// ============================================================================
// 54. VMA 0x26dcbb0 (32 B) — GXMtdMaterialContainer_GetVersion
// ============================================================================
uint32_t GXMtdMaterialContainer_GetVersion(GXMtdMaterialContainer* self) {
    if (self->rootBlock == nullptr) {
        self->lastError = 3;
        return 0xFFFFFFFFu;
    }
    return self->rootBlock->headerBlock->version;
}

// ============================================================================
// 55. VMA 0x26dcba0 (10 B) — GXMtdMaterialContainer_SetUnsupportedErrorA
// ============================================================================
bool GXMtdMaterialContainer_SetUnsupportedErrorA(GXMtdMaterialContainer* self) {
    self->lastError = 1;
    return false;
}

// ============================================================================
// 56. VMA 0x26dcb90 (10 B) — GXMtdMaterialContainer_SetUnsupportedErrorB
// ============================================================================
bool GXMtdMaterialContainer_SetUnsupportedErrorB(GXMtdMaterialContainer* self) {
    self->lastError = 1;
    return false;
}

// ============================================================================
// 57. VMA 0x26dc990 (511 B) — GXMtdMaterialContainer_SaveToFileAscii
//     Writes the 'M','T','D',' ' 4-byte signature into rootBlock->headerBlock
//     and serializes the entire MTD container to a buffered file stream.
// ============================================================================
bool GXMtdMaterialContainer_SaveToFileAscii(
    GXMtdMaterialContainer* self,
    const DLStringA* filePath)
{
    if (self->rootBlock == nullptr) {
        return false;
    }

    const char kMtdMagic[4] = { 'M', 'T', 'D', ' ' };
    GXMtdHeaderBlock* hdr = self->rootBlock->headerBlock;
    UnshareAndCommitBlockField(&hdr->magicField);
    for (int i = 0; i < 4; ++i) {
        hdr->magicField.dataPtr[i] = static_cast<uint8_t>(kMtdMagic[i]);
    }

    alignas(16) uint8_t fileStream[0x60]{};
    DLBufferedFileStream_Ctor(fileStream, 0x2000);

    bool ok = false;
    if (!DLBufferedFileStream_OpenWrite(fileStream, filePath->c_str())) {
        self->lastError = 6;
    } else if (!GXMtdRootBlock_SerializeToStream(self->rootBlock, fileStream)) {
        self->lastError = 5;
        DLBufferedFileStream_Close(fileStream);
    } else {
        DLBufferedFileStream_Close(fileStream);
        ok = true;
    }

    DLBufferedFileStream_Dtor(fileStream);
    return ok;
}

// ============================================================================
// 58. VMA 0x26dc910 (120 B) — GXMtdMaterialContainer_SaveToFileWide
// ============================================================================
bool GXMtdMaterialContainer_SaveToFileWide(
    GXMtdMaterialContainer* self,
    const DLStringW* wideFilePath)
{
    char fillCh = g_defaultLocaleFillChar;
    DLStringA asciiPath{};
    GXMtdStringConvert_WideToMultiByteString(&asciiPath, wideFilePath, &fillCh);
    bool ok = GXMtdMaterialContainer_SaveToFileAscii(self, &asciiPath);
    if (asciiPath.capacity >= 16) {
        asciiPath.allocator->vptr->deallocate(asciiPath.allocator, asciiPath.buf.ptr);
    }
    return ok;
}

// ============================================================================
// 59. VMA 0x26dc7c0 (331 B) — GXMtdMaterialContainer_LoadFromMemoryBuffer
//     Reallocates the 0x108-byte GXMtdRootBlock and deserializes an MTD binary
//     directly from an in-memory buffer via DLMemoryReadStream.
// ============================================================================
bool GXMtdMaterialContainer_LoadFromMemoryBuffer(
    GXMtdMaterialContainer* self,
    const void* buffer,
    uint32_t byteSize)
{
    alignas(16) uint8_t memStream[0x30]{};
    DLMemoryReadStream_Ctor(memStream);

    if (self->rootBlock != nullptr) {
        self->rootBlock->vptr->dtor(self->rootBlock);
        g_pfnMtdBlockDeallocate(self->rootBlock);
        self->rootBlock = nullptr;
    }

    GXMtdRootBlock* newRoot = static_cast<GXMtdRootBlock*>(g_pfnMtdBlockAllocate(0x108));
    bool ok = false;

    if (newRoot == nullptr) {
        self->rootBlock = nullptr;
        self->lastError = 2;
    } else {
        GXMtdRootBlock_Ctor(newRoot);
        newRoot->vptr->initializeDefaults(newRoot);
        self->rootBlock = newRoot;

        if (!DLMemoryReadStream_OpenBuffer(memStream, buffer, byteSize)) {
            self->lastError = 7;
        } else if (!self->rootBlock->vptr->deserializeFromStream(self->rootBlock, memStream)) {
            self->lastError = 4;
        } else {
            DLMemoryReadStream_Close(memStream);
            ok = true;
        }
    }

    if (!ok) {
        if (DLMemoryReadStream_IsOpen(memStream)) {
            DLMemoryReadStream_Close(memStream);
        }
        if (self->rootBlock != nullptr) {
            self->rootBlock->vptr->dtor(self->rootBlock);
            g_pfnMtdBlockDeallocate(self->rootBlock);
            self->rootBlock = nullptr;
        }
    }

    DLMemoryReadStream_Dtor(memStream);
    return ok;
}

// ============================================================================
// 60. VMA 0x26dc650 (355 B) — GXMtdMaterialContainer_LoadFromFileAscii
//     Reallocates the 0x108-byte GXMtdRootBlock and deserializes an MTD binary
//     from disk via DLBufferedFileStream.
// ============================================================================
bool GXMtdMaterialContainer_LoadFromFileAscii(
    GXMtdMaterialContainer* self,
    const DLStringA* filePath)
{
    if (self->rootBlock != nullptr) {
        self->rootBlock->vptr->dtor(self->rootBlock);
        g_pfnMtdBlockDeallocate(self->rootBlock);
        self->rootBlock = nullptr;
    }

    GXMtdRootBlock* newRoot = static_cast<GXMtdRootBlock*>(g_pfnMtdBlockAllocate(0x108));
    if (newRoot != nullptr) {
        GXMtdRootBlock_Ctor(newRoot);
        newRoot->vptr->initializeDefaults(newRoot);
        self->rootBlock = newRoot;
    } else {
        self->rootBlock = nullptr;
        self->lastError = 2;
    }

    alignas(16) uint8_t fileStream[0x60]{};
    DLBufferedFileStream_Ctor(fileStream, 0x2000);

    bool ok = false;
    if (self->rootBlock == nullptr) {
        self->lastError = 2;
    } else if (!DLBufferedFileStream_OpenRead(fileStream, filePath->c_str())) {
        self->lastError = 6;
    } else if (!self->rootBlock->vptr->deserializeFromStream(self->rootBlock, fileStream)) {
        self->lastError = 4;
    } else {
        DLBufferedFileStream_Close(fileStream);
        ok = true;
    }

    if (!ok) {
        if (DLBufferedFileStream_IsOpen(fileStream)) {
            DLBufferedFileStream_Close(fileStream);
        }
        if (self->rootBlock != nullptr) {
            self->rootBlock->vptr->dtor(self->rootBlock);
            g_pfnMtdBlockDeallocate(self->rootBlock);
            self->rootBlock = nullptr;
        }
    }

    DLBufferedFileStream_Dtor(fileStream);
    return ok;
}

// ============================================================================
// 61. VMA 0x26dc500 (325 B) — GXMtdStringConvert_WideToMultiByteString
//     Constructs a DLStringA in *outAscii by converting srcWide from UTF-16
//     to the engine's multi-byte encoding.
// ============================================================================
DLStringA* GXMtdStringConvert_WideToMultiByteString(
    DLStringA* outAscii,
    const DLStringW* srcWide,
    const char* pFillChar)
{
    const char16_t* widePtr = srcWide->c_str();
    uint64_t wideLen = srcWide->length;
    char fillCh = *pFillChar;

    DLAllocator* alloc = g_pCachedDefaultStringAllocator;
    if (alloc == nullptr) {
        alloc = DLSystem_GetDefaultStringAllocator();
        g_pCachedDefaultStringAllocator = alloc;
    }

    uint32_t caps[8]{};
    alloc->vptr->getCapabilities(caps, alloc, 0);
    if ((caps[0] & 0x20u) == 0) {
        DLDebug_LogContainerHeapError(0, 0x3E, reinterpret_cast<const char*>(GX_VMA(0x4737dc0ULL)));
    }

    outAscii->allocator  = alloc;
    outAscii->capacity   = 15;
    outAscii->length     = 0;
    outAscii->buf.sso[0] = '\0';
    outAscii->fillChar   = fillCh;

    if (wideLen == ~0ULL) {
        wideLen = DLCharTraitsW_Length(widePtr);
    }
    if (wideLen != 0) {
        if (fillCh == g_sentinelLocaleFillChar) {
            fillCh = outAscii->fillChar;
        }
        uint64_t mbLen = DLCharset_MeasureWideToMultiByte(widePtr, wideLen, &fillCh);
        DLStringA_ResizeWithChar(outAscii, 0, mbLen, '\0');
        char* dstBuf = outAscii->c_str();
        uint64_t wideLenCopy = wideLen;
        DLCharset_ConvertWideToMultiByte(&fillCh, dstBuf, mbLen, widePtr, &wideLenCopy);
    }
    return outAscii;
}

// ============================================================================
// 62. VMA 0x26dc480 (120 B) — GXMtdMaterialContainer_LoadFromFileWide
// ============================================================================
bool GXMtdMaterialContainer_LoadFromFileWide(
    GXMtdMaterialContainer* self,
    const DLStringW* wideFilePath)
{
    char fillCh = g_defaultLocaleFillChar;
    DLStringA asciiPath{};
    GXMtdStringConvert_WideToMultiByteString(&asciiPath, wideFilePath, &fillCh);
    bool ok = GXMtdMaterialContainer_LoadFromFileAscii(self, &asciiPath);
    if (asciiPath.capacity >= 16) {
        asciiPath.allocator->vptr->deallocate(asciiPath.allocator, asciiPath.buf.ptr);
    }
    return ok;
}

// ============================================================================
// 63. VMA 0x26dc470 (4 B) — GXMtdMaterialContainer_SetLastError
// ============================================================================
void GXMtdMaterialContainer_SetLastError(GXMtdMaterialContainer* self, uint32_t errorCode) {
    self->lastError = errorCode;
}

// ============================================================================
// 64. VMA 0x26dc3f0 (113 B) — GXMtdMaterialContainer_ResetAndAllocateRoot
// ============================================================================
bool GXMtdMaterialContainer_ResetAndAllocateRoot(GXMtdMaterialContainer* self) {
    if (self->rootBlock != nullptr) {
        self->rootBlock->vptr->dtor(self->rootBlock);
        g_pfnMtdBlockDeallocate(self->rootBlock);
        self->rootBlock = nullptr;
    }

    GXMtdRootBlock* newRoot = static_cast<GXMtdRootBlock*>(g_pfnMtdBlockAllocate(0x108));
    if (newRoot != nullptr) {
        GXMtdRootBlock_Ctor(newRoot);
        newRoot->vptr->initializeDefaults(newRoot);
        self->rootBlock = newRoot;
        return true;
    }

    self->rootBlock = nullptr;
    self->lastError = 2;
    return false;
}

// ============================================================================
// 65. VMA 0x26dc380 (100 B) — GXMtdMaterialContainer_CopyAssign
//     Deep-copies the root block from rhs via GXMtdBlockObject_Clone (0x26edf60).
// ============================================================================
GXMtdMaterialContainer* GXMtdMaterialContainer_CopyAssign(
    GXMtdMaterialContainer* self,
    const GXMtdMaterialContainer* rhs)
{
    if (self != rhs) {
        if (self->rootBlock != nullptr) {
            self->rootBlock->vptr->dtor(self->rootBlock);
            g_pfnMtdBlockDeallocate(self->rootBlock);
            self->rootBlock = nullptr;
        }
        self->lastError = 0;
        if (rhs->rootBlock != nullptr) {
            self->rootBlock = static_cast<GXMtdRootBlock*>(GXMtdBlockObject_Clone(rhs->rootBlock));
            if (self->rootBlock == nullptr) {
                self->lastError = 2;
            }
        }
    }
    return self;
}

} // extern "C"

} // namespace GX
} // namespace FromSoftware
