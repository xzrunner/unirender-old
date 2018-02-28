#pragma once

#include <guard/check.h>
#include <cu/cu_macro.h>

#include <memory>

namespace ur
{

class RenderContext;

class Blackboard
{
public:
	void SetRenderContext(const std::shared_ptr<RenderContext>& rc) {
		m_rc = rc;
	}
	RenderContext& GetRenderContext() {
		GD_ASSERT(m_rc, "null rc");
		return *m_rc;
	}

private:
	std::shared_ptr<RenderContext> m_rc;

	CU_SINGLETON_DECLARATION(Blackboard);

}; // Blackboard

}