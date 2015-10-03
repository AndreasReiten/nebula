kernel void scatteringDataToImage(
    global float * in_buf,
    write_only image2d_t out_image,
    read_only image2d_t tsf_image,
    sampler_t tsf_sampler,
    float2 data_limit,
    int log
)
{
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));
    int2 image_dim = get_image_dim(out_image);

    if ((id_glb.x < image_dim.x) && (id_glb.y < image_dim.y))
    {
        float intensity = in_buf[id_glb.y * image_dim.x + id_glb.x];

        float2 tsf_position;
        float4 sample;

        if (log)
        {
            // If the lower data limit is less than one (but not less than zero), use a proportional part of the color scale for values between this lower limit and one, using instead a linear scale.
            if (data_limit.x < 1.0)
            {
                float linear_fraction = (1.0 - data_limit.x) / (log10(data_limit.y) + (1.0 - data_limit.x));
                float log10_fraction = log10(data_limit.y) / (log10(data_limit.y) + (1.0 - data_limit.x));

                if (intensity < 0.0f)
                {
                    tsf_position = (float2)(0.0f, 0.5f);
                }
                else if (intensity < 1.0)
                {
                    // Linear regime
                    tsf_position = (float2)(native_divide(intensity - data_limit.x, 1.0f - data_limit.x)*linear_fraction, 0.5f);
                }
                else
                {
                    // Logarithmic regime
                    tsf_position = (float2)(linear_fraction + native_divide(log10(intensity), log10(data_limit.y))*log10_fraction, 0.5f);
                }
            }
            else
            {
                if (intensity < data_limit.x)
                {
                    tsf_position = (float2)(0.0f, 0.5f);
                }
                else
                {
                    tsf_position = (float2)(native_divide(log10(intensity) - log10(data_limit.x), log10(data_limit.y) - log10(data_limit.x)), 0.5f);
                }
            }
        }
        else
        {
            tsf_position = (float2)(native_divide(intensity - data_limit.x, data_limit.y - data_limit.x), 0.5f);
        }

        sample = read_imagef(tsf_image, tsf_sampler, tsf_position);
        write_imagef(out_image, id_glb, sample);
    }
}

