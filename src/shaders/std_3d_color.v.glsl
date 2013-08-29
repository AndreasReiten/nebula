uniform vec4 color;
uniform mat4 transform;
uniform int time;
in vec4 position;
in vec2 texpos;
out vec2 f_texpos;
out vec4 f_color;
flat out int f_time;

void main(void)
{
    gl_Position = transform * position;
    f_color = color;
    f_texpos = texpos;
    f_time = time;
}

