void dot3x3(float * M, float * N, float * OUT)
{
    OUT[0] = M[0]*N[0] + M[1]*N[3] + M[2]*N[6];
    OUT[1] = M[0]*N[1] + M[1]*N[4] + M[2]*N[7];
    OUT[2] = M[0]*N[2] + M[1]*N[5] + M[2]*N[8];
    
    OUT[3] = M[3]*N[0] + M[4]*N[3] + M[5]*N[6];
    OUT[4] = M[3]*N[1] + M[4]*N[4] + M[5]*N[7];
    OUT[5] = M[3]*N[2] + M[4]*N[5] + M[5]*N[8];
    
    OUT[6] = M[6]*N[0] + M[7]*N[3] + M[8]*N[6];
    OUT[7] = M[6]*N[1] + M[7]*N[4] + M[8]*N[7];
    OUT[8] = M[6]*N[2] + M[7]*N[5] + M[8]*N[8];
}

void rot_y(float * M, float arg)
{
    M[0] = cos(arg);
    M[1] = 0;
    M[2] = -sin(arg);
    
    M[3] = 0;
    M[4] = 1;
    M[5] = 0;
    
    M[6] = sin(arg);
    M[7] = 0;
    M[8] = cos(arg);
}

void rot_z(float * M, float arg)
{
    M[0] = cos(arg);
    M[1] = sin(arg);
    M[2] = 0;
    
    M[3] = -sin(arg);
    M[4] = cos(arg);
    M[5] = 0;
    
    M[6] = 0;
    M[7] = 0;
    M[8] = 1;
}

void get_sample_rot_matrix(float * buf, float alpha, float beta, float phi, float kappa, float omega)
{
    // Omega
    float omega_rot_z[9];
    rot_z(omega_rot_z, omega);

    // Kappa
    float kappa_rot_y_p[9];
    float kappa_rot_z[9];
    float kappa_rot_y_m[9];
    rot_y(kappa_rot_y_p, alpha);
    rot_z(kappa_rot_z, kappa);    
    rot_y(kappa_rot_y_m, -alpha);
    
    // Phi    
    float phi_rot_y_p[9];
    float phi_rot_z[9];
    float phi_rot_y_m[9];
    rot_y(phi_rot_y_p, beta);
    rot_z(phi_rot_z, phi);    
    rot_y(phi_rot_y_m, -beta);
    
    // Multiply
    float temp_a[9];
    float temp_b[9];
    
    dot3x3(phi_rot_y_p, phi_rot_z, temp_b);
    dot3x3(temp_b, phi_rot_y_m, temp_a);
    dot3x3(temp_a, kappa_rot_y_p, temp_b);
    dot3x3(temp_b, kappa_rot_z, temp_a);
    dot3x3(temp_a, kappa_rot_y_m, temp_b);
    dot3x3(temp_b, omega_rot_z, buf);
}

__kernel void FRAME_FILTER(
    __write_only image2d_t i_target,
    __write_only image2d_t xyzi_target,
    __read_only image2d_t background,
    __read_only image2d_t source,
    sampler_t intensity_sampler,
    __constant float * sample_rotation_matrix,
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
    int2 target_dim = get_image_dim(i_target);
    
    /*
     * Background subtraction and lorentz polarization correction. Scaling to a common factor. Finally a flat min/max filter
     * */
    if ((id_glb.x < target_dim.x) && (id_glb.y < target_dim.y))
    {
        
        float intensity = read_imagef(source, intensity_sampler, id_glb).w;
        float4 xyzi = (float4)(0.0);
        // Scale source to background
        intensity *= native_divide(background_flux * background_exposure_time, h_flux * h_exposure_time);

        // Subtract background
        intensity -= read_imagef(background, intensity_sampler, id_glb).w;

        // Scale to a common flux and exposure time
        //~ intensity *= native_divide(common_flux * common_exposure_time, background_flux * background_exposure_time);
        
        // Flat min/max filter
        if (((intensity < threshold.x) || (intensity > threshold.y)))
        {
            intensity = 0.0f;
            float4 iiii = (float4)(intensity);
            write_imagef(i_target, id_glb, iiii);
        } 
        else
        {
            // Write the (almost) corrected result to an image for display in the viewer
            float4 iiii = (float4)(intensity);
            write_imagef(i_target, id_glb, iiii);

            /*
             * Lorentz Polarization correction and distance correction + projecting the pixel onto the Ewald sphere
             * */
            xyzi = (float4)(
                -h_detector_distance,
                h_pixel_size_x * (float) id_glb.y,
                h_pixel_size_y * (float) id_glb.x,
                0.0);
                
            float k = 1.0f/h_wavelength; // Multiply with 2pi if desired
            
            // Center the detector
            xyzi.y -= h_beam_x * h_pixel_size_x; // Not sure if one should offset by half a pixel more/less
            xyzi.z -= h_beam_y * h_pixel_size_y;
            
            // Titlt the detector around origo assuming it correctly coincides with the actual center of rotation ( not yet implemented)
            

            // Distance scale the intensity. The common scale here is the wavelength, so it should be constant across images
            //~ intensity *= powr(fast_length(xyzi.xyz),2.0);
            
            // Project onto Ewald's sphere, moving to k-space
            xyzi.xyz = k*normalize(xyzi.xyz);
            
            {
                // XYZ now has the direction of the scattered ray with respect to the incident one. This can be used to calculate the scattering angle for correction purposes. lab_theta and lab_phi are not to be confused with the detector/sample angles. These are simply the circular coordinate representation of the pixel position
                float lab_theta = asin(native_divide(xyzi.y, k));
                float lab_phi = atan2(xyzi.z,-xyzi.x);
                
                /* Lorentz Polarization correction - The Lorentz part will depend on the scanning axis, but has to be applied if the frames are integrated over some time */
                
                // Assuming rotation around the z-axis of the lab frame:
                float L = native_sin(lab_theta);
                
                // The polarization correction also needs a bit more work...
                intensity *= L;
            }
            
            // This translation by k gives us the scattering vector Q, which is the reciprocal coordinate of the intensity. This step must be applied _after_ any detector rotation if such is used. 
            xyzi.x += k;
            
            /* Sample rotation */
            float4 temp = xyzi;
            
            xyzi.x = temp.x * sample_rotation_matrix[0] + temp.y * sample_rotation_matrix[1] + temp.z * sample_rotation_matrix[2];
            xyzi.y = temp.x * sample_rotation_matrix[4] + temp.y * sample_rotation_matrix[5] + temp.z * sample_rotation_matrix[6];
            xyzi.z = temp.x * sample_rotation_matrix[8] + temp.y * sample_rotation_matrix[9] + temp.z * sample_rotation_matrix[10];
        }
        xyzi.w = intensity;
        write_imagef(xyzi_target, id_glb, xyzi);
    }
}
