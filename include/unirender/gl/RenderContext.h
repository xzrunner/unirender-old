#ifndef _UNIRENDER_GL_RENDER_CONTEXT_H_
#define _UNIRENDER_GL_RENDER_CONTEXT_H_

#include "unirender/RenderContext.h"

#include <functional>

struct render;

namespace ur
{
namespace gl
{

class RenderContext : public ur::RenderContext
{
public:
	RenderContext(int max_texture, std::function<void(ur::RenderContext&)> flush_shader);
	virtual ~RenderContext();

	virtual int RenderVersion() const override final;

	/************************************************************************/
	/* Texture                                                              */
	/************************************************************************/

	virtual int  CreateTexture(const void* pixels, int width, int height, int format, int mipmap_levels = 0, int linear = 1) override final;
	virtual int  CreateTexture3D(const void* pixels, int width, int height, int depth, int format) override final;
	virtual int  CreateTextureID(int width, int height, int format, int mipmap_levels = 0) override final;
	virtual void ReleaseTexture(int id) override final;

	virtual void UpdateTexture(int tex_id, const void* pixels, int width, int height, int slice = 0, int miplevel = 0, int flags = 0) override final;
	virtual void UpdateTexture3d(int tex_id, const void* pixels, int width, int height, int depth) override final;
	virtual void UpdateSubTexture(const void* pixels, int x, int y, int w, int h, unsigned int id, int slice = 0, int miplevel = 0) override final;

	virtual void BindTexture(int id, int channel) override final;

	virtual void ClearTextureCache() override final;

	virtual int  GetCurrTexture() const override final;

	/************************************************************************/
	/* RenderTarget                                                         */
	/************************************************************************/

	virtual int  CreateRenderTarget(int id) override final;
	virtual void ReleaseRenderTarget(int id) override final;

	virtual void BindRenderTarget(int id) override final;
	virtual void BindRenderTargetTex(int color_tex, int depth_tex = -1) override final;
	virtual void UnbindRenderTarget() override final;

	virtual int  CheckRenderTargetStatus() override final;

// 	virtual void SetCurrRenderTarget(int id) override final;
// 	virtual int  GetCurrRenderTarget() const override final;

	/************************************************************************/
	/* PixelBuffer                                                          */
	/************************************************************************/

	// todo: GL_PIXEL_PACK_BUFFER

	virtual int  CreatePixelBuffer(uint32_t id, int width, int height, int format) override final;
	virtual void ReleasePixelBuffer(uint32_t id) override final;

	virtual void BindPixelBuffer(uint32_t id) override final;
	virtual void UnbindPixelBuffer() override final;

	virtual void* MapPixelBuffer(ACCESS_MODE mode) override final;
	virtual void  UnmapPixelBuffer() override final;

	/************************************************************************/
	/* Shader                                                               */
	/************************************************************************/

	virtual int  CreateShader(const char* vs, const char* fs, const std::vector<std::string>& textures) override final;
	virtual void ReleaseShader(int id) override final;

	virtual void BindShader(int id) override final;

	virtual int  GetShaderUniform(const char* name) override final;
	virtual void SetShaderUniform(int loc, UNIFORM_FORMAT format, const float* v, int n = 1) override final;

	/************************************************************************/
	/* State                                                                */
	/************************************************************************/

	virtual void EnableBlend(bool blend) override final;
	virtual void SetBlend(int m1, int m2) override final;
	virtual void SetBlendEquation(int func) override final;
	virtual void SetDefaultBlend() override final;

	virtual void EnableDepthMask(bool depth) override final;
	virtual void SetDepthTest(DEPTH_FORMAT fmt) override final;

	virtual void SetFrontFace(bool clockwise) override final;
	virtual void SetCull(CULL_MODE cull) override final;

	virtual void SetClearFlag(int flag) override final;
	virtual void SetClearColor(uint32_t arbg) override final;
	virtual void Clear() override final;

	virtual void EnableScissor(int enable) override final;
	virtual void SetScissor(int x, int y, int width, int height) override final;

	virtual void SetViewport(int x, int y, int w, int h) override final;
	virtual void GetViewport(int& x, int& y, int& w, int& h) override final;

	virtual bool IsTexture(int id) const override final;

