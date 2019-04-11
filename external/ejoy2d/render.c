#include "render.h"
#include "opengl.h"
#include "carray.h"
#include "block2.h"

#include <logger.h>

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#if !defined (VAO_DISABLE) && !defined (__ANDROID__)
// If your platform doesn't support VAO, comment it out.
// Or define VAO_DISABLE first
// fixme: some state changed not needed
#define VAO_ENABLE


#if defined (GL_OES_vertex_array_object)
	#define glBindVertexArray glBindVertexArrayOES
	#define glGenVertexArrays glGenVertexArraysOES
	#define glDeleteVertexArrays glDeleteVertexArraysOES
#endif

#endif

#define MAX_VB_SLOT			8
#define MAX_ATTRIB			16
#define MAX_TEXTURE			8

#define CHANGE_VERTEXARRAY	0x1
#define CHANGE_TEXTURE		0x2
#define CHANGE_BLEND_FUNC	0x4
#define CHANGE_BLEND_EQ		0x8
#define CHANGE_ALPHA		0x10
#define CHANGE_DEPTH		0x20
#define CHANGE_CULL			0x40
#define CHANGE_TARGET		0x80
#define CHANGE_SCISSOR		0x100

#ifdef NO_CHECK_GL_ERROR
	#define CHECK_GL_ERROR
#else
	//#define CHECK_GL_ERROR assert(check_opengl_error());
	#define CHECK_GL_ERROR check_opengl_error_debug((struct render *)R, __FILE__, __LINE__);
#endif

#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif // GL_COMPRESSED_RGBA8_ETC2_EAC
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT

struct buffer {
	GLuint glid;
	GLenum gltype;
};

struct attrib {
	int n;
	struct vertex_attrib a[MAX_ATTRIB];
};

struct target {
	GLuint glid;
	RID tex;
};

struct texture {
	GLuint glid;
	int width;
	int height;
	int depth;
	int mipmap_levels;
	enum EJ_TEXTURE_FORMAT format;
	enum EJ_TEXTURE_TYPE type;
	int memsize;
};

struct attrib_layout {
	int vbslot;
	GLint size;
 	GLenum type;
 	GLboolean normalized;
	int stride;
	int offset;
};

struct shader {
	GLuint glid;
#ifdef VAO_ENABLE
	GLuint glvao;
	RID vbslot[MAX_VB_SLOT];
	RID ib;
#endif
	int n;
	struct attrib_layout a[MAX_ATTRIB];
	int texture_n;
	int texture_uniform[MAX_TEXTURE];
};

struct rstate {
	RID target;
	enum EJ_BLEND_FORMAT blend_src;
	enum EJ_BLEND_FORMAT blend_dst;
	enum EJ_BLEND_FUNC blend_func;
	enum EJ_ALPHA_FUNC alpha_func;
	float alpha_ref;
	enum EJ_DEPTH_FORMAT depth;
	enum EJ_CULL_MODE cull;
	int depthmask;
	int scissor;
	int scissor_x, scissor_y, scissor_w, scissor_h;
	RID texture[MAX_TEXTURE];
};

struct render {
	uint32_t changeflag;
	RID attrib_layout;
	RID vbslot[MAX_VB_SLOT];
	RID indexbuffer;
	RID program;
	GLint default_framebuffer;
	struct rstate current;
	struct rstate last;
	struct logger log;
	struct array buffer;
	struct array attrib;
	struct array target;
	struct array texture;
	struct array shader;
};

static inline void
check_opengl_error_debug(struct render *R, const char *filename, int line) {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR
//		&& error != GL_INVALID_ENUM
//		&& error != GL_INVALID_VALUE
//		&& error != GL_INVALID_OPERATION
//		&& error != GL_OUT_OF_MEMORY
//		&& error != GL_STACK_OVERFLOW
//		&& error != GL_STACK_UNDERFLOW
	) {
		logger_printf(&R->log, "GL_ERROR (0x%x) @ %s : %d\n", error, filename, line);
		exit(1);
	}
}

// what should be EJ_VERTEXBUFFER or EJ_INDEXBUFFER
RID
render_buffer_create(struct render *R, enum EJ_RENDER_OBJ what, const void *data, int size) {
#ifdef VAO_ENABLE
	glBindVertexArray(0);
#endif

	GLenum gltype;
	switch(what) {
	case EJ_VERTEXBUFFER:
		gltype = GL_ARRAY_BUFFER;
		break;
	case EJ_INDEXBUFFER:
		gltype = GL_ELEMENT_ARRAY_BUFFER;
		break;
	default:
		return 0;
	}
	struct buffer * buf = (struct buffer *)array_alloc(&R->buffer);
	if (buf == NULL)
		return 0;
	glGenBuffers(1, &buf->glid);
	glBindBuffer(gltype, buf->glid);
	if (data && size > 0) {
		glBufferData(gltype, size, data, GL_STATIC_DRAW);
	}
	buf->gltype = gltype;

	CHECK_GL_ERROR

	return array_id(&R->buffer, buf);
}

void
render_buffer_update(struct render *R, RID id, const void* data, int size) {
	struct buffer * buf = (struct buffer *)array_ref(&R->buffer, id);
#ifdef VAO_ENABLE
	glBindVertexArray(0);
#endif
	R->changeflag |= CHANGE_VERTEXARRAY;
	glBindBuffer(buf->gltype, buf->glid);
	glBufferData(buf->gltype, size, data, GL_DYNAMIC_DRAW);
	CHECK_GL_ERROR
}

static void
close_buffer(void *p, void *R) {
	struct buffer * buf = (struct buffer *)p;
	glDeleteBuffers(1,&buf->glid);

	CHECK_GL_ERROR
}

