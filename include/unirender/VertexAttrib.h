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
	int tot_size;

	VertexAttrib() : n(0), size(0), tot_size(0) {}
	VertexAttrib(const CU_STR& name, int n, int size) {
		Assign(name, n, size);
	}
	void Assign(const CU_STR& name, int n, int size) {
		this->name = name;
		this->n = n;
		this->size = size;
		this->tot_size = size * n;
	}
};

}

#endif // _UNIRENDER_VERTEX_ATTRIB_H_