#include "unirender/TextureCube.h"
#include "unirender/RenderContext.h"

namespace ur
{

TextureCube::TextureCube(RenderContext* rc)
    : m_rc(rc)
{
}

TextureCube::~TextureCube()
{
    if (m_texid != 0) {
        m_rc->ReleaseTexture(m_texid);
    }
}

}