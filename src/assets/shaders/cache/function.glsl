float func_w(vec2 pos, vec2 step, float arg_x){
return (arg_x * pow(2.72, arg_x));
}

float uniq_1_0(vec2 pos, vec2 step){
return (pos.y) - (func_w(pos, step, log(log(pos.x)/log(2.71828182846))/log(2.71828182846)));
}

float uniq_2_0(vec2 pos, vec2 step){
float 
    lb = uniq_1_0(pos + vec2(0.0, step.y), step), 
    rb = uniq_1_0(pos + step, step), 
    lt = uniq_1_0(pos, step), 
    rt = uniq_1_0(pos + vec2(step.x, 0.0), step);

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
return uniq_2_0(pos, step);
}
