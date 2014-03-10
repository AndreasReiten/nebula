attribute highp vec3 fragpos;
uniform highp mat4 transform;
uniform highp vec3 lim_low;
uniform highp vec3 lim_high;
uniform lowp vec4 color;
varying lowp vec4 f_color;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 1.0);
    
    if (((fragpos.x <= lim_low.x) || (fragpos.x >= lim_high.x)) || ((fragpos.y <= lim_low.y) || (fragpos.y >= lim_high.y)) || ((fragpos.z <= lim_low.z) || (fragpos.z >= lim_high.z)))
    {
        f_color = vec4(0.2,1,0.2,0);
    }
    else
    {
        f_color = color;
    }
}

