#include "unirender/TextureCube.h"
#include "unirender/RenderContext.h"
#include "unirender/Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{

const char* cubemap_vs = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    WorldPos = aPos;
    gl_Position =  projection * view * vec4(WorldPos, 1.0);
}
)";

const char* equirectangular_to_cubemap_fs = R"(
#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(equirectangularMap, uv).rgb;

    FragColor = vec4(color, 1.0);
}
)";

}

namespace ur
{

TextureCube::TextureCube(RenderContext* rc)
    : m_rc(rc)
{
}

TextureCube::~TextureCube()
{
    if (m_texid != 0) {
        m_rc->ReleaseTexture(m_texid);
    }
}

// code from https://learnopengl.com/PBR/IBL/Diffuse-irradiance
void TextureCube::BuildFromEquirectangularTex(unsigned int tex)
{
    if (m_texid != 0) {
        m_rc->ReleaseTexture(m_texid);
    }

    m_texid = m_rc->CreateTextureCube();

    m_rc->CheckError();

    // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
    // ----------------------------------------------------------------------------------------------
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    std::vector<std::string> textures;
    CU_VEC<VertexAttrib> va_list;
    CU_VEC<ur::VertexAttrib> layout;
    auto shader = std::make_shared<Shader>(m_rc, cubemap_vs, equirectangular_to_cubemap_fs, textures, va_list, true);
    m_rc->CheckError();

    shader->Use();
    m_rc->CheckError();

    shader->SetInt("equirectangularMap", 0);
    shader->SetMat4("projection", &captureProjection[0][0]);

    m_rc->CheckError();

    m_rc->BindTexture(tex, 0);
    m_rc->SetViewport(0, 0, 512, 512);

    m_rc->CheckError();

    auto rt = m_rc->CreateRenderTarget(0);
    m_rc->BindRenderTarget(rt);
    for (int i = 0; i < 6; ++i)
    {
        m_rc->CheckError();

        shader->SetMat4("view", &captureViews[i][0][0]);
        m_rc->CheckError();

        m_rc->BindRenderTargetTex(m_texid, ur::ATTACHMENT_COLOR0, ur::TEXTURE_CUBE0 + i);
        m_rc->CheckError();

        m_rc->SetClearFlag(ur::MASKC | ur::MASKD);
        m_rc->SetClearColor(0x88888888);
        m_rc->CheckError();

        m_rc->Clear();

        m_rc->CheckError();

        m_rc->RenderCube();
    }
    m_rc->BindRenderTarget(0);
    m_rc->ReleaseRenderTarget(rt);
}

}