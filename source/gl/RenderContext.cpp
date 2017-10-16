#include <gl/RenderContext.h>
#include <gl/ur_gl_typedef.h>

#include <ejoy2d/render.h>
#include <ejoy2d/opengl.h>
#include <logger.h>
#include <fault.h>

#include <cmath>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

namespace ur
{
namespace gl
{

RenderContext::RenderContext(const RenderContext::Callback& cb, int max_texture)
	: m_cb(cb)
{
	render_init_args RA;
	// todo: config these args
	RA.max_buffer  = 128;
	RA.max_layout  = MAX_LAYOUT;
	RA.max_target  = 128;
	RA.max_texture = max_texture;
	RA.max_shader  = MAX_SHADER;

	int smz = render_size(&RA);
	m_render = (render*)malloc(smz);
	m_render = render_init(&RA, m_render, smz);

	// Texture
	memset(m_textures, 0, sizeof(m_textures));

	// RenderTarget
//	m_curr_rt = render_query_target();

	m_rt_depth = 0;
	m_rt_layers[m_rt_depth++] = render_query_target();

	// State
	m_blend = true;
	m_blend_src = BLEND_ONE;
	m_blend_dst = BLEND_ONE_MINUS_SRC_ALPHA;
	m_blend_func = BLEND_FUNC_ADD;
	m_clear_mask = 0;
	m_vp_x = m_vp_y = m_vp_w = m_vp_h = -1;
	render_set_blendfunc(m_render, (EJ_BLEND_FORMAT)m_blend_src, (EJ_BLEND_FORMAT)m_blend_dst);
	render_set_blendeq(m_render, (EJ_BLEND_FUNC)m_blend_func);
	m_scissor = false;
	m_scissor_x = m_scissor_y = m_scissor_w = m_scissor_h = -1;

	m_etc2 = CheckETC2Support();
	LOGI("Support etc2 %d\n", m_etc2);
}

RenderContext::~RenderContext()
{
	render_exit(m_render);
	free(m_render);
}

int RenderContext::RenderVersion() const
{
	return render_version(m_render);
}

bool RenderContext::Init() const
{
#if OPENGLES < 2
	return glewInit() == GLEW_OK;
#else
	return true;
#endif
}

/************************************************************************/
/* Texture                                                              */
/************************************************************************/

int  RenderContext::CreateTexture(const void* pixels, int width, int height, int format)
{
	RID id = render_texture_create(m_render, width, height, (EJ_TEXTURE_FORMAT)(format), EJ_TEXTURE_2D, 0);
	render_texture_update(m_render, id, width, height, pixels, 0, 0);
	return id;
}

int RenderContext::CreateTextureID(int width, int height, int format)
{
	RID id = render_texture_create(m_render, width, height, (EJ_TEXTURE_FORMAT)(format), EJ_TEXTURE_2D, 0);
	return id;		
}

void RenderContext::ReleaseTexture(int id)
{
	// clear texture curr
	for (int i = 0; i < MAX_TEXTURE_CHANNEL; ++i) {
		if (m_textures[i] == id) {
			BindTexture(i, 0);
		}
	}

	render_release(m_render, EJ_TEXTURE, id);
}

void RenderContext::UpdateTexture(int tex_id, const void* pixels, int width, int height)
{
	render_texture_update(m_render, tex_id, width, height, pixels, 0, 0);
}

void RenderContext::UpdateSubTexture(const void* pixels, int x, int y, int w, int h, unsigned int id)
{
	render_texture_subupdate(m_render, id, pixels, x, y, w, h);
}

void RenderContext::BindTexture(int id, int channel)
{
	if (channel < 0 || channel >= MAX_TEXTURE_CHANNEL || m_textures[channel] == id) {
		return;
	}

	m_cb.flush_render_shader();

	m_textures[channel] = id;
	render_set(m_render, EJ_TEXTURE, id, channel);
}

void RenderContext::ClearTextureCache()
{
	render_clear_texture_cache(m_render);
}

int  RenderContext::GetCurrTexture() const
{
	return m_textures[0];
}

/************************************************************************/
/* RenderTarget                                                         */
/************************************************************************/

int  RenderContext::CreateRenderTarget(int id)
{
	GLuint gl_id = id;
	glGenFramebuffers(1, &gl_id);
	return gl_id;
}

void RenderContext::ReleaseRenderTarget(int id)
{
	GLuint gl_id = id;
	glDeleteFramebuffers(1, &gl_id);
}

void RenderContext::BindRenderTarget(int id)
{
	assert(m_rt_depth < MAX_RENDER_TARGET_LAYER);

	m_cb.flush_shader();

	int curr = m_rt_layers[m_rt_depth - 1];
	if (curr != id) {
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}

	m_rt_layers[m_rt_depth++] = id;
}

void RenderContext::BindRenderTargetTex(int tex_id)
{
	int gl_id = render_get_texture_gl_id(m_render, tex_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_id, 0);
}

void RenderContext::UnbindRenderTarget()
{
	assert(m_rt_depth > 1);

	m_cb.flush_shader();

	int curr = m_rt_layers[m_rt_depth - 1],
		prev = m_rt_layers[m_rt_depth - 2];
	if (curr != prev) {
		glBindFramebuffer(GL_FRAMEBUFFER, prev);
	}

	--m_rt_depth;
}

int  RenderContext::CheckRenderTargetStatus()
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		return 1;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		LOGW("%s", "Framebuffer incomplete: Attachment is NOT complete.\n");
		return 0;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		LOGW("%s", "Framebuffer incomplete: No image is attached to FBO.\n");
		return 0;
#if !defined(_WIN32) && !defined(__MACH__)
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		LOGW("%s", "Framebuffer incomplete: Attached images have different dimensions.\n");
		return 0;
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		LOGW("%s", "Unsupported by FBO implementation.\n");
		return 0;
	default:
		LOGW("%s", "Unknow error.\n");
		return 0;
	}
}

