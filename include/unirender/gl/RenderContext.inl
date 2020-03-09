#pragma once

#include <ejoy2d/opengl.h>

namespace ur
{
namespace gl
{

template <typename T>
void RenderContext::ReadPixelsImpl(const T* pixels, int channels, int x, int y, int w, int h, int type)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

//	glReadBuffer(GL_COLOR_ATTACHMENT0);
    GLenum format;
    switch (channels)
    {
    case 4:
        format = GL_RGBA;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 1:
        format = GL_RED;
        break;
    default:
        return;
    }
    glReadPixels(x, y, w, h, type, type, (GLvoid*)pixels);
}

template <typename T>
uint32_t RenderContext::CreateComputeBufferImpl(const std::vector<T>& buf, size_t index) const
{
    GLuint data_buf;
    glGenBuffers(1, &data_buf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, data_buf);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(buf), &buf.front(), GL_STREAM_COPY);
    return data_buf;
}

}
}