RID
render_register_vertexlayout(struct render *R, int n, struct vertex_attrib * attrib) {
	assert(n <= MAX_ATTRIB);
	struct attrib * a = (struct attrib*)array_alloc(&R->attrib);
	if (a == NULL) {
		return 0;
	}

	a->n = n;
	memcpy(a->a, attrib, n * sizeof(struct vertex_attrib));

	RID id = array_id(&R->attrib, a);

	R->attrib_layout = id;

	return id;
}

void
render_update_vertexlayout(struct render* R, int n, struct vertex_attrib* attrib) {
	struct attrib* a = (struct attrib *)array_ref(&R->attrib, R->attrib_layout);
	assert(a && a->n == n);
	for (int i = 0; i < a->n; ++i) {
		a->a[i] = attrib[i];
		strcpy(a->a[i].name, attrib[i].name);
	}
}

RID
render_get_binded_vertexlayout(struct render *R) {
    return R->attrib_layout;
}

static GLuint
compile(struct render *R, const char * source, int type, int no_header) {
	GLint status;

	GLuint shader = glCreateShader(type);

    if (no_header == 0)
    {
	    const GLchar* sources[3] = {
		    // Define GLSL version
    #if defined(GL_ES_VERSION_2_0) || defined(__MACOSX)
		    "#version 100\n"
    #else
		    "#version 120\n"
    #endif
		    ,
		    // GLES2 precision specifiers
    #if defined(GL_ES_VERSION_2_0) || defined(__MACOSX)
		    // Define default float precision for fragment shaders:
		    (type == GL_FRAGMENT_SHADER) ?
		    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
		    "precision highp float;           \n"
		    "#else                            \n"
		    "precision mediump float;         \n"
		    "#endif                           \n"
		    : ""
		    // Note: OpenGL ES automatically defines this:
		    // #define GL_ES
    #else
		    // Ignore GLES 2 precision specifiers:
		    "#define lowp   \n"
		    "#define mediump\n"
		    "#define highp  \n"
    #endif
		    ,
		    source
	    };

	    glShaderSource(shader, 3, sources, NULL);
    }
    else
    {
        glShaderSource(shader, 1, &source, NULL);
    }

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		char buf[1024];
		GLint len;
		glGetShaderInfoLog(shader, 1024, &len, buf);

		logger_printf(&R->log, "compile failed:%s\n"
			"source:\n %s\n",
			buf, source);
		glDeleteShader(shader);
		return 0;
	}

	CHECK_GL_ERROR

	return shader;
}

static int
link(struct render *R, GLuint prog) {
	GLint status;
	glLinkProgram(prog);

	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == 0) {
		char buf[1024];
		GLint len;
		glGetProgramInfoLog(prog, 1024, &len, buf);

		logger_printf(&R->log, "link failed:%s\n", buf);

		return 0;
	}

	CHECK_GL_ERROR

	return 1;
}

static int
compile_link(struct render *R, struct shader *s, const char * VS, const char *FS, int no_header) {
	GLuint fs = compile(R, FS, GL_FRAGMENT_SHADER, no_header);
	if (fs == 0) {
		logger_printf(&R->log, "Can't compile fragment shader\n");
		return 0;
	} else {
		glAttachShader(s->glid, fs);
	}

	GLuint vs = compile(R, VS, GL_VERTEX_SHADER, no_header);
	if (vs == 0) {
		logger_printf(&R->log, "Can't compile vertex shader");
		return 0;
	} else {
		glAttachShader(s->glid, vs);
	}

	if (R->attrib_layout == 0)
		return 0;

	struct attrib * a = (struct attrib *)array_ref(&R->attrib, R->attrib_layout);
	s->n = a->n;
	int i;
	for (i=0;i<a->n;i++) {
		struct vertex_attrib *va = &a->a[i];
		struct attrib_layout *al = &s->a[i];
		glBindAttribLocation(s->glid, i, va->name);
		al->vbslot = va->vbslot;
		al->size = va->n;
		al->stride = va->stride;
		al->offset = va->offset;
		switch (va->size) {
		case 1:
			al->type = GL_UNSIGNED_BYTE;
			al->normalized = GL_TRUE;
			break;
		case 2:
			al->type = GL_UNSIGNED_SHORT;
			al->normalized = GL_TRUE;
			break;
		case 4:
			al->type = GL_FLOAT;
			al->normalized = GL_FALSE;
			break;
		default:
			return 0;
		}
	}

	return link(R, s->glid);
}

RID
render_shader_create(struct render *R, struct shader_init_args *args) {
	struct shader * s = (struct shader *)array_alloc(&R->shader);
	if (s == NULL) {
		return 0;
	}
	s->glid = glCreateProgram();
	if (!compile_link(R, s, args->vs, args->fs, args->no_header)) {
		glDeleteProgram(s->glid);
		array_free(&R->shader, s);
		return 0;
	}

	s->texture_n = args->texture;
	int i;
	for (i=0;i<s->texture_n;i++) {
		s->texture_uniform[i] = glGetUniformLocation(s->glid, args->texture_uniform[i]);
	}

#ifdef VAO_ENABLE
	glGenVertexArrays(1, &s->glvao);
	for (i=0;i<MAX_VB_SLOT;i++) {
		s->vbslot[i] = 0;
	}
	s->ib = 0;
#endif

	CHECK_GL_ERROR

	return array_id(&R->shader, s);
}

static void
close_shader(void *p, void *R) {
	struct shader * shader = (struct shader *)p;
	glDeleteProgram(shader->glid);
#ifdef VAO_ENABLE
	glDeleteVertexArrays(1, &shader->glvao);
#endif

	CHECK_GL_ERROR
}

