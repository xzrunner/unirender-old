#include "unirender/Utility.h"
#include "unirender/typedef.h"

namespace ur
{

int Utility::CalcTextureSize(int format, int width, int height, int depth)
{
	float times = 0;

	switch (format) {
	case TEXTURE_RGBA8:
	case TEXTURE_BGRA_EXT:
		times = 4;
		break;
	case TEXTURE_RGB565:
	case TEXTURE_RGBA4:
		times = 2;
		break;
	case TEXTURE_RGB:
	case TEXTURE_BGR_EXT:
		times = 3;
		break;
    case TEXTURE_RGBA16F:
        times = 8;
        break;
    case TEXTURE_RGB16F:
        times = 6;
        break;
    case TEXTURE_RGB32F:
        times = 12;
        break;
    case TEXTURE_RG16F:
        times = 4;
        break;
	case TEXTURE_A8:
    case TEXTURE_RED:
	case TEXTURE_DEPTH:
		times = 1;
		break;
	case TEXTURE_PVR2:
		times = 1.0f / 4;
		break;
	case TEXTURE_PVR4:
	case TEXTURE_ETC1:
		times = 1.0f / 2;
		break;
	case TEXTURE_ETC2:
		times = 1;
		break;
	case TEXTURE_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		times = 1.0f / 2;
		break;
	case TEXTURE_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case TEXTURE_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		times = 1;
		break;
	default:
		times = 0;
	}

	auto sz = depth == 0 ? width * height * times : width * height * depth * times;
	return static_cast<int>(sz);
}

}