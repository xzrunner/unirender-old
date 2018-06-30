#ifndef _UNIRENDER_VERTEX_ATTRIB_H_
#define _UNIRENDER_VERTEX_ATTRIB_H_

#include <cu/cu_stl.h>

namespace ur
{

struct VertexAttrib
{
	CU_STR name;
	int n;
	int size;
	int stride;
	int offset;

	VertexAttrib() : n(0), size(0), stride(0), offset(0) {}
	VertexAttrib(const CU_STR& name, int n, int size, int stride, int offset) {
		Assign(name, n, size, stride, offset);
	}
	void Assign(const CU_STR& name, int n, int size, int stride, int offset) {
		this->name = name;
		this->n = n;
		this->size = size;
		this->stride = stride;
		this->offset = offset;
	}
};

}

#endif // _UNIRENDER_VERTEX_ATTRIB_H_