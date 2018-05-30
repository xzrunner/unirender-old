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
	RenderContext(int max_texture);
	virtual ~RenderContext();

	virtual int RenderVersion() const override final;

	/************************************************************************/
	/* Texture                                                              */
	/************************************************************************/

	virtual int  CreateTexture(const void* pixels, int width, int height, int format) override final;
	virtual int  CreateTextureID(int width, int height, int format) override final;
	virtual void ReleaseTexture(int id) override final;

	virtual void UpdateTexture(int tex_id, const void* pixels, int width, int height) override final;
	virtual void UpdateSubTexture(const void* pixels, int x, int y, int w, int h, unsigned int id) override final;

	virtual void BindTexture(int id, int channel) override final;

	virtual void ClearTextureCache() override final;

	virtual int  GetCurrTexture() const override final;

	/************************************************************************/
	/* RenderTarget                                                         */
	/************************************************************************/

	virtual int  CreateRenderTarget(int id) override final;
	virtual void ReleaseRenderTarget(int id) override final;

	virtual void BindRenderTarget(int id) override final;
	virtual void BindRenderTargetTex(int tex_id) override final;
	virtual void UnbindRenderTarget() override final;

	virtual int  CheckRenderTargetStatus() override final;

// 	virtual void SetCurrRenderTarget(int id) override final;
// 	virtual int  GetCurrRenderTarget() const override final;

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

	virtual void EnableDepth(bool depth) override final;
	virtual void SetDepthFormat(DEPTH_FORMAT fmt) override final;

	virtual void SetClearFlag(int flag) override final;
	virtual void Clear(unsigned long argb) override final;

	virtual void EnableScissor(int enable) override final;
	virtual void SetScissor(int x, int y, int width, int height) override final;

	virtual void SetViewport(int x, int y, int w, int h) override final;
	virtual void GetViewport(int& x, int& y, int& w, int& h) override final;

	virtual bool IsTexture(int id) const override final;

	virtual bool OutOfMemory() const override final;
	virtual void CheckError() const override final;

	virtual void SetPointSize(float size) override final;
	virtual void SetLineWidth(float size) override final;

	virtual void EnableLineStripple(bool stripple) override final;
	virtual void SetLineStripple(int pattern) override final;

	/************************************************************************/
	/* Draw                                                                 */
	/************************************************************************/

	virtual void DrawElements(DRAW_MODE mode, int fromidx, int ni) override final;
	virtual void DrawArrays(DRAW_MODE mode, int fromidx, int ni) override final;

	virtual int  CreateBuffer(RENDER_OBJ what, const void *data, int n, int stride) override final;
	virtual void ReleaseBuffer(RENDER_OBJ what, int id) override final;
	virtual void BindBuffer(RENDER_OBJ what, int id) override final;
	virtual void UpdateBuffer(int id, const void* data, int n) override final;

	virtual int  CreateVertexLayout(const CU_VEC<VertexAttrib>& va_list) override final;
	virtual void ReleaseVertexLayout(int id) override final;
	virtual void BindVertexLayout(int id) override final;

	virtual void CreateVAO(const VertexInfo& vi, unsigned int& vao, unsigned int& vbo, unsigned int& ebo) override final;
	virtual void ReleaseVAO(unsigned int vao, unsigned int vbo, unsigned int ebo) override final;
	virtual void DrawElementsVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao) override final;

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

public:
	struct Callback
	{
		std::function<void()> flush_shader;
		std::function<void()> flush_render_shader;
	};

	void RegistCB(const RenderContext::Callback& cb) { m_cb = cb; }

private:
	static bool CheckETC2Support();
	static bool CheckETC2SupportFast();
	static bool CheckETC2SupportSlow();

private:
	static const int MAX_TEXTURE_CHANNEL = 8;
	static const int MAX_RENDER_TARGET_LAYER = 8;

private:
	render* m_render;
	Callback m_cb;

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
	/* State                                                                */
	/************************************************************************/

	bool         m_blend;
	BLEND_FORMAT m_blend_src, m_blend_dst;
	BLEND_FUNC   m_blend_func;

	bool         m_depth;
	DEPTH_FORMAT m_depth_fmt;

	int          m_clear_mask;

	int          m_vp_x, m_vp_y, m_vp_w, m_vp_h;

	bool         m_scissor;
	int          m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h;

}; // RenderContext

}
}

#endif // _UNIRENDER_GL_RENDER_CONTEXT_H_