//void RenderContext::SetCurrRenderTarget(int id)
//{
////	render_set(RS->R, TARGET, id, 0);
//	m_curr_rt = id;	
//}
//
//int  RenderContext::GetCurrRenderTarget() const
//{
////	return render_get(RS->R, TARGET, 0);
//	return m_curr_rt;
//}

/************************************************************************/
/* Shader                                                               */
/************************************************************************/

int  RenderContext::CreateShader(const char* vs, const char* fs)
{
	struct shader_init_args args;
	args.vs = vs;
	args.fs = fs;
	args.texture = 0;
	return render_shader_create(m_render, &args);
}

void RenderContext::ReleaseShader(int id)
{
	render_release(m_render, EJ_SHADER, id);
}

void RenderContext::BindShader(int id)
{
	render_shader_bind(m_render, id);
}

int RenderContext::GetShaderUniform(const char* name)
{
	return render_shader_locuniform(m_render, name);
}

void RenderContext::SetShaderUniform(int loc, UNIFORM_FORMAT format, const float* v)
{
	render_shader_setuniform(m_render, loc, (EJ_UNIFORM_FORMAT)format, v);
}

/************************************************************************/
/* State                                                                */
/************************************************************************/

void RenderContext::EnableBlend(bool blend)
{
	if (m_blend == blend) {
		return;
	}

	m_blend = blend;
	m_cb.flush_shader();

	if (blend) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
}

void RenderContext::SetBlend(int m1, int m2)
{
	if (m1 == m_blend_src && m2 == m_blend_dst) {
		return;
	}

	m_cb.flush_shader();

	m_blend_src = static_cast<BLEND_FORMAT>(m1);
	m_blend_dst = static_cast<BLEND_FORMAT>(m2);
	render_set_blendfunc(m_render, (EJ_BLEND_FORMAT)m_blend_src, (EJ_BLEND_FORMAT)m_blend_dst);
}

void RenderContext::SetBlendEquation(int func)
{
	if (func == m_blend_func) {
		return;
	}

	m_cb.flush_shader();

	m_blend_func = static_cast<BLEND_FUNC>(func);
	render_set_blendeq(m_render, (EJ_BLEND_FUNC)m_blend_func);
}

void RenderContext::SetDefaultBlend()
{
	SetBlend(BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);
	SetBlendEquation(BLEND_FUNC_ADD);
}

void RenderContext::SetClearFlag(int flag)
{
	m_clear_mask |= flag;
}

