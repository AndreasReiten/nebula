attribute vec2 fragpos;
uniform mat4 transform;
varying vec2 f_fragpos;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 0.0, 1.0);
//    f_fragpos = fragpos;
}

