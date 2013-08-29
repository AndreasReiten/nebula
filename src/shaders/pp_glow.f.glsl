uniform sampler2D texy;
in vec2 f_texpos;
in vec2 f_pixel_size;
in float f_scale;
in float f_deviation;
flat in int f_samples;
flat in int f_orientation;
out vec4 fcolor;


vec4 color = vec4(0.0);

float Gaussian (float x, float sigma)
{
	return (exp(-((x * x) / (2.0 * sigma * sigma))));	
}

void main ()
{
    float weight = 0;
    
	if ( f_orientation == 0 )
	{
		// Horizontal blur
		for (int i = -f_samples; i <= f_samples; ++i)
		{
            color += texture2D(texy, f_texpos + vec2(float(i) * f_pixel_size.x, 0.0)) * Gaussian(float(i), f_deviation) * f_scale;
            weight += Gaussian(float(i), f_deviation) * f_scale;
        }
	}
	else if ( f_orientation == 1 )
	{
		// Vertical blur
		for (int i = -f_samples; i <= f_samples; ++i)
		{
            color += texture2D(texy, f_texpos + vec2(0.0, float(i) * f_pixel_size.y)) * Gaussian(float(i), f_deviation) * f_scale;
            weight += Gaussian(float(i), f_deviation) * f_scale;
        }
	}
	else if ( f_orientation == 2 )
	{
		// Vertical blur
		for (int i = -f_samples; i <= f_samples; ++i)
		{
            for (int j = -f_samples; j <= f_samples; ++j)
            {
                color += texture2D(texy, f_texpos + vec2(float(i),float(j))  * f_pixel_size) * Gaussian(sqrt(float(i*i+j*j)), f_deviation) * f_scale;
                weight += Gaussian(sqrt(float(i*i+j*j)), f_deviation) * f_scale;
            }
        }
	}
    color *= (1.0/weight);
    //~ fcolor.w = 1.0;
	fcolor = clamp(color, 0.0, 1.0); 
}
