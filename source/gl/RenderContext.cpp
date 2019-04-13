#include "unirender/gl/RenderContext.h"
#include "unirender/gl/typedef.h"
#include "unirender/Utility.h"

#include <guard/check.h>
#include <ejoy2d/render.h>
#include <ejoy2d/opengl.h>
#include <logger.h>
#include <fault.h>

#include <cmath>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define CHECK_MT

#ifdef CHECK_MT
#include <thread>
#endif // CHECK_MT

namespace ur
{
namespace gl
{

#ifdef CHECK_MT
static std::thread::id MAIN_THREAD_ID;
#endif // CHECK_MT

RenderContext::RenderContext(int max_texture, std::function<void(ur::RenderContext&)> flush_shader)
	: m_flush_shader(std::move(flush_shader))
{
#ifdef CHECK_MT
	MAIN_THREAD_ID = std::this_thread::get_id();
#endif // CHECK_MT

#if OPENGLES < 2
	// Initialize GLEW to setup the OpenGL Function pointers
//	return glewInit() == GLEW_OK;
	glewInit();
#endif

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
	m_alpha_func = ALPHA_ALWAYS;
	m_alpha_ref = 0;
	m_depth = false;
	m_depth_fmt = DEPTH_DISABLE;
	m_clear_mask = 0;
	m_vp_x = m_vp_y = m_vp_w = m_vp_h = -1;
	render_set_blendfunc(m_render, (EJ_BLEND_FORMAT)m_blend_src, (EJ_BLEND_FORMAT)m_blend_dst);
	render_set_blendeq(m_render, (EJ_BLEND_FUNC)m_blend_func);
	render_set_alpha_test(m_render, (EJ_ALPHA_FUNC)m_alpha_func, m_alpha_ref);
	m_scissor = false;
	m_scissor_x = m_scissor_y = m_scissor_w = m_scissor_h = -1;

#if defined( __APPLE__ ) && !defined(__MACOSX)
#else
	m_etc2 = CheckETC2Support();
#endif
	LOGI("Support etc2 %d\n", IsSupportETC2());
}

RenderContext::~RenderContext()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

    glDeleteVertexArrays(1, &m_cube_vao);
    glDeleteBuffers(1, &m_cube_vbo);

	render_exit(m_render);
	free(m_render);
}

int RenderContext::RenderVersion() const
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	return render_version(m_render);
}

/************************************************************************/
/* Texture                                                              */
/************************************************************************/

int  RenderContext::CreateTexture(const void* pixels, int width, int height, int format,
                                  int mipmap_levels, TEXTURE_WRAP wrap, TEXTURE_FILTER filter)
{
	CheckError();

#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	RID id = render_texture_create(m_render, width, height, 0, (EJ_TEXTURE_FORMAT)(format), EJ_TEXTURE_2D, mipmap_levels);

	render_texture_update(m_render, id, width, height, 0, pixels, 0, 0,
        static_cast<EJ_TEXTURE_WRAP>(wrap), static_cast<EJ_TEXTURE_FILTER>(filter));

	return id;
}

int RenderContext::CreateTexture3D(const void* pixels, int width, int height, int depth, int format)
{
	CheckError();

#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	RID id = render_texture_create(m_render, width, height, depth, (EJ_TEXTURE_FORMAT)(format), EJ_TEXTURE_3D, 0);

    render_texture_update(m_render, id, width, height, depth, pixels, 0, 0, EJ_TEXTURE_REPEAT, EJ_TEXTURE_LINEAR);

	return id;
}

int RenderContext::CreateTextureCube(int width, int height, int mipmap_levels)
{
    CheckError();

#ifdef CHECK_MT
    assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

    RID id = render_texture_create(m_render, 0, 0, 0, EJ_TEXTURE_RGB16F, EJ_TEXTURE_CUBE, mipmap_levels);

    render_texture_update(m_render, id, width, height, 0, nullptr, 0, 0, EJ_TEXTURE_REPEAT, EJ_TEXTURE_LINEAR);

    return id;
}

int RenderContext::CreateTextureID(int width, int height, int format, int mipmap_levels)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	RID id = render_texture_create(m_render, width, height, 0, (EJ_TEXTURE_FORMAT)(format), EJ_TEXTURE_2D, mipmap_levels);
	return id;
}

