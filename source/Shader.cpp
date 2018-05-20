#include "unirender/Shader.h"
#include "unirender/RenderContext.h"

#include <string>
#include <fstream>
#include <sstream>

namespace ur
{

Shader::Shader(RenderContext* rc, const char* vertex_path, const char* fragment_path)
	: m_rc(rc)
	, m_id(-1)
{
	std::ifstream fvs(vertex_path);
	std::ifstream ffs(fragment_path);
	if (!fvs.fail() && !ffs.fail())
	{
		std::stringstream svs, sfs;
		svs << fvs.rdbuf();
		sfs << ffs.rdbuf();
		fvs.close();
		ffs.close();

		std::string vs = svs.str(), fs = sfs.str();
		m_id = rc->CreateShader(vs.c_str(), fs.c_str());
	}
}

Shader::~Shader()
{
	if (m_id != -1) {
		m_rc->ReleaseShader(m_id);
	}
}

void Shader::Use()
{
	if (m_id != -1) {
		m_rc->BindShader(m_id);
	}
}

void Shader::SetInt(const std::string& name, int value) const
{
	if (m_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_INT1, &static_cast<float>(value));
	}
}

void Shader::SetFloat(const std::string& name, float value) const
{
	if (m_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT1, &value);
	}
}

void Shader::SetVec2(const std::string& name, const float value[2]) const
{
	if (m_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT2, value);
	}
}

void Shader::SetVec3(const std::string& name, const float value[3]) const
{
	if (m_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT3, value);
	}
}

void Shader::SetMat3(const std::string& name, const float value[9]) const
{
	if (m_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT33, value);
	}
}

void Shader::SetMat4(const std::string& name, const float value[16]) const
{
	if (m_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_FLOAT44, value);
	}
}

void Shader::SetMultiMat4(const std::string& name, const float* value, int n) const
{
	if (m_id != -1) {
		m_rc->SetShaderUniform(m_rc->GetShaderUniform(name.c_str()), UNIFORM_MULTI_FLOAT44, value, n);
	}
}

}