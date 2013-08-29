uniform sampler2D texy;
in vec4 f_color;
in vec2 f_texpos;
flat in int f_time;
out vec4 fcolor;

void main(void) 
{
    fcolor = f_color;
}
