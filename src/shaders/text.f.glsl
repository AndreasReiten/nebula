uniform sampler2D tex;
in vec4 f_color;
in vec2 f_texpos;
out vec4 fcolor;

void main(void) 
{
    fcolor = vec4(1, 1, 1, texture2D(tex, f_texpos).r) * f_color;
}
