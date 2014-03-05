attribute highp vec3 fragpos;
uniform highp mat4 transform;
uniform highp vec3 lim_low;
uniform highp vec3 lim_high;
uniform highp float unitcell_diagonal;
uniform lowp vec4 color;
varying lowp vec4 f_color;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 1.0);
    
    if (((fragpos.x <= lim_low.x - unitcell_diagonal) || (fragpos.x >= lim_high.x + unitcell_diagonal)) || ((fragpos.y <= lim_low.y - unitcell_diagonal) || (fragpos.y >= lim_high.y + unitcell_diagonal)) || ((fragpos.z <= lim_low.z - unitcell_diagonal) || (fragpos.z >= lim_high.z + unitcell_diagonal)))
    {
        f_color = vec4(1,0,1,0);
    }
    else
    {
        f_color = color;
    }
}

