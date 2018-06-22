#include "unirender/Utility.h"
#include "unirender/typedef.h"

namespace ur
{

int Utility::CalcTextureSize(int format, int width, int height)
{
	switch (format) {
	case TEXTURE_RGBA8:
	case TEXTURE_BGRA_EXT:
		return width * height * 4;
	case TEXTURE_RGB565:
	case TEXTURE_RGBA4:
		return width * height * 2;
	case TEXTURE_RGB:
	case TEXTURE_BGR_EXT:
		return width * height * 3;
	case TEXTURE_A8:
	case TEXTURE_DEPTH:
		return width * height;
	case TEXTURE_PVR2:
		return width * height / 4;
	case TEXTURE_PVR4:
	case TEXTURE_ETC1:
		return width * height / 2;
	case TEXTURE_ETC2:
		return width * height;
	case TEXTURE_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return width * height / 2;
	case TEXTURE_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case TEXTURE_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return width * height;
	default:
		return 0;
	}
}

}