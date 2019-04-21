#include "unirender/Sandbox.h"
#include "unirender/RenderContext.h"

namespace ur
{

Sandbox::Sandbox(RenderContext& rc)
    : m_rc(rc)
{
    m_rt_depth = m_rc.GetRenderTargetDepth();

    m_shader_id = m_rc.GetBindedShader();
    m_vert_layout_id = m_rc.GetVertexLayout();

    m_blend_eq = m_rc.GetBlendEquation();
    m_rc.GetBlendFunc(m_blend_src, m_blend_dst);

    m_rc.GetAlphaTest(m_alpha_func, m_alpha_ref);

    m_zwrite = m_rc.GetZWrite();
    m_ztest = m_rc.GetZTest();

    m_front_face_clockwise = m_rc.GetFrontFace();

    m_cull = m_rc.GetCullMode();

    m_clear_flag = m_rc.GetClearFlag();
    m_clear_color = m_rc.GetClearColor();

    m_rc.GetViewport(m_vp_x, m_vp_y, m_vp_w, m_vp_h);

    m_point_size = m_rc.GetPointSize();
    m_line_width = m_rc.GetLineWidth();

    m_poly_mode = m_rc.GetPolygonMode();
}

Sandbox::~Sandbox()
{
    size_t depth = m_rc.GetRenderTargetDepth();
    for (int i = 0; i < static_cast<int>(depth - m_rt_depth); ++i) {
        m_rc.UnbindRenderTarget();
    }

    m_rc.BindShader(m_shader_id);
    m_rc.BindVertexLayout(m_vert_layout_id);

    m_rc.SetBlendEquation(m_blend_eq);
    m_rc.SetBlend(m_blend_src, m_blend_dst);

    m_rc.SetAlphaTest(m_alpha_func, m_alpha_ref);

    m_rc.SetZWrite(m_zwrite);
    m_rc.SetZTest(m_ztest);

    m_rc.SetFrontFace(m_front_face_clockwise);

    m_rc.SetCullMode(m_cull);

    m_rc.SetClearFlag(m_clear_flag);
    m_rc.SetClearColor(m_clear_color);

    m_rc.SetViewport(m_vp_x, m_vp_y, m_vp_w, m_vp_h);

    m_rc.SetPointSize(m_point_size);
    m_rc.SetLineWidth(m_line_width);

    m_rc.SetPolygonMode(m_poly_mode);
}

}