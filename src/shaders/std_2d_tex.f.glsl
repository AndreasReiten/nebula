#version 400

uniform sampler2D texture;
in vec2 f_texpos;

void main(void)
{
    gl_FragColor = texture2D(texture, vec2(f_texpos.x, 1.0 - f_texpos.y));
}
