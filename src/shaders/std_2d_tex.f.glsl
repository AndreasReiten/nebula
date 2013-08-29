uniform sampler2D texy;
in vec2 f_texpos;
out vec4 fcolor;

void main(void) 
{
    fcolor = texture2D(texy, f_texpos);
}
