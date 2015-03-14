#version 400

in vec2 fragpos;
uniform mat4 transform;
out vec2 f_fragpos;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 0.0, 1.0);
}