kernel void correctScatteringData(
    global float * in_buf,
    global float * out_buf,
    int2 image_size,
    int isCorrectionLorentzActive,
    int isCorrectionNoiseActive,
    int isCorrectionPlaneActive,
    int isCorrectionPolarizationActive,
    int isCorrectionFluxActive,
    int isCorrectionExposureActive,
    int isCorrectionPixelProjectionActive,
    float detector_distance,
    float beam_center_x,
    float beam_center_y,
    float pixel_size_x,
    float pixel_size_y,
    float wavelength,
    float flux,
    float exposure_time,
    float noise_low
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
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));

    if ((id_glb.x < image_size.x) && (id_glb.y < image_size.y))
    {
        float4 Q = (float4)(0.0f);
        Q.w = in_buf[id_glb.y * image_size.x + id_glb.x];

        // Flat background subtraction
        if (isCorrectionNoiseActive)
        {
            Q.w = clamp(Q.w, noise_low, Q.w); // All readings within thresholds
            Q.w -= noise_low; // Subtract
        }

        // Planar background subtraction
//        if (isCorrectionPlaneActive)
//        {
//            float plane_z = -(plane.x * (float)id_glb.x + plane.y * (float)id_glb.y + plane.w) / plane.z;

//            if (plane_z < 0)
//            {
//                plane_z = 0;    // Negative values do not make sense
//            }

//            Q.w = clamp(Q.w, plane_z, noise_high); // All readings within thresholds
//            Q.w -= plane_z;
//        }

        // The real space vector OP going from the origo (O) to the pixel (P)
        float3 OP = (float3)(
                                        -detector_distance,
                                        pixel_size_x * ((float) (image_size.y - id_glb.y - 0.5) - beam_center_x), /* DANGER */
                                        //pixel_size_y * ((float) (image_size.x - 0.5f - id_glb.x) - beam_center_y)
                                        pixel_size_y * ((float) -((id_glb.x + 0.5) - beam_center_y))); /* DANGER */

        float k = 1.0 / wavelength; // Multiply with 2pi if desired

        /* Corrections */
        // Correct for the area of the projection of the pixel onto the Ewald sphere
        if (isCorrectionPixelProjectionActive)
        {
            float forward_projected_area;
            {
                // The four vectors that define projected pixel on the Ewald sphere
                float3 a_vec = k * normalize((float3)(-detector_distance, (float) pixel_size_x * 0.5, -(float) pixel_size_y * 0.5));
                float3 b_vec = k * normalize((float3)(-detector_distance, -(float) pixel_size_x * 0.5, -(float) pixel_size_y * 0.5));
                float3 c_vec = k * normalize((float3)(-detector_distance, -(float) pixel_size_x * 0.5, (float) pixel_size_y * 0.5));
                float3 d_vec = k * normalize((float3)(-detector_distance, (float) pixel_size_x * 0.5, (float) pixel_size_y * 0.5));

                // The area of the two spherical triangles spanned by the projected pixel is approximated by their corresponding planar triangles since they are small
                float3 ab_vec = b_vec - a_vec;
                float3 ac_vec = c_vec - a_vec;
                float3 ad_vec = d_vec - a_vec;

                forward_projected_area = 0.5*fabs(length(cross(ab_vec,ac_vec))) + 0.5*fabs(length(cross(ac_vec,ad_vec)));
            }

            float projected_area;
            {
                // The four vectors that define projected pixel on the Ewald sphere
                float3 a_vec = k * normalize( convert_float3(OP) + (float3)(0, (float) pixel_size_x * 0.5, -(float) pixel_size_y * 0.5));
                float3 b_vec = k * normalize( convert_float3(OP) + (float3)(0, -(float) pixel_size_x * 0.5, -(float) pixel_size_y * 0.5));
                float3 c_vec = k * normalize( convert_float3(OP) + (float3)(0, -(float) pixel_size_x * 0.5, (float) pixel_size_y * 0.5));
                float3 d_vec = k * normalize( convert_float3(OP) + (float3)(0, (float) pixel_size_x * 0.5, (float) pixel_size_y * 0.5));

                // The area of the two spherical triangles spanned by the projected pixel is approximated by their corresponding planar triangles since they are small
                float3 ab_vec = b_vec - a_vec;
                float3 ac_vec = c_vec - a_vec;
                float3 ad_vec = d_vec - a_vec;

                projected_area = 0.5*fabs(length(cross(ab_vec,ac_vec))) + 0.5*fabs(length(cross(ac_vec,ad_vec)));
            }

            // Correction
            Q.w = Q.w * ( forward_projected_area / projected_area);
        }


        float3 k_i = (float3)(-k, 0, 0);
        float3 k_f = k * normalize(OP);


        Q.xyz = k_f - k_i;

        {
            // Lorentz correction assuming rotation around a given axis (corresponding to the rotation of a single motor in most cases)
            // The expression is derived from the formula found in the Nebula open access article
            if (isCorrectionLorentzActive)
            {
                float3 axis_rot = (float3)(0.0f,0.0f,1.0f); // Omega rotation. Todo: Set as kernel input argument and define in host application
                Q.w *= wavelength*fabs((dot(cross(normalize(axis_rot), Q.xyz),normalize(k_f))));
            }

            // Polarization correction begs implementation
        }

        out_buf[id_glb.y * image_size.x + id_glb.x] = Q.w;
    }
}


