#ifndef _UNIRENDER_TEXTURE_H_
#define _UNIRENDER_TEXTURE_H_

#include "unirender/typedef.h"

#include <cu/uncopyable.h>

#include <memory>

namespace ur
{

class RenderContext;

class Texture : private cu::Uncopyable
{
public:
	Texture();
	Texture(RenderContext* rc, int width, int height,
		TEXTURE_FORMAT format, unsigned int texid);
	~Texture();

	void Upload(RenderContext* rc, int width, int height, TEXTURE_FORMAT format = TEXTURE_RGBA8,
		const unsigned char* filling = nullptr, bool filter_linear = true);

	int Width() const { return m_width; }
	int Height() const { return m_height; }

	auto Format() const { return m_format; }

	unsigned int TexID() const { return m_texid; }

protected:
	RenderContext* m_rc = nullptr;

	int m_width = 0, m_height = 0;
	TEXTURE_FORMAT m_format = TEXTURE_INVALID;

	unsigned int m_texid = 0;

}; // Texture

using TexturePtr = std::shared_ptr<Texture>;

}

#endif // _UNIRENDER_TEXTURE_H_