static void
close_texture(void *p, void *R) {
	struct texture * tex = (struct texture *)p;
	glDeleteTextures(1,&tex->glid);

	CHECK_GL_ERROR
}

static void
close_target(void *p, void *R) {
	struct target * tar = (struct target *)p;
	glDeleteFramebuffers(1, &tar->glid);

	CHECK_GL_ERROR
}

void
render_release(struct render *R, enum EJ_RENDER_OBJ what, RID id) {
	switch (what) {
	case EJ_VERTEXBUFFER:
	case EJ_INDEXBUFFER: {
		struct buffer * buf = (struct buffer *)array_ref(&R->buffer, id);
		if (buf) {
			close_buffer(buf, R);
			array_free(&R->buffer, buf);
		}
		break;
	}
	case EJ_SHADER: {
		struct shader * shader = (struct shader *)array_ref(&R->shader, id);
		if (shader) {
			close_shader(shader, R);
			array_free(&R->shader, shader);
		}
		break;
	}
	case EJ_TEXTURE : {
		struct texture * tex = (struct texture *) array_ref(&R->texture, id);
		if (tex) {
			for (int i = 0; i < MAX_TEXTURE; ++i) {
				RID last = R->last.texture[i];
				if (last == id) {
					R->last.texture[i] = 0;
				}
			}
			close_texture(tex, R);
			array_free(&R->texture, tex);
		}
		break;
	}
	case EJ_TARGET : {
		struct target * tar = (struct target *)array_ref(&R->target, id);
		if (tar) {
			close_target(tar, R);
			array_free(&R->target, tar);
		}
		break;
	}
	case EJ_VERTEXLAYOUT:
		struct attrib * attr = (struct attrib *)array_ref(&R->attrib, id);
		if (attr) {
			array_free(&R->attrib, attr);
		}
		break;
	default:
		assert(0);
		break;
	}
}

void
render_set(struct render *R, enum EJ_RENDER_OBJ what, RID id, int slot) {
	switch (what) {
	case EJ_VERTEXBUFFER:
		assert(slot >= 0 && slot < MAX_VB_SLOT);
		R->vbslot[slot] = id;
		R->changeflag |= CHANGE_VERTEXARRAY;
		break;
	case EJ_INDEXBUFFER:
		R->indexbuffer = id;
		R->changeflag |= CHANGE_VERTEXARRAY;
		break;
	case EJ_VERTEXLAYOUT:
		R->attrib_layout = id;
		break;
	case EJ_TEXTURE:
		assert(slot >= 0 && slot < MAX_TEXTURE);
		R->current.texture[slot] = id;
		R->changeflag |= CHANGE_TEXTURE;
		break;
	case EJ_TARGET:
		R->current.target = id;
		R->changeflag |= CHANGE_TARGET;
		break;
	default:
		assert(0);
		break;
	}
}

RID
render_get(struct render *R, enum EJ_RENDER_OBJ what, int slot) {
	switch (what) {
	case EJ_TEXTURE:
		return R->current.texture[slot];
	case EJ_TARGET:
		return R->current.target;
	default:
		assert(0);
		return 0;
	}
}

static void
apply_texture_uniform(struct shader *s) {
	int i;
	for (i=0;i<s->texture_n;i++) {
		int loc = s->texture_uniform[i];
		if (loc >= 0) {
			glUniform1i(loc, i);
		}
	}
}

void
render_shader_bind(struct render *R, RID id) {
	R->program = id;
	R->changeflag |= CHANGE_VERTEXARRAY;
	struct shader * s = (struct shader *)array_ref(&R->shader, id);
	if (s) {
		glUseProgram(s->glid);
		apply_texture_uniform(s);
	} else {
		glUseProgram(0);
	}

	CHECK_GL_ERROR
}

int
render_size(struct render_init_args *args) {
	return sizeof(struct render) +
		array_size(args->max_buffer, sizeof(struct buffer)) +
		array_size(args->max_layout, sizeof(struct attrib)) +
		array_size(args->max_target, sizeof(struct target)) +
		array_size(args->max_texture, sizeof(struct texture)) +
		array_size(args->max_shader, sizeof(struct shader));
}

static void
new_array(struct block *B, struct array *A, int n, int sz) {
	int s = array_size(n, sz);
	void * buffer = block_slice(B, s);
	array_init(A, buffer, n, sz);
}

struct render *
render_init(struct render_init_args *args, void * buffer, int sz) {
	struct block B;
	block_init(&B, buffer, sz);
	struct render * R = (struct render *)block_slice(&B, sizeof(struct render));
	memset(R, 0, sizeof(*R));
	logger_init(&R->log, stderr);
	new_array(&B, &R->buffer, args->max_buffer, sizeof(struct buffer));
	new_array(&B, &R->attrib, args->max_layout, sizeof(struct attrib));
	new_array(&B, &R->target, args->max_target, sizeof(struct target));
	new_array(&B, &R->texture, args->max_texture, sizeof(struct texture));
	new_array(&B, &R->shader, args->max_shader, sizeof(struct shader));

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &R->default_framebuffer);

	CHECK_GL_ERROR

	return R;
}

void
render_exit(struct render * R) {
	array_exit(&R->buffer, close_buffer, R);
	array_exit(&R->shader, close_shader, R);
	array_exit(&R->texture, close_texture, R);
	array_exit(&R->target, close_target, R);
}

void
render_setviewport(int x, int y, int width, int height) {
	glViewport(x, y, width, height);
}

void
render_setscissor(struct render *R, int x, int y, int width, int height ) {
	R->current.scissor = 1;
	R->current.scissor_x = x;
	R->current.scissor_y = y;
	R->current.scissor_w = width;
	R->current.scissor_h = height;
	R->changeflag |= CHANGE_SCISSOR;
}

