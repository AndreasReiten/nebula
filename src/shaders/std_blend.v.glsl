#version 400

in  vec2 fragpos;
in  vec2 texpos;
out  vec2 f_texpos;


void main(void)
{
    gl_Position =  vec4(fragpos, 0.0, 1.0);
    f_texpos = texpos;
}

