#pragma once

#include "unirender/typedef.h"

#include <cstdint>

namespace ur
{

class RenderContext;

class Sandbox
{
public:
    Sandbox(RenderContext& rc);
    ~Sandbox();

private:
    RenderContext& m_rc;

    int m_shader_id = 0;
    int m_vert_layout_id = 0;

    size_t m_rt_depth = 0;

    int m_blend_eq = 0;
    int m_blend_src = 0, m_blend_dst = 0;

    ALPHA_FUNC m_alpha_func = ALPHA_ALWAYS;
    float m_alpha_ref = 0;

    bool m_zwrite = false;
    DEPTH_FORMAT m_ztest = DEPTH_DISABLE;

    bool m_front_face_clockwise = false;

    CULL_MODE m_cull = CULL_DISABLE;

    int m_clear_flag = 0;
    uint32_t m_clear_color = 0;

    int m_vp_x = -1, m_vp_y = -1, m_vp_w = -1, m_vp_h = -1;

    float m_point_size = 1;
    float m_line_width = 1;

    POLYGON_MODE m_poly_mode = POLYGON_FILL;

}; // Sandbox

}