in vec2 position;
uniform vec4 color;
out vec4 f_color;

void main(void)
{
    gl_Position =  vec4(position, 0.0, 1.0);
    f_color = color;
}

