#pragma once

#include "unirender/Texture.h"

#include <cu/uncopyable.h>
#include <cu/cu_stl.h>

#include <string>
#include <vector>
#include <memory>

namespace ur
{

class RenderContext;
struct VertexAttrib;

class Shader : private cu::Uncopyable
{
public:
	Shader(RenderContext* rc, const char* vs, const char* fs,
		const std::vector<std::string>& textures, const CU_VEC<VertexAttrib>& va_list);
	virtual ~Shader();

	virtual ur::DRAW_MODE GetDrawMode() const { return ur::DRAW_TRIANGLES; }
	virtual void DrawBefore(const ur::TexturePtr& tex) {}
	virtual void DrawAfter() {}

	void Use();

	// todo
	void SetUsedTextures(const std::vector<uint32_t>& textures) {
		m_textures = textures;
	}

	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;
	void SetVec2(const std::string& name, const float value[2]) const;
	void SetVec3(const std::string& name, const float value[3]) const;
	void SetVec4(const std::string& name, const float value[4]) const;
	void SetMat3(const std::string& name, const float value[9]) const;
	void SetMat4(const std::string& name, const float value[16]) const;
	void SetMultiMat4(const std::string& name, const float* value, int n) const;

private:
	RenderContext* m_rc;

	int m_shader_id;
	int m_vert_layout_id;

	// todo
	std::vector<uint32_t> m_textures;

}; // Shader

std::unique_ptr<Shader> CreateShaderFromFile(RenderContext* rc, const char* vs_filepath,
	const char* fs_filepath, const std::vector<std::string>& textures, const CU_VEC<VertexAttrib>& va_list);

}