static int
change_vb(struct render *R, struct shader * s) {
	int ret = 0;
	struct attrib* a = (struct attrib*)array_ref(&R->attrib, R->attrib_layout);
	assert(s->n == a->n);
	for (int i = 0; i < s->n; ++i) {
		if (s->a[i].stride != a->a[i].stride) {
			ret = 1;
			s->a[i].stride = a->a[i].stride;
		}
		if (s->a[i].offset != a->a[i].offset) {
			ret = 1;
			s->a[i].offset = a->a[i].offset;
		}
	}

#ifdef VAO_ENABLE
	int change = memcmp(R->vbslot, s->vbslot, sizeof(R->vbslot));
	memcpy(s->vbslot, R->vbslot, sizeof(R->vbslot));
	if (change) {
		ret = 1;
	}
#else
	ret = 1;
#endif
	return ret;
}

static int
change_ib(struct render *R, struct shader *s) {
#ifdef VAO_ENABLE
	RID ib = s->ib;
	s->ib = R->indexbuffer;
	return (R->indexbuffer != ib);
#else
	return 1;
#endif
}

static void
apply_va(struct render *R) {
	RID prog = R->program;
	struct shader * s = (struct shader *)array_ref(&R->shader, prog);
	if (s) {
#ifdef VAO_ENABLE
		glBindVertexArray(s->glvao);
#endif
		if (change_vb(R,s)) {
			int i;
			RID last_vb = 0;
			for (i=0;i<s->n;i++) {
				struct attrib_layout *al = &s->a[i];
				int vbidx = al->vbslot;
				RID vb = R->vbslot[vbidx];
				if (last_vb != vb) {
					struct buffer * buf = (struct buffer *)array_ref(&R->buffer, vb);
					if (buf == NULL) {
						continue;
					}
					glBindBuffer(GL_ARRAY_BUFFER, buf->glid);
					last_vb = vb;
				}
				glEnableVertexAttribArray(i);
				glVertexAttribPointer(i, al->size, al->type, al->normalized, al->stride, (const GLvoid *)(ptrdiff_t)(al->offset));
			}
		}

		if (change_ib(R,s)) {
			struct buffer * b = (struct buffer *)array_ref(&R->buffer, R->indexbuffer);
			if (b) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b->glid);
			}
		}

		CHECK_GL_ERROR
	}
}

// texture

static int
calc_texture_size(enum EJ_TEXTURE_FORMAT format, int width, int height) {
	switch( format ) {
	case EJ_TEXTURE_RGBA8 :
	case EJ_TEXTURE_BGRA_EXT:
		return width * height * 4;
	case EJ_TEXTURE_RGB565:
	case EJ_TEXTURE_RGBA4 :
		return width * height * 2;
	case EJ_TEXTURE_RGB:
	case EJ_TEXTURE_BGR_EXT:
		return width * height * 3;
    case EJ_TEXTURE_RGB16F:
        return width * height * 6;
    case EJ_TEXTURE_RGB32F:
        return width * height * 12;
    case EJ_TEXTURE_RG16F:
        return width * height * 4;
	case EJ_TEXTURE_A8 :
	case EJ_TEXTURE_DEPTH :
		return width * height;
	case EJ_TEXTURE_PVR2 :
		return width * height / 4;
	case EJ_TEXTURE_PVR4 :
	case EJ_TEXTURE_ETC1 :
		return width * height / 2;
	case EJ_TEXTURE_ETC2:
		return width * height;
	case EJ_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return width * height / 2;
	case EJ_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case EJ_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return width * height;
	default:
		return 0;
	}
}

RID
render_texture_create(struct render *R, int width, int height, int depth, enum EJ_TEXTURE_FORMAT format, enum EJ_TEXTURE_TYPE type, int mipmap_levels) {
	struct texture * tex = (struct texture *)array_alloc(&R->texture);
	if (tex == NULL)
		return 0;
	glGenTextures(1, &tex->glid);
	tex->width = width;
	tex->height = height;
	tex->depth = depth;
	tex->format = format;
	tex->type = type;
	assert(type == EJ_TEXTURE_2D || type == EJ_TEXTURE_3D || type == EJ_TEXTURE_CUBE);
	tex->mipmap_levels = mipmap_levels;
	int size = calc_texture_size(format, width, height);
	if (mipmap_levels > 1) {
		size += size / 3;
	}
    switch (type)
    {
    case EJ_TEXTURE_3D:
        size *= depth;
        break;
    case EJ_TEXTURE_CUBE:
        size *= 6;
        break;
    }
	tex->memsize = size;

	CHECK_GL_ERROR
	return array_id(&R->texture, tex);
}

static void
bind_texture(struct render *R, struct texture * tex, int slice, GLenum *type, int *target) {
	if (tex->type == EJ_TEXTURE_2D) {
		*type = GL_TEXTURE_2D;
		*target = GL_TEXTURE_2D;
	} else if (tex->type == EJ_TEXTURE_3D) {
		*type = GL_TEXTURE_3D;
		*target = GL_TEXTURE_3D;
	} else {
		assert(tex->type == EJ_TEXTURE_CUBE);
		*type = GL_TEXTURE_CUBE_MAP;
		*target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + slice;
	}
	glActiveTexture( GL_TEXTURE7 );
	R->changeflag |= CHANGE_TEXTURE;
	R->last.texture[7] = 0;	// use last texture slot

	glBindTexture( *type, tex->glid);
}

