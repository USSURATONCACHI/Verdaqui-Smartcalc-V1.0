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


//CODE


float uniq_0_0(vec2 pos, vec2 step){
return (pos.y) - (sin(pos.x));
}

float uniq_1_0(vec2 pos, vec2 step){
float 
    lb = uniq_0_0(pos + vec2(0.0, step.y), step), 
    rb = uniq_0_0(pos + step, step), 
    lt = uniq_0_0(pos, step), 
    rt = uniq_0_0(pos + vec2(step.x, 0.0), step);

bool res =
    sign_changes(lt, rt) ||
    sign_changes(lt, rb) ||
    sign_changes(lt, lb) ||

    sign_changes(lb, rb) ||
    sign_changes(lb, rt) ||

    sign_changes(rb, rt);
return ( res) ? 1.0 : 0.0;
}

float uniq_2_0(vec2 pos, vec2 step){
return (pos.y) - ((((1.0*pos.x*pos.x) + (2.0000000000 * pos.x)) + 0.0000000000));
}

float uniq_3_0(vec2 pos, vec2 step){
float 
    lb = uniq_2_0(pos + vec2(0.0, step.y), step), 
    rb = uniq_2_0(pos + step, step), 
    lt = uniq_2_0(pos, step), 
    rt = uniq_2_0(pos + vec2(step.x, 0.0), step);

bool res =
    sign_changes(lt, rt) ||
    sign_changes(lt, rb) ||
    sign_changes(lt, lb) ||

    sign_changes(lb, rb) ||
    sign_changes(lb, rt) ||

    sign_changes(rb, rt);
return ( res) ? 1.0 : 0.0;
}

float uniq_4_0(vec2 pos, vec2 step){
return (sin((pos.x + pos.y))) - ((pos.y - pos.x));
}

float uniq_5_0(vec2 pos, vec2 step){
float 
    lb = uniq_4_0(pos + vec2(0.0, step.y), step), 
    rb = uniq_4_0(pos + step, step), 
    lt = uniq_4_0(pos, step), 
    rt = uniq_4_0(pos + vec2(step.x, 0.0), step);

bool res =
    sign_changes(lt, rt) ||
    sign_changes(lt, rb) ||
    sign_changes(lt, lb) ||

    sign_changes(lb, rb) ||
    sign_changes(lb, rt) ||

    sign_changes(rb, rt);
return ( res) ? 1.0 : 0.0;
}

float uniq_6_0(vec2 pos, vec2 step){
return (pos.x) - (1.0000000000);
}

float uniq_7_0(vec2 pos, vec2 step){
float 
    lb = uniq_6_0(pos + vec2(0.0, step.y), step), 
    rb = uniq_6_0(pos + step, step), 
    lt = uniq_6_0(pos, step), 
    rt = uniq_6_0(pos + vec2(step.x, 0.0), step);

bool res =
    sign_changes(lt, rt) ||
    sign_changes(lt, rb) ||
    sign_changes(lt, lb) ||

    sign_changes(lb, rb) ||
    sign_changes(lb, rt) ||

    sign_changes(rb, rt);
return ( res) ? 1.0 : 0.0;
}

float uniq_8_0(vec2 pos, vec2 step){
return (pos.x) - (2.0000000000);
}

float uniq_9_0(vec2 pos, vec2 step){
float 
    lb = uniq_8_0(pos + vec2(0.0, step.y), step), 
    rb = uniq_8_0(pos + step, step), 
    lt = uniq_8_0(pos, step), 
    rt = uniq_8_0(pos + vec2(step.x, 0.0), step);

bool res =
    sign_changes(lt, rt) ||
    sign_changes(lt, rb) ||
    sign_changes(lt, lb) ||

    sign_changes(lb, rb) ||
    sign_changes(lb, rt) ||

    sign_changes(rb, rt);
return ( res) ? 1.0 : 0.0;
}

float uniq_10_0(vec2 pos, vec2 step){
return (pos.y) - ((uniq_7_0(pos, step) + uniq_9_0(pos, step)));
}

float uniq_11_0(vec2 pos, vec2 step){
float 
    lb = uniq_10_0(pos + vec2(0.0, step.y), step), 
    rb = uniq_10_0(pos + step, step), 
    lt = uniq_10_0(pos, step), 
    rt = uniq_10_0(pos + vec2(step.x, 0.0), step);

bool res =
    sign_changes(lt, rt) ||
    sign_changes(lt, rb) ||
    sign_changes(lt, lb) ||

    sign_changes(lb, rb) ||
    sign_changes(lb, rt) ||

    sign_changes(rb, rt);
return ( res) ? 1.0 : 0.0;
}float function(vec2 pos, vec2 step) {
 return uniq_11_0(pos, step);
}
