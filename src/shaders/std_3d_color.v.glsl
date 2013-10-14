attribute highp vec4 fragpos;
attribute highp vec2 texpos;
uniform highp mat4 transform;
varying highp vec2 f_texpos;

void main(void)
{
    gl_Position = transform * fragpos;
    f_texpos = texpos;
}

