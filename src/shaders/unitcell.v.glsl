attribute highp vec3 fragpos;
uniform highp mat4 transform;
uniform highp vec3 lim_low;
uniform highp vec3 lim_high;
uniform lowp vec4 color;
varying lowp vec4 f_color;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 1.0);
    
    if (((gl_Position.x <= lim_low.x) || (gl_Position.x >= lim_high.x)) || ((gl_Position.y <= lim_low.y) || (gl_Position.y >= lim_high.y)) || ((gl_Position.z <= lim_low.z) || (gl_Position.z >= lim_high.z)))
    {
        f_color.xyz = vec3(1.0,1.0,1.0) - color.xyz;
        f_color.w = color.w;
    }
    else
    {
        f_color = color;
    }
}

