uniform vec4 color;
//varying vec2 f_fragpos;

void main(void)
{
//    color.xyz -= (vec3)(1.0,0.0,0.5)*(mod(f_fragpos.x,0.02f)+mod(-f_fragpos.y,-0.02f))*50;
    gl_FragColor = color;
}
