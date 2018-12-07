#pragma once

#include "unirender/typedef.h"

#include <cu/uncopyable.h>

namespace ur
{

class RenderContext;

class Texture3D : private cu::Uncopyable
{
public:
	Texture3D();
	Texture3D(RenderContext* rc, int width, int height, int depth,
		TEXTURE_FORMAT format, unsigned int texid);
	~Texture3D();

	void Upload(RenderContext* rc, int width, int height, int depth, TEXTURE_FORMAT format = TEXTURE_RGBA8,
		const unsigned char* filling = nullptr, bool filter_linear = true);

	int Width() const { return m_width; }
	int Height() const { return m_height; }
	int Depth() const { return m_depth; }

	auto Format() const { return m_format; }

	unsigned int TexID() const { return m_texid; }

protected:
	RenderContext* m_rc = nullptr;

	int m_width = 0;
	int m_height = 0;
	int m_depth = 0;
	TEXTURE_FORMAT m_format = TEXTURE_INVALID;

	unsigned int m_texid = 0;

}; // Texture3D

}