attribute highp vec4 fragpos;
uniform highp mat4 transform;

void main(void)
{
    gl_Position = transform * fragpos;
}

