in vec2 position;
in vec2 texpos;
uniform vec4 color;
out vec2 f_texpos;
out vec4 f_color;

void main(void) 
{
    gl_Position = vec4(position, 0, 1);
    f_texpos = texpos;
    f_color = color;
}
