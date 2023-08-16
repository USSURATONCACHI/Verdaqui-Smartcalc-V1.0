#version 330 core

out vec4 out_color;

in vec2  f_tex_pos;      // Fragment position in world coordinates

uniform vec2 camera_step;
uniform vec2 camera_start;
uniform vec2 pixel_offset;
uniform vec2 window_size;

#define MSAA 16
#define GRID_BASE 4

float render(vec2 pos, vec2 step);
float function(vec2 pos);
bool does_intersect(vec2 pos, vec2 step);
float draw_msaa(vec2 pos);

bool does_intersect_grid(vec2 pos, vec2 step, vec2 scale);
bool does_intersect_zero(vec2 pos, vec2 step);

bool sign_changes(float a, float b) {
    if ((a < 0 && b > 0) || (a > 0 && b < 0))
        return abs(a - b) < 10; // This 10 is arbitrary. It should be a high as possible, while not allowing for artifacts

    return false;
}

float transition(float x, float from, float to) {
    return x * to + (1.0 - x) * from;
}

vec4 color(float brightness) {
    return vec4(vec3(brightness), 1.0);
}

void main() {
    vec2 f_pos = vec2(f_tex_pos.x, f_tex_pos.y);
    vec2 pos = (f_tex_pos * window_size + pixel_offset) * camera_step + camera_start;

    if (isinf(pos.x) || isinf(pos.y)) {
        out_color = vec4(0.0, 0.0, 0.0, 0.5);
        return;
    }

    //float val = function(pos);
    //vec4 bgc = vec4(val, 0.0, -val, 1.0);
    vec4 bgc = vec4(1.0, 1.0, 1.0, 1.0);
    float value = draw_msaa(pos);

    // If background might be visible
    if (value < 0.8) {
        float grid_exp = log(camera_step.x * 100.0) / log(float(GRID_BASE));
        float grid_exp_int = round(grid_exp);

        float zoom_anim_coef = grid_exp - floor(grid_exp);
        float scale_coef = grid_exp_int <= grid_exp ? 1.0 : GRID_BASE;

        float low_scale = pow(GRID_BASE, floor(grid_exp) - 1.0);
        float mid_scale = low_scale * GRID_BASE;
        float hig_scale = mid_scale * GRID_BASE;

        if (does_intersect_zero(pos, camera_step * 2))                      bgc = color(0.2);
        else if (does_intersect_grid(pos, camera_step, vec2(hig_scale)))    bgc = color(transition(zoom_anim_coef, 0.2, 0.4));
        else if (does_intersect_grid(pos, camera_step, vec2(mid_scale)))    bgc = color(transition(zoom_anim_coef, 0.4, 0.8));
        else if (does_intersect_grid(pos, camera_step, vec2(low_scale)))    bgc = color(transition(zoom_anim_coef, 0.8, 1.0));
    }

    out_color = value * vec4(0.8, 0.2, 0.1, 1.0) + (1.0 - value) * bgc;
}

float var_a(vec2 pos, vec2 step) {
return ((2.000000 * pos.x) + 3.000000);
}

float func_b(vec2 pos, vec2 step, float a, float b) {
return (pow(var_a(pos, step), b) + pos.y);
}

float uniq_2_0(vec2 pos, vec2 step) {
return (var_a(pos, step)) - (func_b(pos, step, 2.000000, 3.000000));
}

