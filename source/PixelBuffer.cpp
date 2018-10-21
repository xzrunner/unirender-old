// use some code from: android/hwui/hwui/PixelBuffer.cpp

#include "unirender/PixelBuffer.h"
#include "unirender/RenderContext.h"
#include "unirender/Utility.h"
#include "unirender/Texture.h"

namespace
{

class CpuPixelBuffer : public ur::PixelBuffer
{
public:
	CpuPixelBuffer(ur::RenderContext* rc, size_t width, size_t height, ur::TEXTURE_FORMAT format)
		: PixelBuffer(width, height, format)
		, m_rc(rc)
		, m_buf(new uint8_t[CalcSize()])
	{
	}
	virtual ~CpuPixelBuffer() {}

	virtual uint8_t* Map(ur::ACCESS_MODE mode = ur::WRITE_ONLY) override
	{
		if (m_access_mode < 0) {
			m_access_mode = mode;
		}
		return m_buf.get();
	}

	virtual void Unmap() override
	{
		m_access_mode = -1;
	}

	virtual uint8_t* GetMappedPointer() const override
	{
		return m_access_mode < 0 ? nullptr : m_buf.get();
	}

	virtual void Upload(uint32_t x, uint32_t y, uint32_t width, uint32_t height, int offset, uint32_t tex_id) override
	{
		m_rc->UpdateSubTexture(&m_buf[offset], x, y, width, height, tex_id);
	}

	void PixelBuffer::Clear()
	{
		memset(m_buf.get(), 0, CalcSize());
	}

private:
	ur::RenderContext* m_rc;

	std::unique_ptr<uint8_t[]> m_buf;

}; // CpuPixelBuffer

class GpuPixelBuffer : public ur::PixelBuffer
{
public:
	GpuPixelBuffer(ur::RenderContext* rc, size_t width, size_t height, ur::TEXTURE_FORMAT format)
		: PixelBuffer(width, height, format)
		, m_rc(rc)
	{
		m_buf_id = m_rc->CreatePixelBuffer(0, width, height, format);
	}
	virtual ~GpuPixelBuffer()
	{
		m_rc->ReleasePixelBuffer(m_buf_id);
	}

	virtual uint8_t* Map(ur::ACCESS_MODE mode = ur::WRITE_ONLY) override
	{
		if (m_access_mode < 0)
		{
			m_rc->BindPixelBuffer(m_buf_id);
			m_mapped_ptr = (uint8_t*)m_rc->MapPixelBuffer(mode);
			m_access_mode = mode;
		}
		return m_mapped_ptr;
	}

	virtual void Unmap() override
	{
		if (m_access_mode >= 0)
		{
			if (m_mapped_ptr) {
				m_rc->BindPixelBuffer(m_buf_id);
				m_rc->UnmapPixelBuffer();
			}
			m_access_mode = -1;
			m_mapped_ptr = nullptr;
		}
	}

	virtual uint8_t* GetMappedPointer() const override
	{
		return m_mapped_ptr;
	}

	virtual void Upload(uint32_t x, uint32_t y, uint32_t width, uint32_t height, int offset, uint32_t tex_id) override
	{
		m_rc->BindPixelBuffer(m_buf_id);
		Unmap();
		m_rc->UpdateSubTexture(reinterpret_cast<void*>(offset), x, y, width, height, tex_id);
	}

	virtual void Clear() override
	{
		auto bmp_buf = Map(ur::WRITE_ONLY);
		if (bmp_buf) {
			memset(bmp_buf, 0, CalcSize());
		}
		m_rc->UnbindPixelBuffer();
	}

private:
	ur::RenderContext* m_rc;

	uint32_t m_buf_id;

	uint8_t* m_mapped_ptr = nullptr;

}; // GpuPixelBuffer

}

namespace ur
{

std::unique_ptr<PixelBuffer> PixelBuffer::Create(RenderContext* rc, size_t width, size_t height,
	                                             TEXTURE_FORMAT format, bool gpu)
{
	if (gpu) {
		return std::make_unique<GpuPixelBuffer>(rc, width, height, format);
	} else {
		return std::make_unique<CpuPixelBuffer>(rc, width, height, format);
	}
}

PixelBuffer::PixelBuffer(size_t width, size_t height, TEXTURE_FORMAT format)
	: m_width(width)
	, m_height(height)
	, m_format(format)
{
}

size_t PixelBuffer::CalcSize() const
{
	return ur::Utility::CalcTextureSize(m_format, m_width, m_height);
}

}