void RenderContext::Clear(unsigned long argb)
{
	m_cb.flush_shader();

	render_clear(m_render, (EJ_CLEAR_MASK)m_clear_mask, argb);
}

void RenderContext::EnableScissor(int enable)
{
	if (static_cast<bool>(enable) == m_scissor) {
		return;
	}

	m_scissor = enable;
	m_cb.flush_shader();

	render_enablescissor(m_render, enable);
	if (enable) {
		render_setscissor(m_render, m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
	}
}

void RenderContext::SetScissor(int x, int y, int width, int height)
{
	if (m_scissor_x == x &&
		m_scissor_y == y &&
		m_scissor_w == width &&
		m_scissor_h == height) {
		return;
	}

	m_scissor_x = x;
	m_scissor_y = y;
	m_scissor_w = width;
	m_scissor_h = height;
	m_cb.flush_shader();

	assert(x >= 0 && y >= 0 && width >= 0 && height >= 0);
	render_setscissor(m_render, x, y, width, height);
}

void RenderContext::SetViewport(int x, int y, int w, int h)
{
	if (x == m_vp_x && y == m_vp_y &&
		w == m_vp_w && h == m_vp_h) {
		return;
	}

	m_vp_x = x;
	m_vp_y = y;
	m_vp_w = w;
	m_vp_h = h;

	render_setviewport(m_vp_x, m_vp_y, m_vp_w, m_vp_h);
}

void RenderContext::GetViewport(int& x, int& y, int& w, int& h)
{
	x = m_vp_x;
	y = m_vp_y;
	w = m_vp_w;
	h = m_vp_h;
}

void RenderContext::SetDepth(DEPTH_FORMAT d)
{
	render_setdepth(m_render, (EJ_DEPTH_FORMAT)d);
}

bool RenderContext::IsTexture(int id) const
{
	return glIsTexture(id);
}

bool RenderContext::OutOfMemory() const
{
	GLenum err = glGetError();
	return err == GL_OUT_OF_MEMORY;
}

void RenderContext::CheckError() const
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		fault("gl error %d\n", error);
	}
}

void RenderContext::SetPointSize(float size)
{
#if OPENGLES < 2
	glPointSize(size);
#endif
}

void RenderContext::SetLineWidth(float size)
{
#if OPENGLES < 2
	glLineWidth(size);
#endif
}

void RenderContext::EnableLineStripple(bool stripple)
{
#if OPENGLES < 2
	if (stripple) {
		glEnable(GL_LINE_STIPPLE);
	} else {
		glDisable(GL_LINE_STIPPLE);
	}
#endif
}

void RenderContext::SetLineStripple(int pattern)
{
#if OPENGLES < 2
	glLineStipple(1, pattern);
#endif
}

/************************************************************************/
/* Draw                                                                 */
/************************************************************************/

void RenderContext::DrawElements(DRAW_MODE mode, int fromidx, int ni)
{
	render_draw_elements(m_render, (EJ_DRAW_MODE)mode, fromidx, ni);
}

void RenderContext::DrawArrays(DRAW_MODE mode, int fromidx, int ni)
{
	render_draw_arrays(m_render, (EJ_DRAW_MODE)mode, fromidx, ni);
}

int  RenderContext::CreateBuffer(RENDER_OBJ what, const void *data, int n, int stride)
{
	return render_buffer_create(m_render, (EJ_RENDER_OBJ)what, NULL, n, stride);
}

void RenderContext::ReleaseBuffer(RENDER_OBJ what, int id)
{
	render_release(m_render, (EJ_RENDER_OBJ)what, id);
}

void RenderContext::BindBuffer(RENDER_OBJ what, int id)
{
	render_set(m_render, (EJ_RENDER_OBJ)what, id, 0);
}

void RenderContext::UpdateBuffer(int id, const void* data, int n)
{
	render_buffer_update(m_render, id, data, n);
}

