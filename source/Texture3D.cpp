#include "unirender/Texture3D.h"
#include "unirender/RenderContext.h"
#include "unirender/Utility.h"

#include <stdint.h>
#include <string.h>

namespace ur
{

Texture3D::Texture3D()
	: m_rc(nullptr)
	, m_width(0)
	, m_height(0)
	, m_depth(0)
	, m_format(TEXTURE_INVALID)
	, m_texid(0)
{
}

Texture3D::Texture3D(RenderContext* rc, int width, int height, int depth,
	                 TEXTURE_FORMAT format, unsigned int texid)
	: m_rc(rc)
	, m_width(width)
	, m_height(height)
	, m_depth(depth)
	, m_format(format)
	, m_texid(texid)
{
}

Texture3D::~Texture3D()
{
	if (m_texid != 0) {
		m_rc->ReleaseTexture(m_texid);
	}
}

void Texture3D::Upload(RenderContext* rc, int width, int height, int depth, TEXTURE_FORMAT format,
	                 const unsigned char* filling, bool filter_linear)
{
	if (m_texid != 0) {
		m_rc->ReleaseTexture(m_texid);
	}
	m_rc = rc;
	m_width  = width;
	m_height = height;
	m_depth  = depth;
	m_format = format;

	if (filling == nullptr)
	{
		size_t sz = Utility::CalcTextureSize(m_format, m_width, m_height, m_depth);
		uint8_t* pixels = new uint8_t[sz];
		memset(pixels, 0, sz);

		m_texid = m_rc->CreateTexture3D(pixels, m_width, m_height, m_depth, m_format);

		delete[] pixels;
	}
	else
	{
		m_texid = m_rc->CreateTexture3D(filling, m_width, m_height, m_depth, m_format);
	}

}

}