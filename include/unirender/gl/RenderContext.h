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

	virtual int  CreateTexture(const void* pixels, int width, int height, int format,
        int mipmap_levels = 0, TEXTURE_WRAP wrap = TEXTURE_REPEAT, TEXTURE_FILTER filter = TEXTURE_LINEAR) override final;
	virtual int  CreateTexture3D(const void* pixels, int width, int height, int depth, int format) override final;
    virtual int  CreateTextureCube(int width, int height, int mipmap_levels = 0) override final;
	virtual int  CreateTextureID(int width, int height, int format, int mipmap_levels = 0) override final;
	virtual void ReleaseTexture(int id) override final;

	virtual void UpdateTexture(int tex_id, const void* pixels, int width, int height, int slice = 0,
        int miplevel = 0, TEXTURE_WRAP wrap = TEXTURE_REPEAT, TEXTURE_FILTER filter = TEXTURE_LINEAR) override final;
	virtual void UpdateTexture3d(int tex_id, const void* pixels, int width, int height, int depth) override final;
	virtual void UpdateSubTexture(const void* pixels, int x, int y, int w, int h, unsigned int id, int slice = 0, int miplevel = 0) override final;

	virtual void BindTexture(int id, int channel) override final;
    virtual const std::vector<int>& GetBindedTextures() const override final { return m_textures; }
    virtual int GetBindedTexture(TEXTURE_TYPE type, int channel) const override final;

	virtual void ClearTextureCache() override final;

	virtual int  GetCurrTexture() const override final;

    virtual void CopyTexture(int x, int y, size_t w, size_t h, int format, int tex) const override final;

	/************************************************************************/
	/* RenderTarget                                                         */
	/************************************************************************/

	virtual int  CreateRenderTarget(int id) override final;
	virtual void ReleaseRenderTarget(int id) override final;

	virtual void BindRenderTarget(int id) override final;
	virtual void UnbindRenderTarget() override final;
    virtual size_t GetRenderTargetDepth() const override final;

    // attach texture
    virtual void BindRenderTargetTex(int tex, ATTACHMENT_TYPE attachment = ATTACHMENT_COLOR0,
        TEXTURE_TARGET textarget = TEXTURE2D, int level = 0) override final;
    virtual void SetColorBufferList(const std::vector<ATTACHMENT_TYPE>& list) override final;

    // attach framebuffer
    virtual uint32_t CreateRenderbufferObject(uint32_t fbo, INTERNAL_FORMAT fmt,
        size_t width, size_t height) override final;
    virtual void ReleaseRenderbufferObject(uint32_t id) override final;
    virtual void BindRenderbufferObject(uint32_t rbo, ATTACHMENT_TYPE attachment) override final;

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

	virtual int  CreateShader(const char* vs, const char* fs, const std::vector<std::string>& textures, bool no_header = false) override final;
    virtual int  CreateShader(const char* cs) override final;
	virtual void ReleaseShader(int id) override final;

	virtual void BindShader(int id) override final;
    virtual int GetBindedShader() const override final;

	virtual int  GetShaderUniform(const char* name) override final;
	virtual void SetShaderUniform(int loc, UNIFORM_FORMAT format, const float* v, int n = 1) override final;

    virtual int GetComputeWorkGroupSize(int id) const override final;

	/************************************************************************/
	/* State                                                                */
	/************************************************************************/

	virtual void EnableBlend(bool blend) override final;
	virtual void SetBlend(int m1, int m2) override final;
    virtual void GetBlendFunc(int& m1, int& m2) const override final {
        m1 = m_blend_src;
        m2 = m_blend_dst;
    }
	virtual void SetBlendEquation(int func) override final;
    virtual int  GetBlendEquation() const override final { return m_blend_func; }
	virtual void SetDefaultBlend() override final;

	virtual void SetAlphaTest(ALPHA_FUNC func, float ref = 0) override final;
    virtual void GetAlphaTest(ALPHA_FUNC& func, float& ref) const override final {
        func = m_alpha_func;
        ref = m_alpha_ref;
    }

	virtual void SetZWrite(bool enable) override final;
    virtual bool GetZWrite() const override final { return m_zwrite; }
	virtual void SetZTest(DEPTH_FORMAT depth) override final;
    virtual DEPTH_FORMAT GetZTest() const override final { return m_ztest; }

	virtual void SetFrontFace(bool clockwise) override final;
    virtual bool GetFrontFace() const override final { return m_front_face_clockwise; }
	virtual void SetCullMode(CULL_MODE cull) override final;
    virtual CULL_MODE GetCullMode() const override final { return static_cast<CULL_MODE>(m_cull); }

    virtual int  GetBindedVertexLayoutID() override final;

	virtual void SetClearFlag(int flag) override final;
    virtual int GetClearFlag() const override final { return m_clear_mask; }
	virtual void SetClearColor(uint32_t argb) override final;
    virtual uint32_t GetClearColor() const override final { return m_clear_color; }
	virtual void Clear() override final;

	virtual void EnableScissor(int enable) override final;
	virtual void SetScissor(int x, int y, int width, int height) override final;

	virtual void SetViewport(int x, int y, int w, int h) override final;
	virtual void GetViewport(int& x, int& y, int& w, int& h) override final;

	virtual bool IsTexture(int id) const override final;

	virtual bool OutOfMemory() const override final;
	virtual void CheckError() const override final;

	virtual void SetPointSize(float size) override final;
    virtual float GetPointSize() const override final { return m_point_size; }
	virtual void SetLineWidth(float size) override final;
    virtual float GetLineWidth() const override final { return m_line_width; }

	virtual void SetPolygonMode(POLYGON_MODE poly_mode) override final;
    virtual POLYGON_MODE GetPolygonMode() const { return m_poly_mode; }

	virtual void EnableLineStripple(bool stripple) override final;
	virtual void SetLineStripple(int pattern) override final;

	virtual void SetUnpackRowLength(int len) override final;

	/************************************************************************/
	/* Draw                                                                 */
	/************************************************************************/

	virtual void DrawElements(DRAW_MODE mode, int fromidx, int ni, bool type_short = true) override final;
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
    virtual int  GetVertexLayout() const override final;
	virtual void UpdateVertexLayout(const CU_VEC<VertexAttrib>& va_list) override final;

	virtual void CreateVAO(const VertexInfo& vi, unsigned int& vao, unsigned int& vbo, unsigned int& ebo) override final;
	virtual void ReleaseVAO(unsigned int vao, unsigned int vbo, unsigned int ebo) override final;
	virtual void DrawElementsVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao, bool type_short = true) override final;
	virtual void DrawArraysVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao) override final;

    virtual void RenderCube(VertLayout layout) override final;
    virtual void RenderQuad(VertLayout layout) override final;

    /************************************************************************/
    /* Compute                                                              */
    /************************************************************************/

    virtual uint32_t CreateComputeBuffer(const std::vector<float>& buf, size_t index) const override final;
    virtual uint32_t CreateComputeBuffer(const std::vector<int>& buf, size_t index) const override final;
    virtual void     ReleaseComputeBuffer(uint32_t id) const override final;
    virtual void DispatchCompute(int thread_group_count) const override final;
    virtual void GetComputeBufferData(uint32_t id, std::vector<float>& result) const override final;

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
    struct VertBuf
    {
        ~VertBuf();

        bool IsValid() const { return vao != 0 && vbo != 0; }

        unsigned int vao = 0;
        unsigned int vbo = 0;
    };

private:
	render* m_render;

	int m_cb_enable = 0;
	std::function<void(ur::RenderContext&)> m_flush_shader = nullptr;

	/************************************************************************/
	/* Texture                                                              */
	/************************************************************************/

	std::vector<int> m_textures;

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

	ALPHA_FUNC   m_alpha_func;
	float        m_alpha_ref;

	bool         m_zwrite;
	DEPTH_FORMAT m_ztest;

    bool         m_front_face_clockwise = false;

	int          m_clear_mask;

	int          m_vp_x, m_vp_y, m_vp_w, m_vp_h;

	bool         m_scissor;
	int          m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h;

	int          m_cull = CULL_DISABLE;

	uint32_t     m_clear_color = 0;

    int          m_binded_shader = 0;
    int          m_binded_vertexlayout = 0;

    /************************************************************************/
    /* Draw                                                                 */
    /************************************************************************/

    VertBuf m_cached_cube[VL_MAX_COUNT];
    VertBuf m_cached_quad[VL_MAX_COUNT];

}; // RenderContext

}
}

#endif // _UNIRENDER_GL_RENDER_CONTEXT_H_