int  RenderContext::CreateVertexLayout(const std::vector<VertexAttrib>& va_list)
{
	struct vertex_attrib va[MAX_LAYOUT];
	int offset = 0;
	for (size_t i = 0, n = va_list.size(); i < n; ++i)
	{
		const VertexAttrib& src = va_list[i];
		vertex_attrib& dst = va[i];
		dst.name = src.name.c_str();
		dst.vbslot = 0;	// todo
		dst.n = src.n;
		dst.size = src.size;
		dst.offset = offset;
		offset += src.tot_size;
	}

	return render_register_vertexlayout(m_render, (int)(va_list.size()), va);
}

void RenderContext::ReleaseVertexLayout(int id)
{
	render_release(m_render, EJ_VERTEXLAYOUT, id);
}

void RenderContext::BindVertexLayout(int id)
{
	render_set(m_render, EJ_VERTEXLAYOUT, id, 0);
}

/************************************************************************/
/* Debug                                                                */
/************************************************************************/

int RenderContext::GetRealTexID(int id)
{
	return render_get_texture_gl_id(m_render, id);
}

/************************************************************************/
/* Other                                                                */
/************************************************************************/

void RenderContext::ReadBuffer()
{
#if OPENGLES != 2
	glReadBuffer(GL_COLOR_ATTACHMENT0);
#endif // OPENGLES
}

void RenderContext::ReadPixels(const void* pixels, int channels, int x, int y, int w, int h)
{
	if (channels == 4) {
		glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixels);
	} else if (channels == 3) {
		glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)pixels);
	}
}

bool RenderContext::CheckAvailableMemory(int need_texture_area) const
{
	const int EDGE = 1024;
	const int AREA = EDGE * EDGE;
	uint8_t* empty_data = new uint8_t[AREA * 4];
	memset(empty_data, 0x00, AREA * 4);

	glActiveTexture(GL_TEXTURE0);

	int max_count = static_cast<int>(std::ceil((float)need_texture_area / AREA));
	GLuint* id_list = new GLuint[max_count];

	int curr_area = 0;
	int curr_count = 0;
	while (true) {
		unsigned int texid = 0;
		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)EDGE, (GLsizei)EDGE, 0, GL_RGBA, GL_UNSIGNED_BYTE, empty_data);
		if (glGetError() == GL_OUT_OF_MEMORY) {
			break;
		} else {
			id_list[curr_count++] = texid;
			curr_area += AREA;
			if (curr_area >= need_texture_area) {
				break;
			}
		}
	}

	glDeleteTextures(curr_count, id_list);

	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] empty_data;
	delete[] id_list;

	return curr_area >= need_texture_area;
}

bool RenderContext::CheckETC2Support()
{
	bool etc2 = CheckETC2SupportFast();
	if (!etc2) {
		etc2 = CheckETC2SupportSlow();
	}
	return etc2;
}

bool RenderContext::CheckETC2SupportFast()
{
	bool ret = false;
#if defined( __APPLE__ ) && !defined(__MACOSX)
    std::string gl_ext = (char*)glGetString(GL_EXTENSIONS);
    ret = gl_ext.find("GL_OES_depth_texture_cube_map") != std::string::npos;
#elif defined _WIN32
	std::string gl_ext = (char*)glGetString(GL_EXTENSIONS);
	ret = gl_ext.find("GL_ARB_ES3_compatibility") != std::string::npos;
#else
	GLint num;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num);
	std::vector<GLint> fmt_list(num);
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, &fmt_list[0]);
	for (int i = 0, n = fmt_list.size(); i < n; ++i) {
		if (fmt_list[i] == 0x9278) {
			ret = true;
			break;
		}		
	}
#endif
	return ret;
}

bool RenderContext::CheckETC2SupportSlow()
{
#if defined( __APPLE__ ) && !defined(__MACOSX)
    return false;
#endif
    
	bool ret = false;
    
	const int WIDTH = 4;
	const int HEIGHT = 4;
	const int BPP = 8;
	const int SIZE = WIDTH * HEIGHT * BPP / 8;
	char pixels[SIZE];
	memset(pixels, 0, SIZE);

	GLuint tex_id;
	glGenTextures(1, &tex_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	glGetError();
	glCompressedTexImage2D(GL_TEXTURE_2D, 0, 0x9278, 4, 4, 0, SIZE, pixels);
	GLenum error = glGetError();
	ret = error == GL_NO_ERROR;

	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &tex_id);

	return ret;
}

}
}