void RenderContext::ReleaseTexture(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	// clear texture curr
	for (int i = 0; i < MAX_TEXTURE_CHANNEL; ++i) {
		if (m_textures[i] == id) {
			BindTexture(0, i);
		}
	}

	render_release(m_render, EJ_TEXTURE, id);
}

void RenderContext::UpdateTexture(int tex_id, const void* pixels, int width, int height, int slice,
                                  int miplevel, TEXTURE_WRAP wrap, TEXTURE_FILTER filter)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_texture_update(m_render, tex_id, width, height, 0, pixels, slice, miplevel,
        static_cast<EJ_TEXTURE_WRAP>(wrap), static_cast<EJ_TEXTURE_FILTER>(filter));
}

void RenderContext::UpdateTexture3d(int tex_id, const void* pixels, int width, int height, int depth)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

    render_texture_update(m_render, tex_id, width, height, depth, pixels, 0, 0, EJ_TEXTURE_REPEAT, EJ_TEXTURE_LINEAR);
}

void RenderContext::UpdateSubTexture(const void* pixels, int x, int y, int w, int h, unsigned int id, int slice, int miplevel)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_texture_subupdate(m_render, id, pixels, x, y, w, h, slice, miplevel);
}

void RenderContext::BindTexture(int id, int channel)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (channel < 0 || channel >= MAX_TEXTURE_CHANNEL || m_textures[channel] == id) {
		return;
	}

	CallFlushCB();

	m_textures[channel] = id;
	render_set(m_render, EJ_TEXTURE, id, channel);
}

void RenderContext::ClearTextureCache()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_clear_texture_cache(m_render);
}

int  RenderContext::GetCurrTexture() const
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	return m_textures[0];
}

/************************************************************************/
/* RenderTarget                                                         */
/************************************************************************/

int  RenderContext::CreateRenderTarget(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	GLuint gl_id = id;
	glGenFramebuffers(1, &gl_id);
	return gl_id;
}

void RenderContext::ReleaseRenderTarget(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	GLuint gl_id = id;
	glDeleteFramebuffers(1, &gl_id);
}

void RenderContext::BindRenderTarget(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	assert(m_rt_depth < MAX_RENDER_TARGET_LAYER);

	CallFlushCB();

	int curr = m_rt_layers[m_rt_depth - 1];
	if (curr != id) {
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}

	m_rt_layers[m_rt_depth++] = id;
}

void RenderContext::UnbindRenderTarget()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	assert(m_rt_depth > 1);

	CallFlushCB();

	int curr = m_rt_layers[m_rt_depth - 1],
		prev = m_rt_layers[m_rt_depth - 2];
	if (curr != prev) {
		glBindFramebuffer(GL_FRAMEBUFFER, prev);
	}

	--m_rt_depth;
}

size_t RenderContext::GetRenderTargetDepth() const
{
    return m_rt_depth;
}

void RenderContext::BindRenderTargetTex(int tex, int attachment, int textarget, int level)
{
#ifdef CHECK_MT
    assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

    int gl_tex = render_get_texture_gl_id(m_render, tex);

    GLenum gl_attachment = GL_COLOR_ATTACHMENT0;
    switch (attachment)
    {
    case ATTACHMENT_DEPTH:
        gl_attachment = GL_DEPTH_ATTACHMENT;
        break;
    }

    GLenum gl_textarget = GL_TEXTURE_2D;
    switch (textarget)
    {
    case TEXTURE_CUBE0:
        gl_textarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
        break;
    case TEXTURE_CUBE1:
        gl_textarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
        break;
    case TEXTURE_CUBE2:
        gl_textarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
        break;
    case TEXTURE_CUBE3:
        gl_textarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
        break;
    case TEXTURE_CUBE4:
        gl_textarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
        break;
    case TEXTURE_CUBE5:
        gl_textarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
        break;
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, gl_attachment, gl_textarget, gl_tex, level);
}

int  RenderContext::CheckRenderTargetStatus()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

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
/* PixelBuffer                                                          */
/************************************************************************/

int RenderContext::CreatePixelBuffer(uint32_t id, int width, int height, int format)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	GLuint gl_id = id;
	glGenBuffers(1, &gl_id);
	BindPixelBuffer(gl_id);
	size_t sz = Utility::CalcTextureSize(format, width, height);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, sz, 0, GL_STREAM_DRAW);
	UnbindPixelBuffer();
	return gl_id;
}

