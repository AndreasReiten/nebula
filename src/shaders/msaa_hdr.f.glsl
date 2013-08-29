smooth in vec2 vertOutTexCoords;
 
uniform sampler2DMS origImage;
uniform samplerBuffer sampleWeightSampler;
uniform int sampleCount; 
uniform int useWeightedResolve; 
uniform float exposure;
 
out vec4 outColor;
 
// Tone mapping
vec4 toneMap (vec4 hdrColor)
{
    vec4 ldrColor = 1.f - exp2 (-hdrColor * exposure);
    //~ ldrColor.a = 1.f;
 
    return ldrColor;
}
 
void main(void)
{
    // Calculate un-normalized texture coordinates
    vec2 tmp = floor (textureSize (origImage) * vertOutTexCoords);
 
    // find both the weighted and unweighted colors
    vec4 color = vec4 (0.f, 0.f, 0.f, 1.f);
    vec4 weightedColor = vec4 (0.f, 0.f, 0.f, 1.f);
 
    for (int i = 0; i < sampleCount; ++i)
    {
        // Get the weight for this sample from the TBO, this changes based on
        // the number of samples
        float weight = texelFetch (sampleWeightSampler, i);
 
        // Tone-map the HDR texel before it is weighted
        vec4 sample = toneMap (texelFetch (origImage, ivec2 (tmp), i));
 
        weightedColor += sample * weight;
        color += sample;
    }
 
    // Now, decide on the type of resolve to perform
    outColor = weightedColor;
 
    // If the user selected the unweighted resolve, output the equally weighted
    // value
    if (useWeightedResolve == 0)
    {
        outColor = color / sampleCount;
    }
 
    //~ outColor.a = 1.f;
}