// return compressed
static int
texture_format(struct texture* tex, GLint* internal_format, GLenum* pixel_format, GLenum* itype) {
	int compressed = 0;
	switch(tex->format) {
	case EJ_TEXTURE_RGBA8 :
		*internal_format = *pixel_format = GL_RGBA;
		*itype = GL_UNSIGNED_BYTE;
		break;
	case EJ_TEXTURE_RGB :
		*internal_format = *pixel_format = GL_RGB;
		*itype = GL_UNSIGNED_BYTE;
		break;
	case EJ_TEXTURE_RGBA4 :
		*internal_format = *pixel_format = GL_RGBA;
		*itype = GL_UNSIGNED_SHORT_4_4_4_4;
		break;
	case EJ_TEXTURE_RGB565:
		*internal_format = *pixel_format = GL_RGB;
		*itype = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case EJ_TEXTURE_BGRA_EXT:
		*internal_format = GL_RGBA;
		*pixel_format = GL_BGRA_EXT;
		*itype = GL_UNSIGNED_BYTE;
		break;
	case EJ_TEXTURE_BGR_EXT:
		*internal_format = GL_RGB;
		*pixel_format = GL_BGR_EXT;
		*itype = GL_UNSIGNED_BYTE;
		break;
    case EJ_TEXTURE_RGB16F:
        *internal_format = GL_RGB16F;
        *pixel_format = GL_RGB;
        *itype = GL_FLOAT;
        break;
    case EJ_TEXTURE_RGB32F:
        *internal_format = GL_RGB32F;
        *pixel_format = GL_RGB;
        *itype = GL_FLOAT;
        break;
    case EJ_TEXTURE_RG16F:
        *internal_format = GL_RG16F;
        *pixel_format = GL_RG;
        *itype = GL_FLOAT;
        break;
	case EJ_TEXTURE_A8 :
	case EJ_TEXTURE_DEPTH :
	#if OPENGLES == 3 || OPENGLES == 0
//		*internal_format = *pixel_format = GL_RED;
		*internal_format = /*GL_DEPTH_COMPONENT24*/GL_DEPTH_COMPONENT;
		*pixel_format = GL_DEPTH_COMPONENT;
	#else
		*internal_format = *pixel_format = GL_ALPHA;
	#endif
		*itype = GL_UNSIGNED_BYTE;
	//	*itype = GL_FLOAT;
		break;
#ifdef GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
	case EJ_TEXTURE_PVR2 :
		*internal_format = *pixel_format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
		compressed = 1;
		break;
#endif
#ifdef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
	case EJ_TEXTURE_PVR4 :
		*internal_format = *pixel_format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		compressed = 1;
		break;
#endif
#ifdef GL_ETC1_RGB8_OES
	case EJ_TEXTURE_ETC1 :
		*internal_format = *pixel_format = GL_ETC1_RGB8_OES;
		compressed = 1;
		break;
#endif // GL_ETC1_RGB8_OES
	case EJ_TEXTURE_ETC2:
		*internal_format = *pixel_format = GL_COMPRESSED_RGBA8_ETC2_EAC;
		compressed = 1;
		break;
	case EJ_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		*internal_format = *pixel_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		compressed = 1;
		break;
	case EJ_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		*internal_format = *pixel_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		compressed = 1;
		break;
	case EJ_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		*internal_format = *pixel_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		compressed = 1;
		break;
	default:
		assert(0);
		return -1;
	}
	return compressed;
}

void
render_texture_update(struct render *R, RID id, int width, int height, int depth, const void *pixels,
                      int slice, int miplevel, enum EJ_TEXTURE_WRAP wrap, enum EJ_TEXTURE_FILTER filter) {
	struct texture * tex = (struct texture *)array_ref(&R->texture, id);
	if (tex == NULL)
		return;

	GLenum type;
	int target;
	bind_texture(R, tex, slice, &type, &target);

	if (tex->mipmap_levels > 1) {
        switch (filter) {
        case EJ_TEXTURE_NEAREST:
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            break;
        case EJ_TEXTURE_LINEAR:
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            break;
        }
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex->mipmap_levels - 1);
	} else {
        switch (filter) {
        case EJ_TEXTURE_NEAREST:
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;
        case EJ_TEXTURE_LINEAR:
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;
        }
	}
    switch (filter) {
    case EJ_TEXTURE_NEAREST:
        glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
    case EJ_TEXTURE_LINEAR:
        glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    GLint gl_wrap;
    switch (wrap) {
    case EJ_TEXTURE_REPEAT:
        gl_wrap = GL_REPEAT;
        break;
    case EJ_TEXTURE_MIRRORED_REPEAT:
        gl_wrap = GL_MIRRORED_REPEAT;
        break;
    case EJ_TEXTURE_CLAMP_TO_EDGE:
        gl_wrap = GL_CLAMP_TO_EDGE;
        break;
    case EJ_TEXTURE_CLAMP_TO_BORDER:
        gl_wrap = GL_CLAMP_TO_BORDER;
        break;
    }
    glTexParameteri(type, GL_TEXTURE_WRAP_S, gl_wrap);
    glTexParameteri(type, GL_TEXTURE_WRAP_T, gl_wrap);
    if (type == GL_TEXTURE_3D) {
        glTexParameteri(type, GL_TEXTURE_WRAP_R, gl_wrap);
    }

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (type == GL_TEXTURE_3D) {
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, depth, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    } else if (type == GL_TEXTURE_CUBE_MAP) {
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        }
    } else {
	    GLint internal_format = 0;
	    GLenum pixel_format = 0;
	    GLenum itype = 0;
	    int compressed = texture_format(tex, &internal_format, &pixel_format, &itype);
	    if (compressed) {
 		    glCompressedTexImage2D(target, miplevel, pixel_format,
 			    (GLsizei)tex->width, (GLsizei)tex->height, 0,
 			    calc_texture_size(tex->format, width, height), pixels);
	    } else {
		    glTexImage2D(target, miplevel, internal_format, (GLsizei)width, (GLsizei)height, 0, pixel_format, itype, pixels);
	    }
    }

    if (tex->mipmap_levels > 1) {
        glGenerateMipmap(type);
    }

	CHECK_GL_ERROR
}