kernel void processScatteringData(
    global float * in_buf,
    global float * out_buf,
    constant float * parameter,
    int2 image_size,
    int isCorrectionLorentzActive,
    int task,
    float mean,
    float deviation,
    int isCorrectionNoiseActive,
    int isCorrectionPlaneActive,
    int isCorrectionPolarizationActive,
    int isCorrectionFluxActive,
    int isCorrectionExposureActive,
    float4 plane,
    int isCorrectionPixelProjectionActive
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
    float noise_low = parameter[0];
    float noise_high = parameter[1];
    float pct_low = parameter[2]; // Post correction threshold
    float pct_high = parameter[3];
    float flux = parameter[4];
    float exp_time = parameter[5];
    float wavelength = parameter[6];
    float det_dist = parameter[7];
    float beam_x = parameter[8];
    float beam_y = parameter[9];
    float pix_size_x = parameter[10];
    float pix_size_y = parameter[11];

    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));

    if ((id_glb.x < image_size.x) && (id_glb.y < image_size.y))
    {
        float4 Q = (float4)(0.0f);
        Q.w = in_buf[id_glb.y * image_size.x + id_glb.x];

        if (task == 0)
        {
            // Flat background subtraction
            if (isCorrectionNoiseActive)
            {
                Q.w = clamp(Q.w, noise_low, noise_high); // All readings within thresholds
                Q.w -= noise_low; // Subtract
            }

            // Planar background subtraction
            if (isCorrectionPlaneActive)
            {
                float plane_z = -(plane.x * (float)id_glb.x + plane.y * (float)id_glb.y + plane.w) / plane.z;

                if (plane_z < 0)
                {
                    plane_z = 0;    // Negative values do not make sense
                }

                Q.w = clamp(Q.w, plane_z, noise_high); // All readings within thresholds
                Q.w -= plane_z;
            }

            // The real space vector OP going from the origo (O) to the pixel (P)
            float3 OP = (float3)(
                                            -det_dist,
                                            pix_size_x * ((float) (image_size.y - id_glb.y - 0.5) - beam_x), /* DANGER */
                                            //pix_size_y * ((float) (image_size.x - 0.5f - id_glb.x) - beam_y)
                                            pix_size_y * ((float) -((id_glb.x + 0.5) - beam_y))); /* DANGER */

            float k = 1.0 / wavelength; // Multiply with 2pi if desired

            /* Corrections */
            // Correct for the area of the projection of the pixel onto the Ewald sphere
            if (isCorrectionPixelProjectionActive)
            {
                float forward_projected_area;
                {
                    // The four vectors that define projected pixel on the Ewald sphere
                    float3 a_vec = k * normalize((float3)(-det_dist, (float) pix_size_x * 0.5, -(float) pix_size_y * 0.5));
                    float3 b_vec = k * normalize((float3)(-det_dist, -(float) pix_size_x * 0.5, -(float) pix_size_y * 0.5));
                    float3 c_vec = k * normalize((float3)(-det_dist, -(float) pix_size_x * 0.5, (float) pix_size_y * 0.5));
                    float3 d_vec = k * normalize((float3)(-det_dist, (float) pix_size_x * 0.5, (float) pix_size_y * 0.5));

                    // The area of the two spherical triangles spanned by the projected pixel is approximated by their corresponding planar triangles since they are small
                    float3 ab_vec = b_vec - a_vec;
                    float3 ac_vec = c_vec - a_vec;
                    float3 ad_vec = d_vec - a_vec;

                    forward_projected_area = 0.5*fabs(length(cross(ab_vec,ac_vec))) + 0.5*fabs(length(cross(ac_vec,ad_vec)));
                }

                float projected_area;
                {
                    // The four vectors that define projected pixel on the Ewald sphere
                    float3 a_vec = k * normalize( convert_float3(OP) + (float3)(0, (float) pix_size_x * 0.5, -(float) pix_size_y * 0.5));
                    float3 b_vec = k * normalize( convert_float3(OP) + (float3)(0, -(float) pix_size_x * 0.5, -(float) pix_size_y * 0.5));
                    float3 c_vec = k * normalize( convert_float3(OP) + (float3)(0, -(float) pix_size_x * 0.5, (float) pix_size_y * 0.5));
                    float3 d_vec = k * normalize( convert_float3(OP) + (float3)(0, (float) pix_size_x * 0.5, (float) pix_size_y * 0.5));

                    // The area of the two spherical triangles spanned by the projected pixel is approximated by their corresponding planar triangles since they are small
                    float3 ab_vec = b_vec - a_vec;
                    float3 ac_vec = c_vec - a_vec;
                    float3 ad_vec = d_vec - a_vec;

                    projected_area = 0.5*fabs(length(cross(ab_vec,ac_vec))) + 0.5*fabs(length(cross(ac_vec,ad_vec)));
                }

                // Correction
                Q.w = Q.w * ( forward_projected_area / projected_area);
            }


            float3 k_i = (float3)(-k, 0, 0);
            float3 k_f = k * normalize(OP);


            Q.xyz = k_f - k_i;

            {
                // Lorentz correction assuming rotation around a given axis (corresponding to the rotation of a single motor in most cases)
                // The expression is derived from the formula found in the Nebula open access article
                if (isCorrectionLorentzActive)
                {
                    float3 axis_rot = (float3)(0.0f,0.0f,1.0f); // Omega rotation. Todo: Set as kernel input argument and define in host application
                    Q.w *= wavelength*fabs((dot(cross(normalize(axis_rot), Q.xyz),normalize(k_f))));
                }

                // Polarization correction begs implementation
            }

            out_buf[id_glb.y * image_size.x + id_glb.x] = Q.w;
        }
        else if (task == 1)
        {
            // Calculate variance, requires mean to be known
            out_buf[id_glb.y * image_size.x + id_glb.x] = pow(Q.w - mean, 2.0f);
        }
        else if (task == 2)
        {
            // Calculate skewness, requires deviation and mean to be known
            out_buf[id_glb.y * image_size.x + id_glb.x] = pow((Q.w - mean) / deviation, 3.0f);
        }
        else if (task == 3)
        {
            // Calculate x weightpoint
            out_buf[id_glb.y * image_size.x + id_glb.x] = Q.w * ((float)id_glb.x + 0.5f);
        }
        else if (task == 4)
        {
            // Calculate y weightpoint
            out_buf[id_glb.y * image_size.x + id_glb.x] = Q.w * ((float)id_glb.y + 0.5f);
        }
        else
        {
            // Should not happen
            out_buf[id_glb.y * image_size.x + id_glb.x] = (float) id_glb.y * image_size.x + id_glb.x;
        }

    }
}

