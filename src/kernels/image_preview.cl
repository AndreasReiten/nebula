__kernel void imageDisplay(
    __global float * data_buf,
    __write_only image2d_t frame_image,
    __read_only image2d_t tsf_image,
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
            if (data_limit.x <= 0.00001)
            {
                data_limit.x = 0.00001;
            }

            if (intensity <= 0.00001)
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

__kernel void bufferMax(
    __global float * data_buf,
    __global float * out_buf,
    int2 image_size)
{
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));

    if ((id_glb.x < image_size.x) && (id_glb.y < image_size.y))
    {
        out_buf[id_glb.y * image_size.x + id_glb.x] = max(out_buf[id_glb.y * image_size.x + id_glb.x], data_buf[id_glb.y * image_size.x + id_glb.x]);
    }
}

__kernel void imageCalculus(
    __global float * data_buf,
    __global float * out_buf,
    __constant float * parameter,
    int2 image_size,
    int correction_lorentz,
    int task,
    float mean,
    float deviation,
    int isCorrectionNoiseActive,
    int isCorrectionPlaneActive,
    int isCorrectionPolarizationActive,
    int isCorrectionFluxActive,
    int isCorrectionExposureActive,
    float4 plane
    //    __global float * xyzi_buf
    //    __read_only image3d_t background,
    //    sampler_t bg_sampler,
    //    int sample_interdist,
    //    int image_number,
    //    int correction_background
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
        float value = data_buf[id_glb.y * image_size.x + id_glb.x];



        //        if (task == -1)
        //        {
        //            float4 pos = (float4)(((float)id_glb.x+0.5f)/(float)sample_interdist, ((float)id_glb.y+0.5f)/(float)sample_interdist, (float)image_number+0.5, 0.0f);
        //            float bg = read_imagef(background, bg_sampler, pos).w;

        //            out_buf[id_glb.y * image_size.x + id_glb.x] = bg;
        //        }
        if (task == 0)
        {
            // Background subtraction based on estimate
            //            if (correction_background)
            //            {
            //                float4 pos = (float4)(((float)id_glb.x+0.5f)/(float)sample_interdist, ((float)id_glb.y+0.5f)/(float)sample_interdist, (float)image_number+0.5, 0.0f);
            //                float bg = read_imagef(background, bg_sampler, pos).w;

            //                value = clamp(value, bg, noise_high); // Set to at least the value of the background
            //                value -= bg; // Subtract
            //            }

            // Flat background subtraction
            if (isCorrectionNoiseActive)
            {
                value = clamp(value, noise_low, noise_high); // All readings within thresholds
                value -= noise_low; // Subtract
            }

            // Planar background subtraction
            if (isCorrectionPlaneActive)
            {
                float plane_z = -(plane.x * (float)id_glb.x + plane.y * (float)id_glb.y + plane.w) / plane.z;

                if (plane_z < 0)
                {
                    plane_z = 0;    // Negative values do not make sense
                }

                value = clamp(value, plane_z, noise_high); // All readings within thresholds
                value -= plane_z;
            }

            // Corrections
            float4 Q = (float4)(0.0f);
            float k = 1.0f / wavelength; // Multiply with 2pi if desired

            float3 k_i = (float3)(-k, 0.0f, 0.0f);
            float3 k_f = k * normalize((float3)(
                                           -det_dist,
                                           pix_size_x * ((float) (image_size.y - 0.5 - id_glb.y) - beam_x), /* DANGER */
                                           pix_size_y * ((float) (image_size.x - 0.5 - id_glb.x) - beam_y))); /* DANGER */

            Q.xyz = k_f - k_i;
            {
                float lab_theta = asin(native_divide(fabs(Q.y), k)); // Not to be confused with 2-theta, the scattering angle

                // Lorentz correction: Assuming rotation around the z-axis of the lab frame:
                if (correction_lorentz)
                {
                    value *= sin(lab_theta);
                }

                // Polarization correction begs implementation
            }

            // Post correction filter
            value = clamp(value, pct_low, pct_high); // Note: remove this filter at some point. It is bad.
            value -= pct_low;

            out_buf[id_glb.y * image_size.x + id_glb.x] = value;

            //            xyzi_buf[(id_glb.y * image_size.x + id_glb.x)*4+0] = Q.x;
            //            xyzi_buf[(id_glb.y * image_size.x + id_glb.x)*4+1] = Q.y;
            //            xyzi_buf[(id_glb.y * image_size.x + id_glb.x)*4+2] = Q.z;
            //            xyzi_buf[(id_glb.y * image_size.x + id_glb.x)*4+3] = value;
        }
        else if (task == 1)
        {
            // Calculate variance, requires mean to be known
            out_buf[id_glb.y * image_size.x + id_glb.x] = pow(value - mean, 2.0);
        }
        else if (task == 2)
        {
            // Calculate skewness, requires deviation and mean to be known
            //            float tmp = pow((value - mean) / deviation, 3.0);
            //            if (tmp <= 0.0001) out_buf[id_glb.y * image_size.x + id_glb.x] = 0;
            //            else out_buf[id_glb.y * image_size.x + id_glb.x] = 1;



            out_buf[id_glb.y * image_size.x + id_glb.x] = pow((value - mean) / deviation, 3.0);
        }
        else if (task == 3)
        {
            // Calculate x weightpoint
            out_buf[id_glb.y * image_size.x + id_glb.x] = value * ((float)id_glb.x + 0.5);
        }
        else if (task == 4)
        {
            // Calculate y weightpoint
            out_buf[id_glb.y * image_size.x + id_glb.x] = value * ((float)id_glb.y + 0.5);
        }
        else
        {
            // Should not happen
        }

    }
}