void RenderContext::ReleasePixelBuffer(uint32_t id)
{
	glDeleteBuffers(1, &id);
}

void RenderContext::BindPixelBuffer(uint32_t id)
{
	if (m_pbo == id) {
		return;
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, id);
	m_pbo = id;
}

void RenderContext::UnbindPixelBuffer()
{
	if (m_pbo != 0) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		m_pbo = 0;
	}
}

void* RenderContext::MapPixelBuffer(ACCESS_MODE mode)
{
	uint32_t gl_mode;
	switch (mode)
	{
	case READ_ONLY:
		gl_mode = GL_READ_ONLY;
		break;
	case WRITE_ONLY:
		gl_mode = GL_WRITE_ONLY;
		break;
	case READ_WRITE:
		gl_mode = GL_READ_WRITE;
		break;
	}
	return glMapBuffer(GL_PIXEL_UNPACK_BUFFER, gl_mode);
}

void  RenderContext::UnmapPixelBuffer()
{
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
}

/************************************************************************/
/* Shader                                                               */
/************************************************************************/

int  RenderContext::CreateShader(const char* vs, const char* fs, const std::vector<std::string>& textures, bool no_header)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	struct shader_init_args args;

	args.vs = vs;
	args.fs = fs;
    args.no_header = no_header ? 1 : 0;

	int n = textures.size();
	args.texture = n;
	if (n > 0)
	{
		args.texture_uniform = (const char**)malloc(sizeof(char*) * n);
		for (int i = 0; i < args.texture; ++i)
		{
			auto& src = textures[i];
			char* dst = (char*)malloc(src.size() + 1);
			strcpy(dst, src.c_str());
			args.texture_uniform[i] = dst;
		}
	}
	else
	{
		args.texture_uniform = NULL;
	}

	return render_shader_create(m_render, &args);
}

void RenderContext::ReleaseShader(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_release(m_render, EJ_SHADER, id);
}

void RenderContext::BindShader(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_shader_bind(m_render, id);

    m_binded_shader = id;
}

int RenderContext::GetBindedShader() const
{
    return m_binded_shader;
}

int RenderContext::GetShaderUniform(const char* name)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	return render_shader_locuniform(m_render, name);
}

void RenderContext::SetShaderUniform(int loc, UNIFORM_FORMAT format, const float* v, int n)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_shader_setuniform(m_render, loc, (EJ_UNIFORM_FORMAT)format, v, n);
}

/************************************************************************/
/* State                                                                */
/************************************************************************/

void RenderContext::EnableBlend(bool blend)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (m_blend == blend) {
		return;
	}

	CallFlushCB();

	m_blend = blend;
	if (blend) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
}

void RenderContext::SetBlend(int m1, int m2)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (m1 == m_blend_src && m2 == m_blend_dst) {
		return;
	}

	CallFlushCB();

	m_blend_src = static_cast<BLEND_FORMAT>(m1);
	m_blend_dst = static_cast<BLEND_FORMAT>(m2);
	render_set_blendfunc(m_render, (EJ_BLEND_FORMAT)m_blend_src, (EJ_BLEND_FORMAT)m_blend_dst);
}

void RenderContext::SetBlendEquation(int func)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (func == m_blend_func) {
		return;
	}

	CallFlushCB();

	m_blend_func = static_cast<BLEND_FUNC>(func);
	render_set_blendeq(m_render, (EJ_BLEND_FUNC)m_blend_func);
}

void RenderContext::SetDefaultBlend()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	SetBlend(BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);
	SetBlendEquation(BLEND_FUNC_ADD);
}

void RenderContext::SetAlphaTest(ALPHA_FUNC func, float ref)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (func == m_alpha_func && ref == m_alpha_ref) {
		return;
	}

	CallFlushCB();

	m_alpha_func = func;
	m_alpha_ref  = ref;
	render_set_alpha_test(m_render, (EJ_ALPHA_FUNC)func, ref);
}

void RenderContext::EnableDepthMask(bool depth)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (m_depth == depth) {
		return;
	}

	CallFlushCB();

	m_depth = depth;
	render_enabledepthmask(m_render, m_depth);
}

void RenderContext::SetDepthTest(DEPTH_FORMAT fmt)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (m_depth_fmt == fmt) {
		return;
	}

	CallFlushCB();

	m_depth_fmt = fmt;
	render_setdepth(m_render, EJ_DEPTH_FORMAT(m_depth_fmt));
}