void
render_texture_subupdate(struct render *R, RID id, const void *pixels, int x, int y, int w, int h, int slice, int miplevel) {
	struct texture * tex = (struct texture *)array_ref(&R->texture, id);
	if (tex == NULL)
		return;

	GLenum type;
	int target;
	bind_texture(R, tex, slice, &type, &target);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	GLint internal_format = 0;
	GLenum pixel_format = 0;
	GLenum itype = 0;
	int compressed = texture_format(tex, &internal_format, &pixel_format, &itype);
	if (compressed) {
		glCompressedTexSubImage2D(GL_TEXTURE_2D, miplevel,
			x, y, w, h, pixel_format,
			calc_texture_size(tex->format, w, h), pixels);
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, miplevel, x, y, w, h, pixel_format, itype, pixels);
	}

	CHECK_GL_ERROR
}

// blend func
void
render_set_blendfunc(struct render *R, enum EJ_BLEND_FORMAT src, enum EJ_BLEND_FORMAT dst) {
	R->current.blend_src = src;
	R->current.blend_dst = dst;
	R->changeflag |= CHANGE_BLEND_FUNC;
}

// blend equation
void
render_set_blendeq(struct render *R, enum EJ_BLEND_FUNC eq) {
	R->current.blend_func = eq;
	R->changeflag |= CHANGE_BLEND_EQ;
}

void
render_set_alpha_test(struct render *R, enum EJ_ALPHA_FUNC func, float ref) {
	R->current.alpha_func = func;
	R->current.alpha_ref = ref;
	R->changeflag |= CHANGE_ALPHA;
}

// depth
void
render_enabledepthmask(struct render *R, int enable) {
	R->current.depthmask = enable;
	R->changeflag |= CHANGE_DEPTH;
}

// depth
void
render_enablescissor(struct render *R, int enable) {
	R->current.scissor = enable;
	R->changeflag |= CHANGE_SCISSOR;
}

void
render_setdepth(struct render *R, enum EJ_DEPTH_FORMAT d) {
	R->current.depth = d;
	R->changeflag |= CHANGE_DEPTH;
}

// cull

void
render_set_front_face(struct render *R, int clockwise) {
	if (clockwise) {
		glFrontFace(GL_CW);
	} else {
		glFrontFace(GL_CCW);
	}
}

void
render_setcull(struct render *R, enum EJ_CULL_MODE c) {
	R->current.cull = c;
	R->changeflag |= CHANGE_CULL;
}

// render target
static RID
create_rt(struct render *R, RID texid) {
	struct target *tar = (struct target *)array_alloc(&R->target);
	if (tar == NULL)
		return 0;
	tar->tex = texid;
	struct texture * tex = (struct texture *)array_ref(&R->texture, texid);
	if (tex == NULL)
		return 0;
	glGenFramebuffers(1, &tar->glid);
	glBindFramebuffer(GL_FRAMEBUFFER, tar->glid);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->glid, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		close_target(tar, R);
		return 0;
	}
	CHECK_GL_ERROR

	return array_id(&R->target, tar);
}

RID
render_target_create(struct render *R, int width, int height, enum EJ_TEXTURE_FORMAT format) {
	RID tex = render_texture_create(R, width, height, 0, format, EJ_TEXTURE_2D, 0);
	if (tex == 0)
		return 0;
	render_texture_update(R, tex, width, height, 0, NULL, 0, 0, EJ_TEXTURE_REPEAT, EJ_TEXTURE_LINEAR);
	RID rt = create_rt(R, tex);
	glBindFramebuffer(GL_FRAMEBUFFER, R->default_framebuffer);
	R->last.target = 0;
	R->changeflag |= CHANGE_TARGET;

	if (rt == 0) {
		render_release(R, EJ_TEXTURE, tex);
	}
	CHECK_GL_ERROR
	return rt;
}

RID
render_target_texture(struct render *R, RID rt) {
	struct target *tar = (struct target *)array_ref(&R->target, rt);
	if (tar) {
		return tar->tex;
	} else {
		return 0;
	}
}

// render state

