in vec2 position;
in vec2 texpos;
out vec2 f_texpos;

void main(void)
{
    gl_Position =  vec4(position, 0.0, 1.0);
    f_texpos = texpos;
}

