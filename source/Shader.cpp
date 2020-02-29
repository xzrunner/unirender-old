#include "unirender/Shader.h"
#include "unirender/RenderContext.h"

#include <string>
#include <fstream>
#include <sstream>

namespace ur
{

Shader::Shader(RenderContext* rc, const char* vs, const char* fs,
	           const std::vector<std::string>& textures,
	           const CU_VEC<VertexAttrib>& va_list, bool no_header)
	: m_rc(rc)
	, m_shader_id(-1)
	, m_vert_layout_id(-1)
{
    auto old_id = rc->GetVertexLayout();

	m_vert_layout_id = rc->CreateVertexLayout(va_list);
	m_shader_id = rc->CreateShader(vs, fs, textures, no_header);

    rc->BindVertexLayout(old_id);
}

Shader::Shader(RenderContext* rc, const char* cs)
    : m_rc(rc)
    , m_shader_id(-1)
    , m_vert_layout_id(-1)
{
    m_shader_id = rc->CreateShader(cs);
}

Shader::~Shader()
{
	if (m_shader_id != -1) {
		m_rc->ReleaseShader(m_shader_id);
	}
	if (m_vert_layout_id != -1) {
		m_rc->ReleaseVertexLayout(m_vert_layout_id);
	}
}

void Shader::Use()
{
	m_rc->CallFlushCB();

	for (int i = 0, n = m_textures.size(); i < n; ++i) {
		m_rc->BindTexture(m_textures[i], i);
	}

	if (m_shader_id != -1) {
		m_rc->BindShader(m_shader_id);
		m_rc->BindVertexLayout(m_vert_layout_id);
	}
}

void Shader::SetInt(const std::string& name, int value) const
{
	if (m_shader_id != -1) {
		float fval = static_cast<float>(value);
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_INT1, &fval);
	}
}

void Shader::SetFloat(const std::string& name, float value) const
{
	if (m_shader_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT1, &value);
	}
}

void Shader::SetVec2(const std::string& name, const float value[2]) const
{
	if (m_shader_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT2, value);
	}
}

void Shader::SetVec3(const std::string& name, const float value[3]) const
{
	if (m_shader_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT3, value);
	}
}

void Shader::SetVec4(const std::string& name, const float value[4]) const
{
	if (m_shader_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT4, value);
	}
}

void Shader::SetMat3(const std::string& name, const float value[9]) const
{
	if (m_shader_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_MATRIX3, value);
	}
}

void Shader::SetMat4(const std::string& name, const float value[16]) const
{
	if (m_shader_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_MATRIX4, value);
	}
}

void Shader::SetVec3Array(const std::string& name, const float* value, int n) const
{
    if (m_shader_id != -1) {
        m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT3_ARRAY, value, n);
    }
}

void Shader::SetVec4Array(const std::string& name, const float* value, int n) const
{
    if (m_shader_id != -1) {
        m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT4_ARRAY, value, n);
    }
}

void Shader::SetMat4Array(const std::string& name, const float* value, int n) const
{
	if (m_shader_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_MATRIX4_ARRAY, value, n);
	}
}

int Shader::GetComputeWorkGroupSize() const
{
    if (m_shader_id != -1) {
        return m_rc->GetComputeWorkGroupSize(m_shader_id);
    } else {
        return 0;
    }
}

std::unique_ptr<Shader> CreateShaderFromFile(RenderContext* rc, const char* vs_filepath, const char* fs_filepath,
	                                         const std::vector<std::string>& textures, const CU_VEC<VertexAttrib>& va_list)
{
	std::ifstream fvs(vs_filepath);
	std::ifstream ffs(fs_filepath);
	if (fvs.fail() || ffs.fail()) {
		return nullptr;
	}

	std::stringstream svs, sfs;
	svs << fvs.rdbuf();
	sfs << ffs.rdbuf();
	fvs.close();
	ffs.close();

	std::string vs = svs.str(), fs = sfs.str();
	return std::make_unique<Shader>(rc, vs.c_str(), fs.c_str(), textures, va_list);
}

}