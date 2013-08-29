float Gaussian (float x, float sigma)
{
	return (exp(-((x * x) / (2.0 * sigma * sigma))));	
}

__kernel void FRAME_FILTER(
    __write_only image2d_t target,
    __read_only image2d_t source,
    sampler_t source_sampler,
    int2 threshold
    )
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));

    int2 target_dim = get_image_dim(target);
    int2 source_dim = get_image_dim(source);
    
    if ((id_glb.x < target_dim.x) && (id_glb.y < target_dim.y))
    {
        //~ // Calculate mean and variance using Gaussian weighting
        //~ float mean = 0;
        //~ float variance = 0;
        //~ float weight = 0;
        //~ int width = 2;
        //~ float std_var = 0.7;
        //~ 
        //~ for (int i = -width; i <= width; i+=1)
        //~ {
            //~ for (int j = -width; j <= width; j+=1)
            //~ {
                //~ mean += read_imagef(source, source_sampler, id_glb + (int2)(i,j)).w * Gaussian(sqrt((float)(i*i + j*j)), 3.0);
                //~ weight += Gaussian(sqrt((float)(i*i + j*j)), std_var);
            //~ }
        //~ }
//~ 
        //~ mean = native_divide(mean, weight);
        //~ 
        //~ for (int i = -width; i <= width; i+=4)
        //~ {
            //~ for (int j = -width; j <= width; j+=4)
            //~ {
                //~ variance += pow((read_imagef(source, source_sampler, id_glb + (int2)(i,j)).w - mean), 2.0) * Gaussian(sqrt((float)(i*i + j*j)), std_var);
            //~ }
        //~ }
//~ 
        //~ variance = sqrt(native_divide(variance, weight));
        
        // Estimate background based on mean and variance
        //~ float background = mean - variance;

        // Subtract the background from the original value
        //~ float corrected_intensity = read_imagef(source, source_sampler, id_glb).w - background;

        //~ corrected_intensity = max(0.0f, corrected_intensity); 
        
        //~ float4 sample = (float4)(corrected_intensity);





        
        float intensity = read_imagef(source, source_sampler, id_glb).w;

        if (((intensity > threshold.x) && (intensity < threshold.y))) ; // && (native_divide(variance, mean) > 0.15)
        else intensity = 0.0f;
        float4 sample = (float4)(intensity);
        
        write_imagef(target, id_glb, sample);
    }
}
