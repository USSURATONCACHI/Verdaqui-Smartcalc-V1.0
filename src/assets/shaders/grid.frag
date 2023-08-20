#version 330 core

out vec4 out_color;

in vec2  f_tex_pos;      // Fragment position in world coordinates

uniform vec2 u_camera_step;
uniform vec2 u_camera_start;
uniform vec2 u_pixel_offset;
uniform vec2 u_window_size;

uniform sampler2D u_read_texture;



#define GRID_BASE 4



bool does_intersect_grid(vec2 pos, vec2 step, vec2 scale);
bool does_intersect_zero(vec2 pos, vec2 step);
float transition(float x, float from, float to);
vec4 color(float brightness);

void main() {
    vec2 f_pos = vec2(f_tex_pos.x, f_tex_pos.y);
    vec2 pos = (f_tex_pos * u_window_size + u_pixel_offset) * u_camera_step + u_camera_start;

    vec4 bgc = vec4(1.0, 1.0, 1.0, 1.0);
    float grid_exp = log(u_camera_step.x * 100.0) / log(float(GRID_BASE));
    float grid_exp_int = round(grid_exp);

    float zoom_anim_coef = grid_exp - floor(grid_exp);
    float scale_coef = grid_exp_int <= grid_exp ? 1.0 : GRID_BASE;

    float low_scale = pow(GRID_BASE, floor(grid_exp) - 1.0);
    float mid_scale = low_scale * GRID_BASE;
    float hig_scale = mid_scale * GRID_BASE;

    if (does_intersect_zero(pos, u_camera_step * 2))                      bgc = color(0.2);
    else if (does_intersect_grid(pos, u_camera_step, vec2(hig_scale)))    bgc = color(transition(zoom_anim_coef, 0.2, 0.4));
    else if (does_intersect_grid(pos, u_camera_step, vec2(mid_scale)))    bgc = color(transition(zoom_anim_coef, 0.4, 0.8));
    else if (does_intersect_grid(pos, u_camera_step, vec2(low_scale)))    bgc = color(transition(zoom_anim_coef, 0.8, 1.0));
    
    out_color = bgc;
}


bool does_intersect_grid(vec2 pos, vec2 step, vec2 scale) {
    vec2 a = mod(pos, scale * 2);
    vec2 b = mod(pos + step, scale * 2);
    return a.x == 0 || b.x == 0 || a.x > b.x || a.y > b.y;
}

bool does_intersect_zero(vec2 pos, vec2 step) {
    vec2 tmp = pos * (pos + step);
    return tmp.x <= 0 || tmp.y <= 0 || pos.x == 0 || pos.y == 0;
}

float transition(float x, float from, float to) {
    return x * to + (1.0 - x) * from;
}

vec4 color(float brightness) {
    return vec4(vec3(brightness), 1.0);
}