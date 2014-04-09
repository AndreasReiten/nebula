__kernel void FRAME_FILTER(
    __write_only image2d_t xyzi_target,
//    __write_only image2d_t raw_target_clgl,
//    __write_only image2d_t corrected_target_clgl,
//    __write_only image2d_t gamma_target_clgl,
//    __read_only image2d_t tsf_source_clgl,
//    __read_only image2d_t background,
    __read_only image2d_t source,
    sampler_t tsf_sampler,
    sampler_t intensity_sampler,
    __constant float * sample_rotation_matrix,
    float2 threshold_one,
    float2 threshold_two,
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
    float h_omega,
    float max_intensity
    )
{
    // The frame has its axes like this, looking from the source to
    // the detector in the zero rotation position. We use the
    // cartiesian coordinate system described in
    // doi:10.1107/S0021889899007347
    //         y
    //         ^
    //         |
    //         |
    // z <-----x------ (fast)
    //         |
    //         |
    //       (slow)
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    int2 target_dim = get_image_dim(xyzi_target);

    /*
     * Background subtraction and lorentz polarization correction. Scaling to a common factor. Finally a flat min/max filter
     * */
    if ((id_glb.x < target_dim.x) && (id_glb.y < target_dim.y))
    {

        float intensity = read_imagef(source, intensity_sampler, (target_dim - id_glb - 1)).w; /* DANGER */

        // Write to alpha target (Shared CL/GL texture)
//        float tmp = intensity;
//        if (tmp < 1) tmp = 1;

//        float2 tsf_position = (float2)(native_divide(log10(tmp), log10(max_intensity)), 0.5f);
//        float4 sample = read_imagef(tsf_source_clgl, tsf_sampler, tsf_position);
//        write_imagef(raw_target_clgl, id_glb, sample);

        // Scale source to background
        intensity *= native_divide(background_flux * background_exposure_time, h_flux * h_exposure_time);

        // Subtract background
//        intensity -= read_imagef(background, intensity_sampler, id_glb).w;

        // Scale to a common flux and exposure time
        //intensity *= native_divide(common_flux * common_exposure_time, background_flux * background_exposure_time);

        // Flat min/max filter (threshold_one)
        if (((intensity < threshold_one.x) || (intensity > threshold_one.y)))
        {
            intensity = 0.0f;
        }
        
        // Write to beta target (Shared CL/GL texture)
//        tmp = intensity;
//        if (tmp < 1) tmp = 1;
        
//        if ((intensity < threshold_one.x) || (intensity > threshold_one.y))
//        {
//            sample = (float4)(0.1,0.0,1.0,0.7);
//        }
//        else
//        {
//            tsf_position = (float2)(native_divide(log10(tmp), log10(max_intensity)), 0.5f);
//            sample = read_imagef(tsf_source_clgl, tsf_sampler, tsf_position);
//        }
//        write_imagef(corrected_target_clgl, ide_glb, sample);

        float4 Q = (float4)(0.0f);
        if (intensity > 0.0f)
        {
            /*
             * Lorentz Polarization correction and distance correction + projecting the pixel onto the Ewald sphere
             * */
//            Q = (float4)(
//                -h_detector_distance,
//                h_pixel_size_x * (float) id_glb.y,
//                h_pixel_size_y * (float) id_glb.x,
//                intensity);

            float k = 1.0f/h_wavelength; // Multiply with 2pi if desired
            
            float3 k_i = (float3)(-k,0,0);
            float3 k_f = k*normalize((float3)(
                -h_detector_distance, 
                h_pixel_size_x * ((float) id_glb.y - h_beam_x), /* DANGER */
                h_pixel_size_y * ((float) id_glb.x - h_beam_y))); /* DANGER */
            
            Q.xyz = k_f - k_i; 
            Q.w = intensity;
            // Center the detector
//            Q.y -= h_beam_x * h_pixel_size_x; // Not sure if one should offset by half a pixel more/less
            // Which one of these is right, though?
//            Q.z -= h_beam_y * h_pixel_size_y;
//            Q.z -= (target_dim.x - h_beam_y) * h_pixel_size_y;

            // Titlt the detector around origo assuming it correctly coincides with the actual center of rotation ( not yet implemented)


            // Distance scale the intensity. The common scale here is the wavelength, so it should be constant across images
            //Q.w *= powr(fast_length(Q.xyz),2.0);

            // Project onto Ewald's sphere, moving to k-space
//            Q.xyz = k*normalize(Q.xyz);
            
            
            

            {
                // XYZ now has the direction of the scattered ray with respect to the incident one. This can be used to calculate the scattering angle for correction purposes. lab_theta and lab_phi are not to be confused with the detector/sample angles. These are simply the circular coordinate representation of the pixel position
                float lab_theta = asin(native_divide(Q.y, k));
                float lab_phi = atan2(Q.z,-Q.x);

                /* Lorentz Polarization correction - The Lorentz part will depend on the scanning axis, but has to be applied if the frames are integrated over some time */

                // Assuming rotation around the z-axis of the lab frame:
                float L = fabs(native_sin(lab_theta));

                // The polarization correction also needs a bit more work...
                Q.w *= L;
            }

            // This translation by k gives us the scattering vector Q, which is the reciprocal coordinate of the intensity. This step must be applied _after_ any detector rotation if such is used.
//            Q.x += k;
            

            // Sample rotation
            float3 temp = Q.xyz;

            Q.x = temp.x * sample_rotation_matrix[0] + temp.y * sample_rotation_matrix[1] + temp.z * sample_rotation_matrix[2];
            Q.y = temp.x * sample_rotation_matrix[4] + temp.y * sample_rotation_matrix[5] + temp.z * sample_rotation_matrix[6];
            Q.z = temp.x * sample_rotation_matrix[8] + temp.y * sample_rotation_matrix[9] + temp.z * sample_rotation_matrix[10];

            // Flat min/max filter (threshold_two)
            if (((Q.w < threshold_two.x) || (Q.w > threshold_two.y)))
            {
                Q.w = 0.0f;
            }
        }

        // Write to gamma target (Shared CL/GL texture)
//        tmp = Q.w;
//        if (tmp < 1) tmp = 1;
        
//        if ((Q.w < threshold_two.x) || (Q.w > threshold_two.y))
//        {
//            sample = (float4)(0.1,0.0,1.0,0.7);
//        }
//        else
//        {
//            tsf_position = (float2)(native_divide(log10(tmp), log10(max_intensity)), 0.5f);
//            sample = read_imagef(tsf_source_clgl, tsf_sampler, tsf_position);
//        }
//        write_imagef(gamma_target_clgl, id_glb, sample);

        // Write to the Q target (CL texture)
        write_imagef(xyzi_target, id_glb, Q);
    }
}