static void
render_state_commit(struct render *R) {
	if (R->changeflag & CHANGE_VERTEXARRAY) {
		apply_va(R);
	}

	if (R->changeflag & CHANGE_TEXTURE) {
		static GLenum mode[] = {
			GL_TEXTURE_2D,
			GL_TEXTURE_3D,
			GL_TEXTURE_CUBE_MAP,
		};
		int i;
		for (i=0;i<MAX_TEXTURE;i++) {
			RID id = R->current.texture[i];
			RID lastid = R->last.texture[i];
			if (id != lastid) {
				R->last.texture[i] = id;
				struct texture * tex = (struct texture *)array_ref(&R->texture, id);
				if (tex) {
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(mode[tex->type], tex->glid);
				}
			}
		}
		CHECK_GL_ERROR
	}

	if (R->changeflag & CHANGE_TARGET) {
		RID crt = R->current.target;
		if (R->last.target != crt) {
			GLuint rt = R->default_framebuffer;
			if (crt != 0) {
				struct target * tar = (struct target *)array_ref(&R->target, crt);
				if (tar) {
					rt = tar->glid;
				} else {
					crt = 0;
				}
			}
			CHECK_GL_ERROR
			glBindFramebuffer(GL_FRAMEBUFFER, rt);
			R->last.target = crt;
			CHECK_GL_ERROR
		}
	}

	if (R->changeflag & CHANGE_BLEND_FUNC) {
		if (R->last.blend_src != R->current.blend_src || R->last.blend_dst != R->current.blend_dst) {
			if (R->current.blend_src == EJ_BLEND_DISABLE) {
				glDisable(GL_BLEND);
			} else if (R->last.blend_src == EJ_BLEND_DISABLE) {
				glEnable(GL_BLEND);
			}
			static GLenum blend[] = {
				0,
				GL_ZERO,
				GL_ONE,
				GL_SRC_COLOR,
				GL_ONE_MINUS_SRC_COLOR,
				GL_SRC_ALPHA,
				GL_ONE_MINUS_SRC_ALPHA,
				GL_DST_ALPHA,
				GL_ONE_MINUS_DST_ALPHA,
				GL_DST_COLOR,
				GL_ONE_MINUS_DST_COLOR,
				GL_SRC_ALPHA_SATURATE,
			};

			enum EJ_BLEND_FORMAT src = R->current.blend_src;
			enum EJ_BLEND_FORMAT dst = R->current.blend_dst;
			glBlendFunc(blend[src], blend[dst]);

			R->last.blend_src = src;
			R->last.blend_dst = dst;
		}
	}

	if (R->changeflag & CHANGE_BLEND_EQ) {
		if (R->last.blend_func != R->current.blend_func) {
			static GLenum blend[] = {
				GL_FUNC_ADD,
				GL_FUNC_SUBTRACT,
				GL_FUNC_REVERSE_SUBTRACT,
                GL_MIN,
                GL_MAX,
			};
			enum EJ_BLEND_FUNC func = R->current.blend_func;
			glBlendEquation(blend[func]);
			R->last.blend_func = func;
		}
	}

	if (R->changeflag & CHANGE_ALPHA) {
		if (R->last.alpha_func != R->current.alpha_func || R->last.alpha_ref != R->current.alpha_ref) {
			if (R->current.alpha_func == EJ_ALPHA_DISABLE) {
				glDisable(GL_ALPHA_TEST);
			} else {
				glEnable(GL_ALPHA_TEST);

				static GLenum alpha[] = {
					0,
					GL_NEVER,
					GL_LESS,
					GL_EQUAL,
					GL_LEQUAL,
					GL_GREATER,
					GL_NOTEQUAL,
					GL_GEQUAL,
					GL_ALWAYS,
				};
				glAlphaFunc(alpha[R->current.alpha_func], R->current.alpha_ref);
			}

			R->last.alpha_func = R->current.alpha_func;
			R->last.alpha_ref = R->current.alpha_ref;
		}
	}

	if (R->changeflag & CHANGE_DEPTH) {
		if (R->last.depth != R->current.depth) {
			if (R->last.depth == EJ_DEPTH_DISABLE) {
				glEnable( GL_DEPTH_TEST);
			}
			if (R->current.depth == EJ_DEPTH_DISABLE) {
				glDisable( GL_DEPTH_TEST);
			} else {
				static GLenum depth[] = {
					0,
					GL_LEQUAL,
					GL_LESS,
					GL_EQUAL,
					GL_GREATER,
					GL_GEQUAL,
					GL_ALWAYS,
                    GL_NOTEQUAL,
                    GL_NEVER,
				};
				glDepthFunc( depth[R->current.depth] );
			}
			R->last.depth = R->current.depth;
		}
		if (R->last.depthmask != R->current.depthmask) {
			glDepthMask(R->current.depthmask ? GL_TRUE : GL_FALSE);
			R->last.depthmask = R->current.depthmask;
		}
	}

	if (R->changeflag & CHANGE_CULL) {
		if (R->last.cull != R->current.cull) {
			if (R->last.cull == EJ_CULL_DISABLE) {
				glEnable(GL_CULL_FACE);
			}
			if (R->current.cull == EJ_CULL_DISABLE) {
				glDisable(GL_CULL_FACE);
			} else {
                static GLenum cull[] = {
					0,
                    GL_FRONT,
                    GL_BACK,
					GL_FRONT_AND_BACK
				};
				glCullFace(cull[R->current.cull]);
			}
			R->last.cull = R->current.cull;
		}
	}

	if (R->changeflag & CHANGE_SCISSOR) {
		if (R->last.scissor != R->current.scissor) {
			if (R->current.scissor) {
				glEnable(GL_SCISSOR_TEST);
			} else {
				glDisable(GL_SCISSOR_TEST);
				R->current.scissor_x = R->last.scissor_x;
				R->current.scissor_y = R->last.scissor_y;
				R->current.scissor_w = R->last.scissor_w;
				R->current.scissor_h = R->last.scissor_h;
			}

			R->last.scissor = R->current.scissor;
		}
		if (R->current.scissor &&
			(R->last.scissor_x != R->current.scissor_x ||
			 R->last.scissor_y != R->current.scissor_y ||
			 R->last.scissor_w != R->current.scissor_w ||
			 R->last.scissor_h != R->current.scissor_h)) {
			glScissor(R->current.scissor_x, R->current.scissor_y, R->current.scissor_w, R->current.scissor_h);
			R->last.scissor_x = R->current.scissor_x;
			R->last.scissor_y = R->current.scissor_y;
			R->last.scissor_w = R->current.scissor_w;
			R->last.scissor_h = R->current.scissor_h;
		}
	}

	R->changeflag = 0;

	CHECK_GL_ERROR
}

void
render_state_reset(struct render *R) {
	R->changeflag = ~0;
	memset(&R->last, 0 , sizeof(R->last));
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDepthMask(GL_FALSE);
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glBindFramebuffer(GL_FRAMEBUFFER, R->default_framebuffer);

	CHECK_GL_ERROR
}

