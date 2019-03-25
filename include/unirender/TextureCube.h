#pragma once

#include <cu/uncopyable.h>

namespace ur
{

class RenderContext;

class TextureCube : private cu::Uncopyable
{
public:
    TextureCube(RenderContext* rc);
    ~TextureCube();

    void BuildFromEquirectangularTex(unsigned int tex);

    unsigned int TexID() const { return m_texid; }

private:
    RenderContext* m_rc = nullptr;

    unsigned int m_texid = 0;

}; // TextureCube

}