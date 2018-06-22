#ifndef _UNIRENDER_RENDER_TARGET_H_
#define _UNIRENDER_RENDER_TARGET_H_

#include <cu/uncopyable.h>

namespace ur
{

class RenderContext;
class Texture;

class RenderTarget : private cu::Uncopyable
{
public:
	RenderTarget(RenderContext* rc, int width, int height, bool has_depth = false);
	~RenderTarget();

	void Bind();
	void Unbind();

	int Width() const;
	int Height() const;
	int TexID() const;

	void Resize(int width, int height);

private:
	RenderContext* m_rc;

	Texture* m_color_tex;
	Texture* m_depth_tex;

	int m_id;

}; // RenderTarget

}

#endif // _UNIRENDER_RENDER_TARGET_H_