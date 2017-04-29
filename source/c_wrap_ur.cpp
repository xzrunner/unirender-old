#include "UR_RenderContext.h"

#include <stdint.h>

namespace ur
{

extern "C"
bool ur_init(void* _rc)
{
	RenderContext* rc = static_cast<RenderContext*>(_rc);
	return rc->Init();
}

extern "C"
void ur_clear(void* _rc, uint32_t argb)
{
	RenderContext* rc = static_cast<RenderContext*>(_rc);
	rc->Clear(argb);
}

extern "C"
void ur_set_viewport(void* _rc, int x, int y, int w, int h)
{
	RenderContext* rc = static_cast<RenderContext*>(_rc);
	rc->SetViewport(x, y, w, h);
}

extern "C"
void ur_set_point_size(void* _rc, float size)
{
	RenderContext* rc = static_cast<RenderContext*>(_rc);
	rc->SetPointSize(size);
}

extern "C"
void ur_set_line_width(void* _rc, float width)
{
	RenderContext* rc = static_cast<RenderContext*>(_rc);
	rc->SetLineWidth(width);
}

extern "C"
void ur_enable_line_stripple(void* _rc, bool stripple)
{
	RenderContext* rc = static_cast<RenderContext*>(_rc);
	rc->EnableLineStripple(stripple);
}

extern "C"
void ur_set_line_stripple(void* _rc, int pattern)
{
	RenderContext* rc = static_cast<RenderContext*>(_rc);
	rc->SetLineStripple(pattern);
}

extern "C"
void ur_check_error(void* _rc)
{
	RenderContext* rc = static_cast<RenderContext*>(_rc);
	rc->CheckError();
}

}