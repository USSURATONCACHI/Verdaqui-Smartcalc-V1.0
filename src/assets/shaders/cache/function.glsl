float uniq_0_0(vec2 pos, vec2 step){
return (sin(cos(tan((pos.x * pos.y))))) - ((sin(cos(tan(pos.x))) + sin(cos(tan(pos.y)))));
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

float function(vec2 pos, vec2 step) {
return uniq_1_0(pos, step);
}
