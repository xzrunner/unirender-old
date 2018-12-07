#ifndef _UNIRENDER_RENDER_CONTEXT_H_
#define _UNIRENDER_RENDER_CONTEXT_H_

#include "unirender/VertexAttrib.h"
#include "unirender/typedef.h"

#include <cu/cu_stl.h>

namespace ur
{

class RenderContext
{
public:
	struct VertexInfo
	{
		BUFFER_USAGE          vert_usage = USAGE_STATIC;
		const void*           vertices = nullptr;
		size_t                vn = 0;
		size_t                stride = 0;

		BUFFER_USAGE          index_usage = USAGE_STATIC;
		const unsigned short* indices = nullptr;
		size_t                in = 0;

		std::vector<VertexAttrib> va_list;
	};

public:
	RenderContext() {}
	virtual ~RenderContext() {}

	virtual int RenderVersion() const = 0;

	/************************************************************************/
	/* Texture                                                              */
	/************************************************************************/

	virtual int  CreateTexture(const void* pixels, int width, int height, int format, int mipmap_levels = 0, int linear = 1) = 0;
	virtual int  CreateTexture3D(const void* pixels, int width, int height, int depth, int format) = 0;
	virtual int  CreateTextureID(int width, int height, int format, int mipmap_levels = 0) = 0;
	virtual void ReleaseTexture(int id) = 0;

	virtual void UpdateTexture(int tex_id, const void* pixels, int width, int height, int slice = 0, int miplevel = 0, int flags = 0) = 0;
	virtual void UpdateTexture3d(int tex_id, const void* pixels, int width, int height, int depth) = 0;
	virtual void UpdateSubTexture(const void* pixels, int x, int y, int w, int h, unsigned int id, int slice = 0, int miplevel = 0) = 0;

	virtual void BindTexture(int id, int channel) = 0;

	virtual void ClearTextureCache() = 0;

	virtual int  GetCurrTexture() const = 0;

	/************************************************************************/
	/* RenderTarget                                                         */
	/************************************************************************/
	virtual int  CreateRenderTarget(int id) = 0;
	virtual void ReleaseRenderTarget(int id) = 0;

	virtual void BindRenderTarget(int id) = 0;
	virtual void BindRenderTargetTex(int color_tex, int depth_tex = -1) = 0;
	virtual void UnbindRenderTarget() = 0;

	virtual int  CheckRenderTargetStatus() = 0;

// 	virtual void SetCurrRenderTarget(int id) = 0;
// 	virtual int  GetCurrRenderTarget() const = 0;

	/************************************************************************/
	/* PixelBuffer                                                          */
	/************************************************************************/

	virtual int  CreatePixelBuffer(uint32_t id, int width, int height, int format) = 0;
	virtual void ReleasePixelBuffer(uint32_t id) = 0;

	virtual void BindPixelBuffer(uint32_t id) = 0;
	virtual void UnbindPixelBuffer() = 0;

	virtual void* MapPixelBuffer(ACCESS_MODE mode) = 0;
	virtual void  UnmapPixelBuffer() = 0;

	/************************************************************************/
	/* Shader                                                               */
	/************************************************************************/

	virtual int  CreateShader(const char* vs, const char* fs, const std::vector<std::string>& textures) = 0;
	virtual void ReleaseShader(int id) = 0;

	virtual void BindShader(int id) = 0;

	virtual int  GetShaderUniform(const char* name) = 0;
	virtual void SetShaderUniform(int loc, UNIFORM_FORMAT format, const float* v, int n = 1) = 0;

	/************************************************************************/
	/* State                                                                */
	/************************************************************************/

	virtual void EnableBlend(bool blend) = 0;
	virtual void SetBlend(int m1, int m2) = 0;
	virtual void SetBlendEquation(int func) = 0;
	virtual void SetDefaultBlend() = 0;

	virtual void EnableDepthMask(bool depth) = 0;
	virtual void SetDepthTest(DEPTH_FORMAT fmt) = 0;

	virtual void SetFrontFace(bool clockwise) = 0;
	virtual void SetCull(CULL_MODE cull) = 0;

	virtual void SetClearFlag(int flag) = 0;
	virtual void SetClearColor(uint32_t arbg) = 0;
	virtual void Clear() = 0;

	virtual void EnableScissor(int enable) = 0;
	virtual void SetScissor(int x, int y, int width, int height) = 0;

	virtual void SetViewport(int x, int y, int w, int h) = 0;
	virtual void GetViewport(int& x, int& y, int& w, int& h) = 0;

	virtual bool IsTexture(int id) const = 0;

	virtual bool OutOfMemory() const = 0;
	virtual void CheckError() const = 0;

	virtual void SetPointSize(float size) = 0;
	virtual void SetLineWidth(float size) = 0;

	virtual void SetPolygonMode(POLYGON_MODE poly_mode) = 0;

	virtual void EnableLineStripple(bool stripple) = 0;
	virtual void SetLineStripple(int pattern) = 0;

	virtual void SetUnpackRowLength(int len) = 0;

	/************************************************************************/
	/* Draw                                                                 */
	/************************************************************************/

	virtual void DrawElements(DRAW_MODE mode, int fromidx, int ni) = 0;
	virtual void DrawElements(DRAW_MODE mode, int count, unsigned int* indices) = 0;
	virtual void DrawArrays(DRAW_MODE mode, int fromidx, int ni) = 0;

	virtual int  CreateBuffer(RENDER_OBJ what, const void *data, int size) = 0;
	virtual void ReleaseBuffer(RENDER_OBJ what, int id) = 0;
	virtual void BindBuffer(RENDER_OBJ what, int id) = 0;
	virtual void UpdateBuffer(int id, const void* data, int size) = 0;
	virtual void UpdateBufferRaw(BUFFER_TYPE type, int id, const void* data, int size, int offset = 0) = 0;

	virtual int  CreateVertexLayout(const CU_VEC<VertexAttrib>& va_list) = 0;
	virtual void ReleaseVertexLayout(int id) = 0;
	virtual void BindVertexLayout(int id) = 0;
	virtual void UpdateVertexLayout(const CU_VEC<VertexAttrib>& va_list) = 0;

	virtual void CreateVAO(const VertexInfo& vi, unsigned int& vao, unsigned int& vbo, unsigned int& ebo) = 0;
	virtual void ReleaseVAO(unsigned int vao, unsigned int vbo, unsigned int ebo) = 0;
	virtual void DrawElementsVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao) = 0;
	virtual void DrawArraysVAO(DRAW_MODE mode, int fromidx, int ni, unsigned int vao) = 0;

	/************************************************************************/
	/* Debug                                                                */
	/************************************************************************/

	virtual int  GetRealTexID(int id) = 0;

	/************************************************************************/
	/* Other                                                                */
	/************************************************************************/

	virtual void ReadBuffer() = 0;
	virtual void ReadPixels(const void* pixels, int channels, int x, int y, int w, int h) = 0;

	virtual bool CheckAvailableMemory(int need_texture_area) const = 0;

	virtual void EnableFlushCB(bool enable) = 0;
	virtual void CallFlushCB() = 0;

	static bool IsSupportETC2() { return m_etc2; }
	static void SetSupportETC2(bool support) { m_etc2 = support; }

protected:
	static bool m_etc2;

}; // RenderContext

}

#endif // _UNIRENDER_RENDER_CONTEXT_H_