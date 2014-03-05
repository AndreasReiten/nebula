attribute highp vec2 fragpos;
uniform highp mat2 transform;

void main(void)
{
    gl_Position = vec4(transform * fragpos, 0.0, 1.0);
}

