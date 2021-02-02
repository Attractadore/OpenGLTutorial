#ifndef SHADOW_MODES_GLSL
#define SHADOW_MODES_GLSL

#ifdef GL_core_profile
#define ENUM_BEGIN(enum_name)  // enum_name
#define ENUM_MEMBER(member_name, val) const int member_name = val;
#define ENUM_END
#else
#define ENUM_BEGIN(enum_name) enum enum_name {
#define ENUM_MEMBER(member_name, val) member_name = val,
#define ENUM_END \
    }            \
    ;
#endif

ENUM_BEGIN(ShadowMode)
ENUM_MEMBER(SHADOW_MODE_STANDARD, 0)
ENUM_MEMBER(SHADOW_MODE_MSM, 1)
ENUM_END

#endif  // SHADOW_MODES_GLSL
