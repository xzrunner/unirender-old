#pragma once

#include <cu/uncopyable.h>

#include <string>

namespace ur
{

class RenderContext;

class Shader : private cu::Uncopyable
{
public:
	Shader(RenderContext* rc, const char* vertex_path, const char* fragment_path);
	~Shader();

	void Use();

	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;
	void SetVec2(const std::string& name, const float value[2]) const;
	void SetVec3(const std::string& name, const float value[3]) const;
	void SetMat3(const std::string& name, const float value[9]) const;
	void SetMat4(const std::string& name, const float value[16]) const;
	void SetMultiMat4(const std::string& name, const float* value, int n) const;

private:
	RenderContext* m_rc;

	int m_id;

}; // Shader

}