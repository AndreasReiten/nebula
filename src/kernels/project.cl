__kernel void FRAME_FILTER(
    __write_only image2d_t xyzi_target,
    __global float * source,
    sampler_t tsf_sampler,
//    sampler_t intensity_sampler,
    __constant float * sample_rotation_matrix,
//    float2 threshold_one,
//    float2 threshold_two,
//    float background_flux,
//    float background_exposure_time,
    float h_pixel_size_x,
    float h_pixel_size_y,
//    float h_exposure_time,
    float h_wavelength,
    float h_detector_distance,
    float h_beam_x,
    float h_beam_y,
//    float h_flux,
    float h_start_angle,
    float h_angle_increment,
    float h_kappa,
    float h_phi,
    float h_omega,
//    float max_intensity,
    int4 selection
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
        float4 Q = (float4)(0.0f);
        
        if ((id_glb.x >= selection.x) && // Left 
            (id_glb.x < selection.y) && // Right
            (id_glb.y >= selection.z) && // Top
            (id_glb.y < selection.w)) // Bottom
        {
            Q.w = source[id_glb.y*target_dim.x + id_glb.x]; //id_glb.y * image_size.x + id_glb.x; //
//            read_imagef(source, intensity_sampler, id_glb).w; /* DANGER */
            
            // Noise filter
//            intensity = clamp(intensity, threshold_one.x, threshold_one.y); // All readings within noise thresholds
//            intensity -= threshold_one.x; // Subtracts the noise
            
            if (Q.w > 0.0f)
            {
                /*
                 * Lorentz Polarization correction and distance correction + projecting the pixel onto the Ewald sphere
                 * */
                float k = 1.0f/h_wavelength; // Multiply with 2pi if desired
                

                //float3 k_f = k*normalize((float3)(
                  //  -det_dist,
                  //  pix_size_x * ((float) (image_size.y - id_glb.y) - beam_x), /* DANGER */
                  //  pix_size_y * ((float) id_glb.x - beam_y))); /* DANGER */

                float3 k_i = (float3)(-k,0,0);
                float3 k_f = k*normalize((float3)(
                    -h_detector_distance, 
                    h_pixel_size_x * ((float) (target_dim.y - 1 - id_glb.y) - h_beam_x), /* DANGER */
                    h_pixel_size_y * ((float) (target_dim.x - 1 - id_glb.x) - h_beam_y))); /* DANGER */
                
                Q.xyz = k_f - k_i; 
                
                // Titlt the detector around origo assuming it correctly coincides with the actual center of rotation ( not yet implemented)
    
//                {
                    // XYZ now has the direction of the scattered ray with respect to the incident one. This can be used to calculate the scattering angle for correction purposes. lab_theta and lab_phi are not to be confused with the detector/sample angles. These are simply the circular coordinate representation of the pixel position
//                    float lab_theta = asin(native_divide(Q.y, k));
//                    float lab_phi = atan2(Q.z,-Q.x);
    
                    /* Lorentz Polarization correction - The Lorentz part will depend on the scanning axis, but has to be applied if the frames are integrated over some time */
    
                    // Assuming rotation around the z-axis of the lab frame:
//                    float L = fabs(native_sin(lab_theta));
    
                    // The polarization correction also needs a bit more work...
//                    Q.w *= L;
//                }
    
                // Sample rotation
                float3 temp = Q.xyz;
    
                Q.x = temp.x * sample_rotation_matrix[0] + temp.y * sample_rotation_matrix[1] + temp.z * sample_rotation_matrix[2];
                Q.y = temp.x * sample_rotation_matrix[4] + temp.y * sample_rotation_matrix[5] + temp.z * sample_rotation_matrix[6];
                Q.z = temp.x * sample_rotation_matrix[8] + temp.y * sample_rotation_matrix[9] + temp.z * sample_rotation_matrix[10];
    
                // Post correction filter (Note: remove this at some point, it has little logical purpose)
//                Q.w = clamp(Q.w, threshold_two.x, threshold_two.y); 
//                Q.w -= threshold_two.x; 
                
            }
            
//            Q.w = 11.0f;
    
        }    
        else 
        {
            Q.w = 0.0f;
        }
        
        // Write to the Q target (CL texture)
        write_imagef(xyzi_target, id_glb, Q);
    }
}