void RenderContext::SetFrontFace(bool clockwise)
{
	render_set_front_face(m_render, clockwise);
}

void RenderContext::SetCull(CULL_MODE cull)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (m_cull == cull) {
		return;
	}

	m_cull = cull;
	render_setcull(m_render, EJ_CULL_MODE(cull));
}

int RenderContext::GetBindedVertexLayoutID()
{
    return render_get_binded_vertexlayout(m_render);
}

void RenderContext::SetClearFlag(int flag)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	m_clear_mask = flag;
}

void RenderContext::SetClearColor(uint32_t argb)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	m_clear_color = argb;
}

void RenderContext::Clear()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	CallFlushCB();

	render_clear(m_render, (EJ_CLEAR_MASK)m_clear_mask, m_clear_color);
}

void RenderContext::EnableScissor(int enable)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (static_cast<bool>(enable) == m_scissor) {
		return;
	}

	CallFlushCB();

	m_scissor = enable;
	render_enablescissor(m_render, enable);
	if (enable) {
		render_setscissor(m_render, m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
	}
}

void RenderContext::SetScissor(int x, int y, int width, int height)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (m_scissor_x == x &&
		m_scissor_y == y &&
		m_scissor_w == width &&
		m_scissor_h == height) {
		return;
	}

	CallFlushCB();

	m_scissor_x = x;
	m_scissor_y = y;
	m_scissor_w = width;
	m_scissor_h = height;

	assert(x >= 0 && y >= 0 && width >= 0 && height >= 0);
	render_setscissor(m_render, x, y, width, height);
}

void RenderContext::SetViewport(int x, int y, int w, int h)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

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
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	x = m_vp_x;
	y = m_vp_y;
	w = m_vp_w;
	h = m_vp_h;
}

bool RenderContext::IsTexture(int id) const
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	return glIsTexture(id);
}

bool RenderContext::OutOfMemory() const
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	GLenum err = glGetError();
	return err == GL_OUT_OF_MEMORY;
}

void RenderContext::CheckError() const
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		fault("gl error %d\n", error);
	}
}

void RenderContext::SetPointSize(float size)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

#if OPENGLES < 2
	if (m_point_size == size) {
		return;
	}

	CallFlushCB();

	m_point_size = size;

	glPointSize(size);
#endif
}

void RenderContext::SetLineWidth(float size)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

#if OPENGLES < 2
	if (m_line_width == size) {
		return;
	}

	CallFlushCB();

	m_line_width = size;

	glLineWidth(size);
#endif
}

void RenderContext::SetPolygonMode(POLYGON_MODE poly_mode)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (m_poly_mode == poly_mode) {
		return;
	}

	CallFlushCB();

	m_poly_mode = poly_mode;

	switch (poly_mode)
	{
	case POLYGON_POINT:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	case POLYGON_LINE:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case POLYGON_FILL:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	}
}

void RenderContext::EnableLineStripple(bool stripple)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	if (m_line_stripple == stripple) {
		return;
	}

#if OPENGLES < 2
	CallFlushCB();

	m_line_stripple = stripple;

	if (stripple) {
		glEnable(GL_LINE_STIPPLE);
	} else {
		glDisable(GL_LINE_STIPPLE);
	}
#endif
}

void RenderContext::SetLineStripple(int pattern)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

#if OPENGLES < 2
	glLineStipple(1, pattern);
#endif
}

void RenderContext::SetUnpackRowLength(int len)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	glPixelStorei(GL_UNPACK_ROW_LENGTH, len);
}

/************************************************************************/
/* Draw                                                                 */
/************************************************************************/

void RenderContext::DrawElements(DRAW_MODE mode, int fromidx, int ni)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_draw_elements(m_render, (EJ_DRAW_MODE)mode, fromidx, ni);
}

void RenderContext::DrawElements(DRAW_MODE mode, int count, unsigned int* indices)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_draw_elements_no_buf(m_render, (EJ_DRAW_MODE)mode, count, indices);
}

void RenderContext::DrawArrays(DRAW_MODE mode, int fromidx, int ni)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_draw_arrays(m_render, (EJ_DRAW_MODE)mode, fromidx, ni);
}