kernel void projectScatteringData(
    global float4 * out_buf,
    global float * in_buf,
    constant float * sample_rotation_matrix,
    float pixel_size_x,
    float pixel_size_y,
    float wavelength,
    float detector_distance,
    float beam_x,
    float beam_y,
    float start_angle,
    float angle_increment,
    float kappa,
    float phi,
    float omega,
    int4 selection,
    int2 image_size
//    global float * data_interdistance_buf
)
{
    // Project data from the detector plane onto the Ewald sphere

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
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));

    if ((id_glb.x < image_size.x) && (id_glb.y < image_size.y))
    {
        float4 Q = (float4)(0.0f);

        // If the pixel is within the selection
        if ((id_glb.x >= selection.x) && // Left
                (id_glb.x < selection.y) && // Right
                (id_glb.y >= selection.z) && // Top
                (id_glb.y < selection.w)) // Bottom
        {
            Q.w = in_buf[id_glb.y * image_size.x + id_glb.x];

            /*
             * Projecting the pixel onto the Ewald sphere
             * */

            // The real space vector OP going from the origo (O) to the pixel (P)
            float3 OP = (float3)(
                                            -detector_distance,
                                            pixel_size_x * ((float) (image_size.y - 0.5f - id_glb.y) - beam_x), /* DANGER */
                                            //pixel_size_y * ((float) (image_size.x - 0.5f - id_glb.x) - beam_y)
                                            pixel_size_y * ((float) -((id_glb.x + 0.5) - beam_y))); /* DANGER */

            float k = 1.0f / wavelength; // Multiply with 2pi if desired

            float3 k_i = (float3)(-k, 0, 0);
            float3 k_f = k * normalize(OP);

            Q.xyz = k_f - k_i;

            // Sample rotation
            float3 temp = Q.xyz;

            Q.x = temp.x * sample_rotation_matrix[0] + temp.y * sample_rotation_matrix[1] + temp.z * sample_rotation_matrix[2];
            Q.y = temp.x * sample_rotation_matrix[4] + temp.y * sample_rotation_matrix[5] + temp.z * sample_rotation_matrix[6];
            Q.z = temp.x * sample_rotation_matrix[8] + temp.y * sample_rotation_matrix[9] + temp.z * sample_rotation_matrix[10];

            // Scale/shift data between twice the maximum Q vector, Qmax = 2/wavelength, to lie between 0 and 1.
            // This is done because p_interpolation_octree uses a root side length of 1.0
            Q.xyz = Q.xyz * wavelength * 0.25f + 0.5f;
        }
        else
        {
            Q.w = 0.0f;
        }

        // Write to the Q target (CL texture)
        out_buf[id_glb.y * image_size.x + id_glb.x] = Q;
    }
}

kernel void bufferMax(
    global float * in_buf,
    global float * out_buf,
    int2 image_size)
{
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));

    if ((id_glb.x < image_size.x) && (id_glb.y < image_size.y))
    {
        out_buf[id_glb.y * image_size.x + id_glb.x] = max(out_buf[id_glb.y * image_size.x + id_glb.x], in_buf[id_glb.y * image_size.x + id_glb.x]);
    }
}
