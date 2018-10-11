#pragma once

#include "unirender/typedef.h"

#include <cu/uncopyable.h>

#include <cstdint>
#include <memory>

namespace ur
{

class RenderContext;

class PixelBuffer : private cu::Uncopyable
{
public:
	static std::unique_ptr<PixelBuffer> Create(RenderContext* rc, 
		size_t width, size_t height, TEXTURE_FORMAT format, bool gpu = true);
	virtual ~PixelBuffer() {}

	virtual uint8_t* Map(ACCESS_MODE mode = WRITE_ONLY) = 0;
	virtual void Unmap() = 0;

	virtual uint8_t* GetMappedPointer() const = 0;

	virtual void Upload(uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
		int offset, uint32_t tex_id) = 0;

	virtual void Clear() = 0;

protected:
	PixelBuffer(size_t width, size_t height, TEXTURE_FORMAT format);

	size_t CalcSize() const;

protected:
	int m_access_mode = -1;

private:
	size_t m_width;
	size_t m_height;

	TEXTURE_FORMAT m_format;

}; // PixelBuffer

}