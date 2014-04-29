uniform highp sampler2D texture;
varying highp vec2 f_texpos;

void main(void)
{
    gl_FragColor = texture2D(texture, f_texpos);
}
