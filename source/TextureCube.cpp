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

void TextureCube::SetTexID(unsigned int texid)
{
    if (m_texid == texid) {
        return;
    }

    if (m_texid != 0) {
        m_rc->ReleaseTexture(m_texid);
    }
    m_texid = texid;
}

}