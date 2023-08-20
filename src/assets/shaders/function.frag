#version 330 core

out vec4 out_color;

in vec2  f_tex_pos;      // Fragment position in world coordinates

uniform vec2 u_camera_step;
uniform vec2 u_camera_start;
uniform vec2 u_pixel_offset;
uniform vec2 u_window_size;
uniform vec4 u_color;

uniform sampler2D u_read_texture;
bool sign_changes(float a, float b);
bool does_intersect(vec2 pos, vec2 step);
float function(vec2 pos, vec2 step);
float render(vec2 pos, vec2 step);

void main() {
    vec2 f_pos = vec2(f_tex_pos.x, f_tex_pos.y);
    vec2 pos = (f_tex_pos * u_window_size + u_pixel_offset) * u_camera_step + u_camera_start;
    
    vec4 bgc = texture(u_read_texture, f_tex_pos);
    float value = render(pos, u_camera_step) * u_color.a;
    out_color = value * u_color + (1.0 - value) * bgc;
    out_color.a = 1.0;
}


bool sign_changes(float a, float b) {
    if ((a < 0 && b > 0) || (a > 0 && b < 0))
        return abs(a - b) < 10; // This 10 is arbitrary. It should be a high as possible, while not allowing for artifacts

    return false;
}

float render(vec2 pos, vec2 step) {
    return clamp(function(pos - step, step * 2), 0, 1);
}

// 'function' will be placed here (at the end of file)