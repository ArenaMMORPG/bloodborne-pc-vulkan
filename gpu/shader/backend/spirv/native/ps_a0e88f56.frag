// SPDX-License-Identifier: GPL-2.0-or-later
// Native Vulkan 1.3 GLSL replacement for ps 0x00000000a0e88f56
// (4096x4096 CSM shadow depth pass + VSM point-light clip-distance shadow pass, 93 draws/frame).
#version 460
#ifdef HAS_CLDIST
#extension GL_EXT_demote_to_helper_invocation : require
layout(location = 0) in vec4 cldist_attr0;
#endif

void main() {
#ifdef HAS_CLDIST
    if (any(lessThan(cldist_attr0, vec4(0.0)))) {
        demote;
    }
#endif
}
