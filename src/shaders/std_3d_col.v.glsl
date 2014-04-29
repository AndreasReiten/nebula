attribute highp vec3 fragpos;
uniform highp mat4 transform;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 1.0);
}