int  RenderContext::CreateBuffer(RENDER_OBJ what, const void *data, int size)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	return render_buffer_create(m_render, (EJ_RENDER_OBJ)what, data, size);
}

void RenderContext::ReleaseBuffer(RENDER_OBJ what, int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_release(m_render, (EJ_RENDER_OBJ)what, id);
}

void RenderContext::BindBuffer(RENDER_OBJ what, int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_set(m_render, (EJ_RENDER_OBJ)what, id, 0);
}

void RenderContext::UpdateBuffer(int id, const void* data, int size)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_buffer_update(m_render, id, data, size);
}

void RenderContext::UpdateBufferRaw(BUFFER_TYPE type, int id, const void* data, int size, int offset)
{
	glBindVertexArray(0);
	switch (type)
	{
	case BUFFER_VERTEX:
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
		break;
	case BUFFER_INDEX:
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
		break;
	}
}

int  RenderContext::CreateVertexLayout(const CU_VEC<VertexAttrib>& va_list)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	struct vertex_attrib va[MAX_LAYOUT];
	for (size_t i = 0, n = va_list.size(); i < n; ++i)
	{
		const VertexAttrib& src = va_list[i];
		vertex_attrib& dst = va[i];
		assert(src.name.size() < sizeof(dst.name) - 1);
		strncpy(dst.name, src.name.c_str(), src.name.size());
		dst.name[src.name.size()] = 0;
		dst.vbslot = 0;	// todo
		dst.n = src.n;
		dst.size = src.size;
		dst.stride = src.stride;
		dst.offset = src.offset;
	}

    m_binded_vertexlayout = render_register_vertexlayout(m_render, (int)(va_list.size()), va);
    return m_binded_vertexlayout;
}

void RenderContext::ReleaseVertexLayout(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_release(m_render, EJ_VERTEXLAYOUT, id);
}

void RenderContext::BindVertexLayout(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_set(m_render, EJ_VERTEXLAYOUT, id, 0);
    m_binded_vertexlayout = id;
}

int RenderContext::GetVertexLayout() const
{
    return m_binded_vertexlayout;
}

void RenderContext::UpdateVertexLayout(const CU_VEC<VertexAttrib>& va_list)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	struct vertex_attrib va[MAX_LAYOUT];
	for (size_t i = 0, n = va_list.size(); i < n; ++i)
	{
		const VertexAttrib& src = va_list[i];
		vertex_attrib& dst = va[i];

		assert(src.name.size() < sizeof(dst.name) - 1);
		strncpy(dst.name, src.name.c_str(), src.name.size());
		dst.name[src.name.size()] = 0;
		dst.vbslot = 0;	// todo
		dst.n = src.n;
		dst.size = src.size;
		dst.stride = src.stride;
		dst.offset = src.offset;
	}

	return render_update_vertexlayout(m_render, (int)(va_list.size()), va);
}

void RenderContext::CreateVAO(const VertexInfo& vi,
	                          unsigned int& vao,
	                          unsigned int& vbo,
	                          unsigned int& ebo)
{
	bool element = vi.in != 0;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	if (element) {
		glGenBuffers(1, &ebo);
	} else {
		ebo = 0;
	}

	glBindVertexArray(vao);

	auto gl_usage = [](BUFFER_USAGE usage)->GLenum {
		auto ret = GL_STATIC_DRAW;
		switch (usage)
		{
		case USAGE_STATIC:
			ret = GL_STATIC_DRAW;
			break;
		case USAGE_DYNAMIC:
			ret = GL_DYNAMIC_DRAW;
			break;
		case USAGE_STREAM:
			ret = GL_STREAM_DRAW;
			break;
		};
		return ret;
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vi.vn * vi.stride, vi.vertices, gl_usage(vi.vert_usage));

	if (element) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * vi.in, vi.indices, gl_usage(vi.index_usage));
	}

	size_t idx = 0;
	for (auto& va : vi.va_list)
	{
		GLenum type;
		GLboolean normalized;
		switch (va.size)
		{
		case 1:
			type = GL_UNSIGNED_BYTE;
			normalized = GL_TRUE;
			break;
		case 2:
			type = GL_UNSIGNED_SHORT;
			normalized = GL_TRUE;
			break;
		case 4:
			type = GL_FLOAT;
			normalized = GL_FALSE;
			break;
		default:
			assert(0);
		}

		glEnableVertexAttribArray(idx);
		glVertexAttribPointer(idx, va.n, type, normalized, va.stride, (const GLvoid *)(ptrdiff_t)(va.offset));

		++idx;
	}

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::ReleaseVAO(unsigned int vao, unsigned int vbo, unsigned int ebo)
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	if (ebo != 0) {
		glDeleteBuffers(1, &ebo);
	}
}

