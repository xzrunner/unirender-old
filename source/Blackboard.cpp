#include "unirender/Blackboard.h"

#include <guard/check.h>

namespace ur
{

CU_SINGLETON_DEFINITION(Blackboard);

Blackboard::Blackboard()
{
}

RenderContext& Blackboard::GetRenderContext()
{
	GD_ASSERT(m_rc, "null rc");
	return *m_rc;
}

}