#include "unirender/RenderTarget.h"
#include "unirender/RenderContext.h"
#include "unirender/Texture.h"

namespace ur
{

RenderTarget::RenderTarget(RenderContext* rc, int width, int height, bool has_depth)
	: m_rc(rc)
{
	m_color_tex = std::make_unique<Texture>();
	m_color_tex->Upload(rc, width, height);

	if (has_depth) {
		m_depth_tex = std::make_unique<Texture>();
		m_depth_tex->Upload(rc, width, height, TEXTURE_DEPTH, nullptr, TEXTURE_REPEAT, TEXTURE_NEAREST);
	} else {
		m_depth_tex = nullptr;
	}

	m_id = m_rc->CreateRenderTarget(0);
}

RenderTarget::~RenderTarget()
{
	m_rc->ReleaseRenderTarget(m_id);
}

void RenderTarget::Bind()
{
	m_rc->BindRenderTarget(m_id);
    m_rc->BindRenderTargetTex(m_color_tex->TexID(), ur::ATTACHMENT_COLOR0, ur::TEXTURE2D);
    if (m_depth_tex) {
        m_rc->BindRenderTargetTex(m_depth_tex->TexID(), ur::ATTACHMENT_DEPTH, ur::TEXTURE2D);
    }
}

void RenderTarget::Unbind()
{
	m_rc->UnbindRenderTarget();
}

int RenderTarget::Width() const
{
	return m_color_tex->Width();
}

int RenderTarget::Height() const
{
	return m_color_tex->Height();
}

int RenderTarget::TexID() const
{
	return m_color_tex->TexID();
}

void RenderTarget::Resize(int width, int height)
{
	if (m_color_tex->Width() == width && m_color_tex->Height() == height) {
		return;
	}

	m_color_tex = std::make_unique<Texture>();
	m_color_tex->Upload(m_rc, width, height);
	if (m_depth_tex) {
		m_depth_tex = std::make_unique<Texture>();
		m_depth_tex->Upload(m_rc, width, height, TEXTURE_DEPTH, nullptr, TEXTURE_REPEAT, TEXTURE_NEAREST);
	}
}

}