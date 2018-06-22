#ifndef _UNIRENDER_TEXTURE_H_
#define _UNIRENDER_TEXTURE_H_

#include "unirender/typedef.h"

#include <cu/uncopyable.h>

namespace ur
{

class RenderContext;

class Texture : private cu::Uncopyable
{
public:
	Texture(RenderContext* rc, int width, int height,
		int format = TEXTURE_RGBA8, bool filter_linear = true);
	~Texture();

	int Width() const { return m_width; }
	int Height() const { return m_height; }

	int ID() const { return m_id; }

	void Bind() const;

private:
	void Init(bool filter_linear);

private:
	RenderContext* m_rc;

	int m_width, m_height;
	int m_format;

	int m_id;

}; // Texture

}

#endif // _UNIRENDER_TEXTURE_H_