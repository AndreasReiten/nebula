uniform highp sampler2D texture_alpha;
uniform highp sampler2D texture_beta;
uniform lowp int method;
varying highp vec2 f_texpos;

void main ()
{
    vec4 alpha = texture2D(texture_alpha, f_texpos);
    vec4 beta = texture2D(texture_beta, f_texpos);

    if ( method == 0 )
    {
        // Additive blending (strong result, high overexposure)
        gl_FragColor = min(alpha + beta, 1.0);
    }
    else if ( method == 1 )
    {
        // Screen blending (mild result, medium overexposure)
        gl_FragColor = clamp((alpha + beta) - (alpha * beta), 0.0, 1.0);
        gl_FragColor.w = 1.0;
    }
    else
    {
        gl_FragColor = alpha;
    }
}
