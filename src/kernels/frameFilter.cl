float Gaussian (float x, float sigma)
{
	return (exp(-((x * x) / (2.0 * sigma * sigma))));	
}

__kernel void FRAME_FILTER(
    __write_only image2d_t target,
    __read_only image2d_t background,
    __read_only image2d_t source,
    sampler_t source_sampler,
    float2 threshold,
    float source_flux,
    float source_exposure_time,
    float background_flux,
    float background_exposure_time
    )
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));

    int2 target_dim = get_image_dim(target);
    int2 source_dim = get_image_dim(source);

    /*
     * Background subtraction and lorentz polarization correction. Scaling to a common factor. Finally a flat min/max filter
     * */
    if ((id_glb.x < target_dim.x) && (id_glb.y < target_dim.y))
    {
        
        float intensity = read_imagef(source, source_sampler, id_glb).w;

        // Scale source to background
        intensity *= native_divide(background_flux * background_exposure_time, source_flux * source_exposure_time);

        // Subtract background
        intensity -= read_imagef(background, source_sampler, id_glb).w;

        // Scale to a common flux and exposure time
        //~ intensity *= native_divide(common_flux * common_exposure_time, background_flux * background_exposure_time);
        
        // Flat min/max filter
        if (((intensity < threshold.x) || (intensity > threshold.y))) intensity = 0.0f; 
        
        float4 sample = (float4)(intensity);
        
        write_imagef(target, id_glb, sample);
    }
}
