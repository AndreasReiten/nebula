__kernel void voxelize(
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
