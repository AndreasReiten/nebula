uniform sampler2DMS texy;
uniform samplerBuffer sampleWeightSampler;
in vec2 f_texpos;
flat in int f_samples;
out vec4 fcolor;

void main(void)
{
    vec2 tmp = floor(textureSize(texy) * f_texpos);

    fcolor = vec4(0.0,0.0,0.0,0.0);
    for (int i = 0; i < f_samples; i++)
    {
        fcolor +=  texelFetch(sampleWeightSampler, i) * texelFetch(texy, ivec2(tmp), i);
    }
}
