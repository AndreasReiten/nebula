float Gaussian (float x, float sigma)
{
	return (exp(-((x * x) / (2.0 * sigma * sigma))));	
}

__kernel void FRAME_FILTER(
    __write_only image2d_t i_target,
    __write_only image2d_t xyzi_target,
    __read_only image2d_t background,
    __read_only image2d_t source,
    sampler_t intensity_sampler,
    float2 threshold,
    float background_flux,
    float background_exposure_time,
    float h_pixel_size_x,
    float h_pixel_size_y,
    float h_exposure_time,
    float h_wavelength,
    float h_detector_distance,
    float h_beam_x,
    float h_beam_y,
    float h_flux,
    float h_start_angle,
    float h_angle_increment,
    float h_kappa,
    float h_phi,
    float h_omega
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
        
        float intensity = read_imagef(source, intensity_sampler, id_glb).w;

        // Scale source to background
        intensity *= native_divide(background_flux * background_exposure_time, h_flux * h_exposure_time);

        // Subtract background
        intensity -= read_imagef(background, intensity_sampler, id_glb).w;

        // Scale to a common flux and exposure time
        //~ intensity *= native_divide(common_flux * common_exposure_time, background_flux * background_exposure_time);
        
        // Flat min/max filter
        if (((intensity < threshold.x) || (intensity > threshold.y))) intensity = 0.0f; 

        // Write the corrected result to an image for display in the viewer
        float4 iiii = (float4)(intensity);
        write_imagef(i_target, id_glb, iiii);

        // Project the pixel onto the Ewald sphere
        float4 xyzi;
        write_imagef(xyzi_target, id_glb, xyzi);
    }
}