float uniq_3_0(vec2 pos, vec2 step) {
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

float function(vec2 pos, vec2 step) {
    return uniq_3_0(pos, step);
    // return sin(cos(tan(x * y))) - (sin(cos(tan(x))) + sin(cos(tan(y))));  // Большой фрактальный график
}

bool does_intersect(vec2 pos, vec2 step) {
    return function(pos, step) >= 1.0;
    /*float 
        lb = function(pos + vec2(0.0, step.y)), 
        rb = function(pos + step), 
        lt = function(pos), 
        rt = function(pos + vec2(step.x, 0.0));

    return
        sign_changes(lt, rt) ||
        sign_changes(lt, rb) ||
        sign_changes(lt, lb) ||

        sign_changes(lb, rb) ||
        sign_changes(lb, rt) ||

        sign_changes(rb, rt);*/
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

float render(vec2 pos, vec2 step) {
    return does_intersect(pos, step) ? 1.0 : 0.0;
}

float draw_msaa(vec2 pos) {
    float total_color = 0.0;
    pos -= camera_step;
    vec2 step = camera_step * 2;

    float bxy = float(int(pos.x + pos.y) & 1);
    float nbxy = 1. - bxy;
    
    // NAA x1
    ///=========
    if (MSAA == 1) {
        total_color = render(pos + step  * vec2(0.0), step);
    } else
    // MSAA x2
    ///=========
    if (MSAA == 2) {
        //step /= sqrt(2.0);
        total_color = (
            render(pos + step  * vec2(0.33 * nbxy, 0.), step) + 
            render(pos + step  * vec2(0.33 * bxy, 0.33), step)) / 2.;
    } else
    // MSAA x3
    ///=========
    if (MSAA == 3) {
        //step /= sqrt(2.0);
        total_color = (
            render(pos + step  * vec2(0.66 * nbxy, 0.), step) + 
            render(pos + step  * vec2(0.66 * bxy, 0.66), step) + 
            render(pos + step  * vec2(0.33, 0.33), step)) / 3.;
    } else
    // MSAA x4
    ///=========
    if (MSAA == 4) { // rotate grid
        //step /= 2.0;
        total_color = (
            render(pos + step  * vec2(0.33, 0.1), step) + 
            render(pos + step  * vec2(0.9, 0.33), step) + 
            render(pos + step  * vec2(0.66, 0.9), step) + 
            render(pos + step  * vec2(0.1, 0.66), step)) / 4.;
    } else
    // MSAA x5
    ///=========
    if (MSAA == 5) { // centre rotate grid
        //step /= 2.0;
        total_color = (
            render(pos + step  * vec2(0.33, 0.2), step) + 
            render(pos + step  * vec2(0.8, 0.33), step) + 
            render(pos + step  * vec2(0.66, 0.8), step) + 
            render(pos + step  * vec2(0.2, 0.66), step) + 
            render(pos + step  * vec2(0.5,  0.5), step)) / 5.;
    } else
    // SSAA x9
    ///=========
    if (MSAA == 9) {  // centre grid 3x3
        //step /= 3;
        total_color = (
        render(pos + step  * vec2(0.17,  0.2), step) + 
        render(pos + step  * vec2(0.17, 0.83), step) + 
        render(pos + step  * vec2(0.83, 0.17), step) + 
        render(pos + step  * vec2(0.83, 0.83), step) +
        render(pos + step  * vec2(0.5,  0.17), step) + 
        render(pos + step  * vec2(0.17,  0.5), step) + 
        render(pos + step  * vec2(0.5,  0.83), step) + 
        render(pos + step  * vec2(0.83,  0.5), step) +
        render(pos + step  * vec2(0.5,   0.5), step) * 2.) / 10.;
    } else
    // SSAA x16
    ///=========
    if (MSAA == 16) { // classic grid 4x4
        //step /= 4.0;
        total_color = (
            render(pos + step  * vec2(0.00, 0.00), step) + 
            render(pos + step  * vec2(0.25, 0.00), step) + 
            render(pos + step  * vec2(0.50, 0.00), step) + 
            render(pos + step  * vec2(0.75, 0.00), step) +
            render(pos + step  * vec2(0.00, 0.25), step) + 
            render(pos + step  * vec2(0.25, 0.25), step) + 
            render(pos + step  * vec2(0.50, 0.25), step) + 
            render(pos + step  * vec2(0.75, 0.25), step) +
            render(pos + step  * vec2(0.00, 0.50), step) + 
            render(pos + step  * vec2(0.25, 0.50), step) + 
            render(pos + step  * vec2(0.50, 0.50), step) + 
            render(pos + step  * vec2(0.75, 0.50), step) +
            render(pos + step  * vec2(0.00, 0.75), step) + 
            render(pos + step  * vec2(0.25, 0.75), step) + 
            render(pos + step  * vec2(0.50, 0.75), step) + 
            render(pos + step  * vec2(0.75, 0.75), step)) / 16.;
    }

    return total_color;
}
