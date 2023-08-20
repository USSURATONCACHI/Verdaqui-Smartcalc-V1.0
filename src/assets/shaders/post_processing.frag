#version 330 core

out vec4 out_color;

in vec2  f_tex_pos;      // Fragment position in world coordinates

uniform vec2 u_camera_step;
uniform vec2 u_camera_start;
uniform vec2 u_pixel_offset;
uniform vec2 u_window_size;

uniform sampler2D u_read_texture;

#define SSAA 2.0

void main() {
    vec2 f_pos = vec2(f_tex_pos.x, f_tex_pos.y);
    vec2 pos = (f_tex_pos * u_window_size + u_pixel_offset) * u_camera_step + u_camera_start;
    
    out_color = texture(u_read_texture, f_tex_pos);
}
