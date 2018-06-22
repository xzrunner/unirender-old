#include "unirender/Texture.h"
#include "unirender/RenderContext.h"
#include "unirender/Utility.h"

#include <stdint.h>
#include <string.h>

namespace ur
{

Texture::Texture(RenderContext* rc, int width, int height, int format, bool filter_linear)
	: m_rc(rc)
	, m_width(width)
	, m_height(height)
	, m_format(format)
	, m_id(0)
{
	Init(filter_linear);
}

Texture::~Texture()
{
	m_rc->ReleaseTexture(m_id);
}

void Texture::Bind() const
{
	m_rc->BindTexture(m_id, 0);
}

void Texture::Init(bool filter_linear)
{
	size_t sz = Utility::CalcTextureSize(m_format, m_width, m_height);

	uint8_t* pixels = new uint8_t[sz];
	memset(pixels, 0, sz);

	m_id = m_rc->CreateTexture(pixels, m_width, m_height, m_format, 0, filter_linear ? 1 : 0);

	delete[] pixels;
}

}