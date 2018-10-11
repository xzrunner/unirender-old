#include "unirender/Texture.h"
#include "unirender/RenderContext.h"
#include "unirender/Utility.h"

#include <stdint.h>
#include <string.h>

namespace ur
{

Texture::Texture()
	: m_rc(nullptr)
	, m_width(0)
	, m_height(0)
	, m_format(TEXTURE_INVALID)
	, m_texid(0)
{
}

Texture::Texture(RenderContext* rc, int width, int height,
	             TEXTURE_FORMAT format, unsigned int texid)
	: m_rc(rc)
	, m_width(width)
	, m_height(height)
	, m_format(format)
	, m_texid(texid)
{
}

Texture::~Texture()
{
	if (m_texid != 0) {
		m_rc->ReleaseTexture(m_texid);
	}
}

void Texture::Upload(RenderContext* rc, int width, int height, TEXTURE_FORMAT format,
	                 const unsigned char* filling, bool filter_linear)
{
	if (m_texid != 0) {
		m_rc->ReleaseTexture(m_texid);
	}
	m_rc = rc;
	m_width  = width;
	m_height = height;
	m_format = format;

	if (filling == nullptr)
	{
		size_t sz = Utility::CalcTextureSize(m_format, m_width, m_height);
		uint8_t* pixels = new uint8_t[sz];
		memset(pixels, 0, sz);

		m_texid = m_rc->CreateTexture(pixels, m_width, m_height, m_format, 0, filter_linear ? 1 : 0);

		delete[] pixels;
	}
	else
	{
		m_texid = m_rc->CreateTexture(filling, m_width, m_height, m_format, 0, filter_linear ? 1 : 0);
	}

}

}