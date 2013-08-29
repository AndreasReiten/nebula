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
    //~ Ry(Rya,-omgp);
    //~ Rz(Rza,-omg);
    //~ Rarb(-kpa,alpha,0,Rkpa);
    //~ Rarb(-phi,beta,0,Rphi);
    //~ float R[9], A[9], B[9];
    //~ 
    //~ if(1)
    //~ {
        //~ dot(Rza, Rya, A,3,3,3);
        //~ dot(Rkpa ,A , B,3,3,3);
        //~ dot(Rphi ,B , R,3,3,3);
    //~ }
    
    
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
    
    if (0)
    {
        dot3x3(omega_rot_z, kappa_rot_y_p, temp_a);
        dot3x3(temp_a, kappa_rot_z, temp_b);
        dot3x3(temp_b, kappa_rot_y_m, temp_a);
        dot3x3(temp_a, phi_rot_y_p, temp_b);
        dot3x3(temp_b, phi_rot_z, temp_a);
        dot3x3(temp_a, phi_rot_y_m, buf);
    }
    else
    {
        dot3x3(phi_rot_y_p, phi_rot_z, temp_b);
        dot3x3(temp_b, phi_rot_y_m, temp_a);
        dot3x3(temp_a, kappa_rot_y_p, temp_b);
        dot3x3(temp_b, kappa_rot_z, temp_a);
        dot3x3(temp_a, kappa_rot_y_m, temp_b);
        dot3x3(temp_b, omega_rot_z, buf);
        
    }
}

__kernel void PILATUS_1_2_WRANGLE(
    __global float * header,
    __global float * intensity,
    __global int * index,
    __global int * offset,
    __global int * length,
    __global float4 * result,
    float4 misc,
    int detector,
    int active_angle)
{
    // Find the local WI size and index
    uint size_loc = get_local_size(0);
    uint id_loc = get_local_id(0);
    
    // Find the index of the file to work on
    uint id_file = get_group_id(0); 
    
    // Find the number of passes per WI
    uint cycles = length[id_file] / size_loc;
    
    // Find the current offset 
    uint off = offset[id_file];
    
    // Access the header. 
    uint HEADER_LENGTH_MAX = 16; 
    float t_exposure = header[id_file*HEADER_LENGTH_MAX];
    float wavelength = header[id_file*HEADER_LENGTH_MAX+1];
    float angle_start = header[id_file*HEADER_LENGTH_MAX+2];
    float angle_incr = header[id_file*HEADER_LENGTH_MAX+3];
    float flux = header[id_file*HEADER_LENGTH_MAX+4];
    float detector_distance = header[id_file*HEADER_LENGTH_MAX+5];
    float beam_x = header[id_file*HEADER_LENGTH_MAX+6]; 
    float beam_y = header[id_file*HEADER_LENGTH_MAX+7]; 
    float phi = header[id_file*HEADER_LENGTH_MAX+8];
    float kappa = header[id_file*HEADER_LENGTH_MAX+9];
    float omega = header[id_file*HEADER_LENGTH_MAX+10]; 
    
    // Set the active angle
    if(active_angle == 0) phi = angle_start + 0.5*angle_incr;
    else if(active_angle == 1) kappa = angle_start + 0.5*angle_incr;
    else if(active_angle == 2) omega = angle_start + 0.5*angle_incr;
    
    // Find the rotation matrix. One lucky WI gets to do this.
    float sample_matrix[9];
    /* ROTATION FUNCTION COULD BE BUGGY! NEED TESTING*/
    get_sample_rot_matrix(sample_matrix, misc.x, misc.y, -phi, -kappa, -omega);
    
    // Process the data points of this WI
    float4 xyzi;
    float4 temp;
    if (detector == 0)
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
        
        uint fast_dimension = 1475;
        uint slow_dimension = 1679;
        float pixel_length_z = 172e-6;
        float pixel_length_y = 172e-6;
        uint fast_index;
        uint slow_index;
        uint tmp;
        float k = 1.0f/wavelength; // Multiply with 2pi if desired
        
        for(int i = 0; i < cycles; i++)
        {
            xyzi.w = intensity[off + i*size_loc + id_loc];
            tmp = index[off + i*size_loc + id_loc];
            
            fast_index = fast_dimension - (tmp % fast_dimension) - 1;
            slow_index = slow_dimension - (tmp / fast_dimension) - 1;
            
            xyzi.x = 0;
            xyzi.y = pixel_length_y * ((float) slow_index);
            xyzi.z = pixel_length_z * ((float) fast_index);
            
            // Center the detector
            xyzi.y -= beam_x * pixel_length_y; //Not sure if I should offset by half a pixel more/less
            xyzi.z -= beam_y * pixel_length_z;
            
            // Titlt the detector around origo assuming it correctly 
            // coincides with the actual center of rotation ( not yet implemented)
            
            
            // Project onto Ewald's sphere, moving to k-space
            xyzi.x = -detector_distance;
            xyzi.xyz = k*normalize(xyzi.xyz); 
            
            {
                // xyz now has the direction of the scattered ray with respect to the incident one. This can be used to calculate the scattering angle for correction purposes. 
                float r = fast_length(xyzi.xyz);
                float x = xyzi.x;
                float y = xyzi.y;
                float z = xyzi.z;
                
                // lab_theta and lab_phi are not to be confused with the detector/sample angles. These are simply the circular coordinate representation of the pixel position
                float lab_theta = asin(native_divide(y,r));
                float lab_phi = atan(native_divide(z,-x));
                
                /* Lorentz Polarization correction - The Lorentz part will depend on the scanning axis, and has to be applied if the frames are integrated over some time */
                
                // Assuming rotation around the z-axis of the lab frame:
                float L = native_sin(lab_theta);
                
                // The polarization correction also needs a bit more work
                //float P = 
                
                xyzi.w *= L;
            }
            
            xyzi.x += k; // This step must be applied _after_ any detector rotation if such is used. This translation by k gives us the scattering vector Q, which is the reciprocal coordinate of the intensity.
            
            
            
            
            
            
            // Sample rotation
            float4 temp = xyzi;
            
            xyzi.x = temp.x * sample_matrix[0] + temp.y * sample_matrix[1] + temp.z * sample_matrix[2];
            xyzi.y = temp.x * sample_matrix[3] + temp.y * sample_matrix[4] + temp.z * sample_matrix[5];
            xyzi.z = temp.x * sample_matrix[6] + temp.y * sample_matrix[7] + temp.z * sample_matrix[8];
            
            //~ xyzi.x = off + i*size_loc + id_loc;
            //~ xyzi.y = id_loc;
            
            // Store result
            result[off + i*size_loc + id_loc] = xyzi;
        }
    }
}

