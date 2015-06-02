#version 400

uniform sampler2D texture;
in vec2 f_texpos;

void main(void)
{ 
    gl_FragColor = texture2D(texture, f_texpos);
}
