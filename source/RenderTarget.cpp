#include "unirender/RenderTarget.h"
#include "unirender/RenderContext.h"
#include "unirender/Texture.h"

namespace ur
{

RenderTarget::RenderTarget(RenderContext* rc, int width, int height, bool has_depth)
	: m_rc(rc)
{
	m_color_tex = new Texture(rc, width, height);

	if (has_depth) {
		m_depth_tex = new Texture(rc, width, height, TEXTURE_DEPTH, false);
	} else {
		m_depth_tex = nullptr;
	}

	m_id = m_rc->CreateRenderTarget(0);
}

RenderTarget::~RenderTarget()
{
	m_rc->ReleaseRenderTarget(m_id);
	delete m_color_tex;
	if (m_depth_tex) {
		delete m_depth_tex;
	}
}

void RenderTarget::Bind()
{
	m_rc->BindRenderTarget(m_id);
	if (m_depth_tex) {
		m_rc->BindRenderTargetTex(m_color_tex->ID(), m_depth_tex->ID());
	} else {
		m_rc->BindRenderTargetTex(m_color_tex->ID());
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
	return m_color_tex->ID();
}

void RenderTarget::Resize(int width, int height)
{
	if (m_color_tex->Width() == width && m_color_tex->Height() == height) {
		return;
	}

	delete m_color_tex;
	m_color_tex = new Texture(m_rc, width, height);

	if (m_depth_tex) {
		delete m_depth_tex;
		m_depth_tex = new Texture(m_rc, width, height, TEXTURE_DEPTH);
	}
}

}