	virtual bool OutOfMemory() const override final;
	virtual void CheckError() const override final;

	virtual void SetPointSize(float size) override final;
	virtual void SetLineWidth(float size) override final;

	virtual void SetPolygonMode(POLYGON_MODE poly_mode) override final;

	virtual void EnableLineStripple(bool stripple) override final;
	virtual void SetLineStripple(int pattern) override final;

	virtual void SetUnpackRowLength(int len) override final;

	/************************************************************************/
	/* Draw                                                                 */
	/************************************************************************/

	virtual void DrawElements(DRAW_MODE mode, int fromidx, int ni) override final;
	virtual void DrawElements(DRAW_MODE mode, int count, unsigned int* indices) override final;
	virtual void DrawArrays(DRAW_MODE mode, int fromidx, int ni) override final;

	virtual int  CreateBuffer(RENDER_OBJ what, const void *data, int size) override final;
	virtual void ReleaseBuffer(RENDER_OBJ what, int id) override final;
	virtual void BindBuffer(RENDER_OBJ what, int id) override final;
	virtual void UpdateBuffer(int id, const void* data, int size) override final;
	virtual void UpdateBufferRaw(BUFFER_TYPE type, int id, const void* data, int size, int offset = 0) override final;

	virtual int  CreateVertexLayout(const CU_VEC<VertexAttrib>& va_list) override final;
	virtual void ReleaseVertexLayout(int id) override final;
	virtual void BindVertexLayout(int id) override final;
	virtual void UpdateVertexLayout(const CU_VEC<VertexAttrib>& va_list) override final;

	virtual void CreateVAO(const VertexInfo& vi, unsigned int& vao, unsigned int& vbo, unsigned int& ebo) override final;
	virtual void ReleaseVAO(unsigned int vao, unsigned int vbo, unsigned int ebo) override final;
	virtual void DrawElementsVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao) override final;
	virtual void DrawArraysVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao) override final;

	/************************************************************************/
	/* Debug                                                                */
	/************************************************************************/

	virtual int  GetRealTexID(int id) override final;

	/************************************************************************/
	/* Other                                                                */
	/************************************************************************/

	virtual void ReadBuffer() override final;
	virtual void ReadPixels(const void* pixels, int channels, int x, int y, int w, int h) override final;

	virtual bool CheckAvailableMemory(int need_texture_area) const override final;

	virtual void EnableFlushCB(bool enable) override final;
	virtual void CallFlushCB() override final;

private:
	static bool CheckETC2Support();
	static bool CheckETC2SupportFast();
	static bool CheckETC2SupportSlow();

private:
	static const int MAX_TEXTURE_CHANNEL = 8;
	static const int MAX_RENDER_TARGET_LAYER = 8;

private:
	render* m_render;

	int m_cb_enable = 0;
	std::function<void(ur::RenderContext&)> m_flush_shader = nullptr;

	/************************************************************************/
	/* Texture                                                              */
	/************************************************************************/

	int m_textures[MAX_TEXTURE_CHANNEL];

	/************************************************************************/
	/* RenderTarget                                                         */
	/************************************************************************/

	int m_rt_depth;
	int m_rt_layers[MAX_RENDER_TARGET_LAYER];

//	int m_curr_rt;

	/************************************************************************/
	/* PixelBuffer                                                          */
	/************************************************************************/

	uint32_t m_pbo = 0;

	/************************************************************************/
	/* State                                                                */
	/************************************************************************/

	float        m_point_size = 1;
	float        m_line_width = 1;
	POLYGON_MODE m_poly_mode = POLYGON_FILL;
	bool         m_line_stripple = false;

	bool         m_blend;
	BLEND_FORMAT m_blend_src, m_blend_dst;
	BLEND_FUNC   m_blend_func;

	bool         m_depth;
	DEPTH_FORMAT m_depth_fmt;

	int          m_clear_mask;

	int          m_vp_x, m_vp_y, m_vp_w, m_vp_h;

	bool         m_scissor;
	int          m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h;

	int          m_cull = CULL_DISABLE;

	uint32_t     m_clear_color = 0;

}; // RenderContext

}
}

#endif // _UNIRENDER_GL_RENDER_CONTEXT_H_