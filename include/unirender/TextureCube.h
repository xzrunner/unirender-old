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

    unsigned int GetTexID() const { return m_texid; }
    void SetTexID(unsigned int texid);

private:
    RenderContext* m_rc = nullptr;

    unsigned int m_texid = 0;

}; // TextureCube

}