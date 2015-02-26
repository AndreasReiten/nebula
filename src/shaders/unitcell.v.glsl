attribute  vec3 fragpos;
uniform  mat4 transform;
uniform  mat4 u;
uniform  vec3 lim_low;
uniform  vec3 lim_high;
uniform  vec4 color;
varying  vec4 f_color;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 1.0);
    
    vec4 pos = u * vec4(fragpos, 1.0);
    
    if ((pos.x <= lim_low.x) || (pos.x >= lim_high.x) || (pos.y <= lim_low.y) || (pos.y >= lim_high.y) || (pos.z <= lim_low.z) || (pos.z >= lim_high.z))
    {
        f_color = vec4(0,0,0,0);
    }
    else
    {
        f_color = color;
    }
}

