kernel void FRAME_FILTER(
    write_only image2d_t xyzi_target, // TODO: Check if this actually needs to be a pic
    global float * source,
    sampler_t tsf_sampler,
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
    int4 selection
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
    int2 target_dim = get_image_dim(xyzi_target);

    if ((id_glb.x < target_dim.x) && (id_glb.y < target_dim.y))
    {
        float4 Q = (float4)(0.0f);

        // If the pixel is within the selection
        if ((id_glb.x >= selection.x) && // Left
                (id_glb.x < selection.y) && // Right
                (id_glb.y >= selection.z) && // Top
                (id_glb.y < selection.w)) // Bottom
        {
            Q.w = source[id_glb.y * target_dim.x + id_glb.x];

            if (Q.w > 0.0f)
            {
                /*
                 * Projecting the pixel onto the Ewald sphere
                 * */

                // The real space vector OP going from the origo (O) to the pixel (P)
                float3 OP = (float3)(
                                                -detector_distance,
                                                pixel_size_x * ((float) (target_dim.y - 0.5f - id_glb.y) - beam_x), /* DANGER */
                                                //pixel_size_y * ((float) (target_dim.x - 0.5f - id_glb.x) - beam_y)
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
            }
        }
        else
        {
            Q.w = 0.0f;
        }

        // Write to the Q target (CL texture)
        write_imagef(xyzi_target, id_glb, Q);
    }
}
