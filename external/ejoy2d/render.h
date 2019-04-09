#ifdef __cplusplus
extern "C"
{
#endif

#ifndef ejoy3d_render_h
#define ejoy3d_render_h

#include <stdint.h>

typedef unsigned int RID;

struct render;

struct render_init_args {
	int max_buffer;
	int max_layout;
	int max_target;
	int max_texture;
	int max_shader;
};

struct vertex_attrib {
	char name[16];
	int vbslot;
	int n;
	int size;
	int stride;
	int offset;
};

struct shader_init_args {
	const char * vs;
	const char * fs;
    int no_header;
	int texture;
	const char **texture_uniform;
};

enum EJ_RENDER_OBJ {
	EJ_INVALID = 0,
	EJ_VERTEXLAYOUT = 1,
	EJ_VERTEXBUFFER = 2,
	EJ_INDEXBUFFER = 3,
	EJ_TEXTURE = 4,
	EJ_TARGET = 5,
	EJ_SHADER = 6,
};

enum EJ_TEXTURE_TYPE {
	EJ_TEXTURE_2D = 0,
	EJ_TEXTURE_3D,
	EJ_TEXTURE_CUBE,
};

enum EJ_TEXTURE_FORMAT {
	EJ_TEXTURE_INVALID = 0,
	EJ_TEXTURE_RGBA8,
	EJ_TEXTURE_RGBA4,
	EJ_TEXTURE_RGB,
	EJ_TEXTURE_RGB565,
	EJ_TEXTURE_BGRA_EXT,
	EJ_TEXTURE_BGR_EXT,
    EJ_TEXTURE_RGB16F,
    EJ_TEXTURE_RGB32F,
    EJ_TEXTURE_RG16F,
	EJ_TEXTURE_A8,
	EJ_TEXTURE_DEPTH,	// use for render target
	EJ_TEXTURE_PVR2,
	EJ_TEXTURE_PVR4,
	EJ_TEXTURE_ETC1,
	EJ_TEXTURE_ETC2,
	EJ_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	EJ_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	EJ_COMPRESSED_RGBA_S3TC_DXT5_EXT,
};

enum EJ_BLEND_FORMAT {
	EJ_BLEND_DISABLE = 0,
	EJ_BLEND_ZERO,
	EJ_BLEND_ONE,
	EJ_BLEND_SRC_COLOR,
	EJ_BLEND_ONE_MINUS_SRC_COLOR,
	EJ_BLEND_SRC_ALPHA,
	EJ_BLEND_ONE_MINUS_SRC_ALPHA,
	EJ_BLEND_DST_ALPHA,
	EJ_BLEND_ONE_MINUS_DST_ALPHA,
	EJ_BLEND_DST_COLOR,
	EJ_BLEND_ONE_MINUS_DST_COLOR,
	EJ_BLEND_SRC_ALPHA_SATURATE,
};

enum EJ_BLEND_FUNC {
	EJ_BLEND_FUNC_ADD = 0,
	EJ_BLEND_FUNC_SUBTRACT,
	EJ_BLEND_FUNC_REVERSE_SUBTRACT,
    EJ_BLEND_MIN,
    EJ_BLEND_MAX,
};

enum EJ_ALPHA_FUNC {
	EJ_ALPHA_DISABLE = 0,
	EJ_ALPHA_NEVER,
	EJ_ALPHA_LESS,
	EJ_ALPHA_EQUAL,
	EJ_ALPHA_LEQUAL,
	EJ_ALPHA_GREATER,
	EJ_ALPHA_NOTEQUAL,
	EJ_ALPHA_GEQUAL,
	EJ_ALPHA_ALWAYS,
};

enum EJ_DEPTH_FORMAT {
	EJ_DEPTH_DISABLE = 0,
	EJ_DEPTH_LESS_EQUAL,
	EJ_DEPTH_LESS,
	EJ_DEPTH_EQUAL,
	EJ_DEPTH_GREATER,
	EJ_DEPTH_GREATER_EQUAL,
	EJ_DEPTH_ALWAYS,
    EJ_DEPTH_NOT_EQUAL,
    EJ_DEPTH_NEVER,
};

enum EJ_CLEAR_MASK {
	EJ_MASKC = 0x1,
	EJ_MASKD = 0x2,
	EJ_MASKS = 0x4,
};

enum EJ_UNIFORM_FORMAT {
	EJ_UNIFORM_INVALID = 0,
	EJ_UNIFORM_FLOAT1,
	EJ_UNIFORM_FLOAT2,
	EJ_UNIFORM_FLOAT3,
	EJ_UNIFORM_FLOAT4,
	EJ_UNIFORM_FLOAT33,
	EJ_UNIFORM_FLOAT44,
	EJ_UNIFORM_INT1,
	EJ_UNIFORM_MULTI_FLOAT44,
};

enum EJ_DRAW_MODE {
	EJ_DRAW_POINTS = 0,
	EJ_DRAW_LINES,
	EJ_DRAW_LINE_LOOP,
	EJ_DRAW_LINE_STRIP,
	EJ_DRAW_TRIANGLES,
	EJ_DRAW_TRIANGLE_STRIP,
	EJ_DRAW_TRIANGLE_FAN,
};

enum EJ_CULL_MODE {
	EJ_CULL_DISABLE = 0,
	EJ_CULL_FRONT,
	EJ_CULL_BACK,
    EJ_CULL_FRONT_AND_BACK,
};

#define EJ_TEXTURE_FILTER_NEAREST 1
#define EJ_TEXTURE_WARP_REPEAT    2
#define EJ_TEXTURE_WARP_BORDER    4

int render_version(struct render *R);
int render_size(struct render_init_args *args);
struct render * render_init(struct render_init_args *args, void * buffer, int sz);
void render_exit(struct render * R);

void render_set(struct render *R, enum EJ_RENDER_OBJ what, RID id, int slot);
RID render_get(struct render *R, enum EJ_RENDER_OBJ what, int slot);
void render_release(struct render *R, enum EJ_RENDER_OBJ what, RID id);

RID render_register_vertexlayout(struct render *R, int n, struct vertex_attrib * attrib);
void render_update_vertexlayout(struct render *R, int n, struct vertex_attrib * attrib);
RID render_get_binded_vertexlayout(struct render *R);

// what should be EJ_VERTEXBUFFER or EJ_INDEXBUFFER
RID render_buffer_create(struct render *R, enum EJ_RENDER_OBJ what, const void* data, int size);
void render_buffer_update(struct render *R, RID id, const void* data, int size);

RID render_texture_create(struct render *R, int width, int height, int depth, enum EJ_TEXTURE_FORMAT format, enum EJ_TEXTURE_TYPE type, int mipmap_levels);
void render_texture_update(struct render *R, RID id, int width, int height, int depth, const void *pixels, int slice, int miplevel, int flags);
// subupdate only support slice 0, miplevel 0
void render_texture_subupdate(struct render *R, RID id, const void *pixels, int x, int y, int w, int h, int slice, int miplevel);

RID render_target_create(struct render *R, int width, int height, enum EJ_TEXTURE_FORMAT format);
// render_release EJ_TARGET would not release the texture attachment
RID render_target_texture(struct render *R, RID rt);

RID render_shader_create(struct render *R, struct shader_init_args *args);
void render_shader_bind(struct render *R, RID id);
int render_shader_locuniform(struct render *R, const char * name);
void render_shader_setuniform(struct render *R, int loc, enum EJ_UNIFORM_FORMAT format, const float *v, int n);

void render_setviewport(int x, int y, int width, int height );
void render_setscissor(struct render *R, int x, int y, int width, int height );

void render_set_blendfunc(struct render *R, enum EJ_BLEND_FORMAT src, enum EJ_BLEND_FORMAT dst);
void render_set_blendeq(struct render *R, enum EJ_BLEND_FUNC eq);
void render_set_alpha_test(struct render *R, enum EJ_ALPHA_FUNC func, float ref);
void render_setdepth(struct render *R, enum EJ_DEPTH_FORMAT d);
void render_set_front_face(struct render *R, int clockwise);
void render_setcull(struct render *R, enum EJ_CULL_MODE c);
void render_enabledepthmask(struct render *R, int enable);
void render_enablescissor(struct render *R, int enable);

void render_state_reset(struct render *R);

void render_clear(struct render *R, enum EJ_CLEAR_MASK mask, unsigned long argb);
void render_draw_elements(struct render *R, enum EJ_DRAW_MODE mode, int fromidx, int ni);
void render_draw_elements_vao(struct render *R, enum EJ_DRAW_MODE mode, int fromidx, int ni, unsigned int vao);
void render_draw_elements_no_buf(struct render *R, enum EJ_DRAW_MODE mode, int size, unsigned int* indices);
void render_draw_arrays(struct render *R, enum EJ_DRAW_MODE mode, int fromidx, int ni);
void render_draw_arrays_vao(struct render *R, enum EJ_DRAW_MODE mode, int fromidx, int ni, unsigned int vao);

// todo
int render_get_texture_gl_id(struct render *R, RID id);

int render_query_target();

void render_clear_texture_cache(struct render* R);

#endif

#ifdef __cplusplus
}
#endif