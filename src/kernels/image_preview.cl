#pragma OPENCL EXTENSION cl_khr_fp64 : enable

kernel void imageDisplay(
    global float * data_buf,
    write_only image2d_t frame_image,
    read_only image2d_t tsf_image,
    sampler_t tsf_sampler,
    float2 data_limit,
    int log
)
{
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));
    int2 image_dim = get_image_dim(frame_image);

    if ((id_glb.x < image_dim.x) && (id_glb.y < image_dim.y))
    {
        float intensity = data_buf[id_glb.y * image_dim.x + id_glb.x];

        float2 tsf_position;
        float4 sample;

        if (log)
        {
            if (data_limit.x <= 0.00001f)
            {
                data_limit.x = 0.00001f;
            }

            if (intensity <= 0.00001f)
            {
                tsf_position = (float2)(0.0f, 0.5f);
                sample = read_imagef(tsf_image, tsf_sampler, tsf_position);// + (float4)(0.0,0.0,1.0,0.2);
            }
            else
            {
                tsf_position = (float2)(native_divide(log10(intensity) - log10(data_limit.x), log10(data_limit.y) - log10(data_limit.x)), 0.5f);
                sample = read_imagef(tsf_image, tsf_sampler, tsf_position);
            }
        }
        else
        {
            tsf_position = (float2)(native_divide(intensity - data_limit.x, data_limit.y - data_limit.x), 0.5f);
            sample = read_imagef(tsf_image, tsf_sampler, tsf_position);
        }

        write_imagef(frame_image, id_glb, sample);
    }
}

kernel void bufferMax(
    global float * data_buf,
    global float * out_buf,
    int2 image_size)
{
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));

    if ((id_glb.x < image_size.x) && (id_glb.y < image_size.y))
    {
        out_buf[id_glb.y * image_size.x + id_glb.x] = max(out_buf[id_glb.y * image_size.x + id_glb.x], data_buf[id_glb.y * image_size.x + id_glb.x]);
    }
}

kernel void imageCalculus(
    global float * data_buf,
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
        Q.w = data_buf[id_glb.y * image_size.x + id_glb.x];

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
                                            pix_size_x * ((float) (image_size.y - 0.5f - id_glb.y) - beam_x), /* DANGER */
                                            pix_size_y * ((float) (image_size.x - 0.5f - id_glb.x) - beam_y)); /* DANGER */

            double k = 1.0 / wavelength; // Multiply with 2pi if desired

            /* Corrections */
            // Correct for the area of the projection of the pixel onto the Ewald sphere
            if (isCorrectionPixelProjectionActive)
            {
                double forward_projected_area;
                {
                    // The four vectors that define projected pixel on the Ewald sphere
                    double3 a_vec = k * normalize((double3)(-det_dist, (double) pix_size_x * 0.5, -(double) pix_size_y * 0.5));
                    double3 b_vec = k * normalize((double3)(-det_dist, -(double) pix_size_x * 0.5, -(double) pix_size_y * 0.5));
                    double3 c_vec = k * normalize((double3)(-det_dist, -(double) pix_size_x * 0.5, (double) pix_size_y * 0.5));
                    double3 d_vec = k * normalize((double3)(-det_dist, (double) pix_size_x * 0.5, (double) pix_size_y * 0.5));

                    // The area of the two spherical triangles spanned by the projected pixel is approximated by their corresponding planar triangles since they are small
                    double3 ab_vec = b_vec - a_vec;
                    double3 ac_vec = c_vec - a_vec;
                    double3 ad_vec = d_vec - a_vec;

                    forward_projected_area = 0.5*fabs(length(cross(ab_vec,ac_vec))) + 0.5*fabs(length(cross(ac_vec,ad_vec)));
                }

                double projected_area;
                {
                    // The four vectors that define projected pixel on the Ewald sphere
                    double3 a_vec = k * normalize( convert_double3(OP) + (double3)(0, (double) pix_size_x * 0.5, -(double) pix_size_y * 0.5));
                    double3 b_vec = k * normalize( convert_double3(OP) + (double3)(0, -(double) pix_size_x * 0.5, -(double) pix_size_y * 0.5));
                    double3 c_vec = k * normalize( convert_double3(OP) + (double3)(0, -(double) pix_size_x * 0.5, (double) pix_size_y * 0.5));
                    double3 d_vec = k * normalize( convert_double3(OP) + (double3)(0, (double) pix_size_x * 0.5, (double) pix_size_y * 0.5));

                    // The area of the two spherical triangles spanned by the projected pixel is approximated by their corresponding planar triangles since they are small
                    double3 ab_vec = b_vec - a_vec;
                    double3 ac_vec = c_vec - a_vec;
                    double3 ad_vec = d_vec - a_vec;

                    projected_area = 0.5*fabs(length(cross(ab_vec,ac_vec))) + 0.5*fabs(length(cross(ac_vec,ad_vec)));
                }

                // Correction
                Q.w = Q.w * ( forward_projected_area / projected_area);
            }


            float3 k_i = (float3)(-k, 0, 0);
            float3 k_f = (float)k * normalize(OP);


            Q.xyz = k_f - k_i;

            {
                float lab_theta = asin(native_divide(fabs(Q.y), k)); // Not to be confused with 2-theta, the scattering angle

                // Lorentz correction: Assuming rotation around the z-axis of the lab frame:
                if (isCorrectionLorentzActive)
                {
                    Q.w *= sin(lab_theta);
                }

                // Polarization correction begs implementation
            }

            // Post correction filter
            //Q.w = clamp(Q.w, pct_low, pct_high); // Note: remove this filter at some point. It is bad.
            //Q.w -= pct_low;

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
        }

    }
}