void RenderContext::DrawElementsVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_draw_elements_vao(m_render, (EJ_DRAW_MODE)mode, fromidx, ni, vao);
}

void RenderContext::DrawArraysVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	render_draw_arrays_vao(m_render, (EJ_DRAW_MODE)mode, fromidx, ni, vao);
}

void RenderContext::RenderCube()
{
    // initialize (if necessary)
    if (m_cube_vao == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &m_cube_vao);
        glGenBuffers(1, &m_cube_vbo);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_cube_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(m_cube_vao);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // render Cube
    int old_cull = m_cull;
    //SetCull(CULL_DISABLE);
    DrawArraysVAO(ur::DRAW_TRIANGLES, 0, 36, m_cube_vao);
    //SetCull(static_cast<CULL_MODE>(old_cull));
}

void RenderContext::RenderQuad()
{
    if (m_quad_vao == 0)
    {
        float vertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &m_quad_vao);
        glGenBuffers(1, &m_quad_vbo);
        glBindVertexArray(m_quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    // render quad
    int old_cull = m_cull;
    //SetCull(CULL_DISABLE);
    DrawArraysVAO(ur::DRAW_TRIANGLE_STRIP, 0, 4, m_quad_vao);
    //SetCull(static_cast<CULL_MODE>(old_cull));
}

/************************************************************************/
/* Debug                                                                */
/************************************************************************/

int RenderContext::GetRealTexID(int id)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	return render_get_texture_gl_id(m_render, id);
}

/************************************************************************/
/* Other                                                                */
/************************************************************************/

void RenderContext::ReadBuffer()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

#if OPENGLES != 2
	glReadBuffer(GL_COLOR_ATTACHMENT0);
#endif // OPENGLES
}

void RenderContext::ReadPixels(const void* pixels, int channels, int x, int y, int w, int h)
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

//	glReadBuffer(GL_COLOR_ATTACHMENT0);
	if (channels == 4) {
		glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixels);
	} else if (channels == 3) {
		glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)pixels);
	}
}

bool RenderContext::CheckAvailableMemory(int need_texture_area) const
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	const int EDGE = 1024;
	const int AREA = EDGE * EDGE;
	uint8_t* empty_data = new uint8_t[AREA * 2];
	memset(empty_data, 0x00, AREA * 2);

	glActiveTexture(GL_TEXTURE0);

	int max_count = static_cast<int>(std::ceil((float)need_texture_area / AREA));
	GLuint* id_list = new GLuint[max_count];

	int curr_area = 0;
	int curr_count = 0;
	while (true) {
		unsigned int texid = 0;
		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)EDGE, (GLsizei)EDGE, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, empty_data);
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

void RenderContext::EnableFlushCB(bool enable)
{
	if (!enable) {
		++m_cb_enable;
	} else {
		--m_cb_enable;
	}
}

void RenderContext::CallFlushCB()
{
	if (m_cb_enable == 0 && m_flush_shader) {
		m_flush_shader(*this);
	}
}

bool RenderContext::CheckETC2Support()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	bool etc2 = CheckETC2SupportFast();
	if (!etc2) {
		etc2 = CheckETC2SupportSlow();
	}
	return etc2;
}

bool RenderContext::CheckETC2SupportFast()
{
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

	bool ret = false;
#if defined( __APPLE__ ) && !defined(__MACOSX)
#elif defined _WIN32
	GLint n;
	glGetIntegerv(GL_NUM_EXTENSIONS, &n);
	for (GLint i = 0; i < n; ++i) {
		if (strcmp((const char*)(glGetStringi(GL_EXTENSIONS, i)), "GL_ARB_ES3_compatibility") == 0) {
			ret = true;
			break;
		}
	}
#else
	GLint num;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num);
	CU_VEC<GLint> fmt_list;
	fmt_list.resize(num);
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
#ifdef CHECK_MT
	assert(std::this_thread::get_id() == MAIN_THREAD_ID);
#endif // CHECK_MT

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
