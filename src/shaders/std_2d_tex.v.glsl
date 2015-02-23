attribute vec2 fragpos;
attribute vec2 texpos;
varying vec2 f_texpos;
uniform mat4 transform;

void main(void)
{
    gl_Position =  transform * vec4(fragpos, 0.0, 1.0);
    f_texpos = texpos;
}

