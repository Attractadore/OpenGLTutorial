#pragma once

#include "glad.h"

#include <vector>

template <typename T>
inline void storeVectorGLBuffer(GLuint buffer, const std::vector<T>& vec, GLbitfield storage_bits = 0) {
    glNamedBufferStorage(buffer, sizeof(T) * vec.size(), vec.data(), storage_bits);
}
