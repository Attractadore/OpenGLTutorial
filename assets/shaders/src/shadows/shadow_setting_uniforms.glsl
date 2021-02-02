#ifndef SHADOW_SETTING_UNIFORMS_GLSL
#define SHADOW_SETTING_UNIFORMS_GLSL

#include "indices.glsl"
#include "shadow_modes.glsl"

layout(location = B_SHADOW_CASCADE_DEPTH_ADJUST_LOCATION)
    uniform bool b_shadow_cascade_depth_adjust;
layout(location = B_SHADOW_TEXEL_MOVE_LOCATION)
    uniform bool b_shadow_texel_move;
layout(location = SHADOW_MODE_LOCATION)
    uniform int shadow_mode;

#endif  // SHADOW_SETTING_UNIFORMS_GLSL