// Send per brick:  number of used indices
__kernel void K_GEN_BRICKS(
    __global float4 * points,
    __global int * subbox_offsets,
    __global int * subbox_lengths,
    __global int * brick_ids,
    __global int * brick_ids_offsets,
    __global int * brick_ids_lengths,
    __global float * extents,
    __global float * output,
    int brick_dim,
    float srchrad
    )
{
    // Bitwise operations used here:
    // X / 2^n = X >> n
    // X % 2^n = X & (2^n - 1)
    // 2^n = 1 << n
    
    // Each WG is one brick. Each WI is one interpolation point in the brick.
    int id_glb_x = get_local_id(0);
    int id_glb_y = get_local_id(1);
    int id_glb_z = get_local_id(2);
    int id_brick = get_global_id(2) >> 3; // NB: ASSUMES brick_dim = 8 !
    int id_output = id_glb_x + id_glb_y*brick_dim + get_global_id(2)*brick_dim*brick_dim;

    // Position of point 
    float4 xyzw;
    xyzw.x = native_divide((float)id_glb_x, (float)brick_dim-1.0)*(extents[id_brick*6+1]-extents[id_brick*6+0]) + extents[id_brick*6+0];
    xyzw.y = native_divide((float)id_glb_y, (float)brick_dim-1.0)*(extents[id_brick*6+3]-extents[id_brick*6+2]) + extents[id_brick*6+2];
    xyzw.z = native_divide((float)id_glb_z, (float)brick_dim-1.0)*(extents[id_brick*6+5]-extents[id_brick*6+4]) + extents[id_brick*6+4];
    xyzw.w = 0.0f;
    
    // Interpolate around positions using invrese distance weighting
    float4 point;
    float sum_intensity = 0;
    float sum_distance = 0;
    float distance;
    int id_sub;
    // For each sub box intersecting with the brick
    for (int i = 0; i < brick_ids_lengths[id_brick]; i++)
    {
        id_sub = brick_ids[brick_ids_offsets[id_brick]+i];
        
        // For each data point
        for (int j = 0; j < subbox_lengths[id_sub]; j++) // There is a read error here
        {
            //~ xyzw.w += 0.01;
            // IDW
            point = points[subbox_offsets[id_sub]+j]; //ERROR
            distance = fast_distance(xyzw.xyz, point.xyz);
        
            if (distance <= srchrad)
            {
                sum_intensity += native_divide(point.w, distance);
                sum_distance += native_divide(1.0f, distance);
            }
        } 
    }
    if (sum_distance > 0) xyzw.w = native_divide(sum_intensity, sum_distance);
    
    // Pass result to output array
    output[id_output] = xyzw.w;
}
