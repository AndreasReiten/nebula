uniform sampler2D texture;
varying vec2 f_texpos;

void main(void)
{
    vec2 flipped_texpos = vec2(f_texpos.x, 1.0 - f_texpos.y); // The OpenGL convention (origin at the bottom-left corner) is different than in Qt applications (origin at the top-left corner). This should be reviewed later. 
    gl_FragColor = texture2D(texture, flipped_texpos);
}
