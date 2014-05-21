__kernel void imagePreview(
    __write_only image2d_t preview,
    __read_only image2d_t source,
    __read_only image2d_t tsf_source,
    __constant float * parameter,
    sampler_t tsf_sampler,
    sampler_t intensity_sampler,
    int mode
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

    // Thresholds and other parameters essential to the file
    float th_a_low = parameter[0];
    float th_a_high = parameter[1];
    float th_b_low = parameter[2];
    float th_b_high = parameter[3];
    float flux = parameter[4];
    float exp_time = parameter[5];
    float wavelength = parameter[6];
    float det_dist = parameter[7];
    float beam_x = parameter[8];
    float beam_y = parameter[9];
    float pix_size_x = parameter[10];
    float pix_size_y = parameter[11];
    float intensity_min = parameter[12];
    float intensity_max = parameter[13];
    

    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    int2 target_dim = get_image_dim(preview);

    if ((id_glb.x < target_dim.x) && (id_glb.y < target_dim.y))
    {

        float intensity = read_imagef(source, intensity_sampler, (target_dim - id_glb - 1)).w; /* DANGER */
            
        // Flat min/max filter (threshold_one)
        if (((intensity < th_a_low) || (intensity > th_a_high)))
        {
            intensity = 0.0f;
        }

        if (mode == 0)
        {
            float tmp = intensity;
            if (tmp < 1) tmp = 1;
    
            float2 tsf_position = (float2)(native_divide(log10(tmp) - log10(intensity_min), log10(intensity_max)-log10(intensity_min)), 0.5f);
            float4 sample = read_imagef(tsf_source, tsf_sampler, tsf_position);

            write_imagef(preview, id_glb, sample);
        }
        else if (mode == 1)
        {
            float4 Q = (float4)(0.0f);
            if (intensity > 0.0f)
            {
                float k = 1.0f/wavelength; // Multiply with 2pi if desired
    
                float3 k_i = (float3)(-k,0,0);
                float3 k_f = k*normalize((float3)(
                    -det_dist,
                    pix_size_x * ((float) id_glb.y - beam_x), /* DANGER */
                    pix_size_y * ((float) id_glb.x - beam_y))); /* DANGER */
    
                Q.xyz = k_f - k_i;
                Q.w = intensity;
    
                {
                    float lab_theta = asin(native_divide(Q.y, k));
                    float lab_phi = atan2(Q.z,-Q.x);
    
                    // Assuming rotation around the z-axis of the lab frame:
                    float L = fabs(native_sin(lab_theta));
    
                    // The polarization correction needs a bit more work...
                    Q.w *= L;
                }
    
                // Flat min/max filter (threshold_two)
                if (((Q.w < th_b_low) || (Q.w > th_b_high)))
                {
                    Q.w = 0.0f;
                }
            }

            float tmp = Q.w;
            if (tmp < 1) tmp = 1;
    
            float2 tsf_position = (float2)(native_divide(log10(tmp) - log10(intensity_min), log10(intensity_max)-log10(intensity_min)), 0.5f);
            float4 sample = read_imagef(tsf_source, tsf_sampler, tsf_position);
            
            write_imagef(preview, id_glb, sample);
        }
    }
}
