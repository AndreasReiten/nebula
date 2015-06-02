#version 400

in vec2 fragpos;
uniform mat4 transform;
in vec2 texpos;
out vec2 f_texpos;

void main(void)
{
    gl_Position =  transform * vec4(fragpos, 0.0, 1.0);

    f_texpos = texpos;
}