void
render_clear(struct render *R, enum EJ_CLEAR_MASK mask, unsigned long c) {
	GLbitfield m = 0;
	if (mask & EJ_MASKC) {
		m |= GL_COLOR_BUFFER_BIT;
		float a = ((c >> 24) & 0xff ) / 255.0f;
		float r = ((c >> 16) & 0xff ) / 255.0f;
		float g = ((c >> 8) & 0xff ) / 255.0f;
		float b = ((c >> 0) & 0xff ) / 255.0f;
		glClearColor(r,g,b,a);
	}
	if (mask & EJ_MASKD) {
		m |= GL_DEPTH_BUFFER_BIT;
	}
	if (mask & EJ_MASKS) {
		m |= GL_STENCIL_BUFFER_BIT;
	}
	render_state_commit(R);
	glClear(m);

	CHECK_GL_ERROR
}

// draw
void
render_draw_elements(struct render *R, enum EJ_DRAW_MODE mode, int fromidx, int ni) {
	static int draw_mode[] = {
		GL_POINTS,
		GL_LINES,
		GL_LINE_LOOP,
		GL_LINE_STRIP,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,
	};
	assert((int)mode < sizeof(draw_mode)/sizeof(int));
	render_state_commit(R);
	RID ib = R->indexbuffer;
	struct buffer * buf = (struct buffer *)array_ref(&R->buffer, ib);
	if (buf) {
		int offset = fromidx * sizeof(short);
		glDrawElements(draw_mode[mode], ni, GL_UNSIGNED_SHORT, (char *)0 + offset);
		CHECK_GL_ERROR
	}
}

void
render_draw_elements_vao(struct render *R, enum EJ_DRAW_MODE mode, int fromidx, int ni, unsigned int vao) {
	render_state_commit(R);

	static int draw_mode[] = {
		GL_POINTS,
		GL_LINES,
		GL_LINE_LOOP,
		GL_LINE_STRIP,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,
	};
	assert((int)mode < sizeof(draw_mode) / sizeof(int));

	glBindVertexArray(vao);

	int offset = sizeof(short) * fromidx;
	glDrawElements(draw_mode[mode], ni, GL_UNSIGNED_SHORT, (char *)0 + offset);

	glBindVertexArray(0);

	CHECK_GL_ERROR
}

void
render_draw_elements_no_buf(struct render *R, enum EJ_DRAW_MODE mode, int size, unsigned int* indices) {
	static int draw_mode[] = {
		GL_POINTS,
		GL_LINES,
		GL_LINE_LOOP,
		GL_LINE_STRIP,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,
	};
	assert((int)mode < sizeof(draw_mode)/sizeof(int));
	render_state_commit(R);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDrawElements(draw_mode[mode], size, GL_UNSIGNED_INT, indices);
	CHECK_GL_ERROR
}

void
render_draw_arrays(struct render *R, enum EJ_DRAW_MODE mode, int fromidx, int ni) {
	static int draw_mode[] = {
		GL_POINTS,
		GL_LINES,
		GL_LINE_LOOP,
		GL_LINE_STRIP,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,
	};
	assert((int)mode < sizeof(draw_mode)/sizeof(int));
	render_state_commit(R);
	glDrawArrays(draw_mode[mode], fromidx, ni);
	CHECK_GL_ERROR
}

void
render_draw_arrays_vao(struct render *R, enum EJ_DRAW_MODE mode, int fromidx, int ni, unsigned int vao) {
	render_state_commit(R);

	static int draw_mode[] = {
		GL_POINTS,
		GL_LINES,
		GL_LINE_LOOP,
		GL_LINE_STRIP,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,
	};
	assert((int)mode < sizeof(draw_mode) / sizeof(int));

	glBindVertexArray(vao);

	glDrawArrays(draw_mode[mode], fromidx, ni);

	glBindVertexArray(0);

	CHECK_GL_ERROR
}

// uniform
int
render_shader_locuniform(struct render *R, const char * name) {
	struct shader * s = (struct shader *)array_ref(&R->shader, R->program);
	if (s) {
		int loc = glGetUniformLocation( s->glid, name);
		CHECK_GL_ERROR
		return loc;
	} else {
		return -1;
	}
}

void
render_shader_setuniform(struct render *R, int loc, enum EJ_UNIFORM_FORMAT format, const float *v, int n) {
	switch(format) {
	case EJ_UNIFORM_FLOAT1:
		glUniform1f(loc, v[0]);
		break;
	case EJ_UNIFORM_FLOAT2:
		glUniform2f(loc, v[0], v[1]);
		break;
	case EJ_UNIFORM_FLOAT3:
		glUniform3f(loc, v[0], v[1], v[2]);
		break;
	case EJ_UNIFORM_FLOAT4:
		glUniform4f(loc, v[0], v[1], v[2], v[3]);
		break;
	case EJ_UNIFORM_FLOAT33:
		glUniformMatrix3fv(loc, 1, GL_FALSE, v);
		break;
	case EJ_UNIFORM_FLOAT44:
		glUniformMatrix4fv(loc, 1, GL_FALSE, v);
		break;
	case EJ_UNIFORM_INT1:
		glUniform1i(loc, (int)(*v));
		break;
	case EJ_UNIFORM_MULTI_FLOAT44:
		glUniformMatrix4fv(loc, n, GL_FALSE, v);
		break;
	default:
		assert(0);
		return;
	}
	CHECK_GL_ERROR
}

int
render_version(struct render *R) {
	return OPENGLES;
}

int
render_get_texture_gl_id(struct render *R, RID id) {
	struct texture * tex = (struct texture *)array_ref(&R->texture, id);
	if (tex == NULL) {
		return 0;
	} else {
		return tex->glid;
	}
}

int
render_query_target() {
	GLint fbo = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
	return fbo;
}

void
render_clear_texture_cache(struct render* R) {
	memset(R->last.texture, 0, sizeof(R->last.texture));
}