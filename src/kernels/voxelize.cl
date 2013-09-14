__kernel void voxelize(
    __constant float4 * items,
    __constant float * extent,
    __global float * target,
    int brick_outer_dimension,
    int item_count,
    float search_radius,
    __local float * addition_array
    )
{
    // Each Work Group is one brick. Each Work Item is one interpolation point in the brick.
    int id_glb_x = get_local_id(0);
    int id_glb_y = get_local_id(1);
    int id_glb_z = get_local_id(2);
    int id_output = id_glb_x + id_glb_y*brick_outer_dimension + id_glb_z*brick_outer_dimension*brick_outer_dimension;

    // Position of point
    float4 xyzw;

    xyzw.x = native_divide((float)id_glb_x, (float)brick_outer_dimension-1.0)*(extent[1]-extent[0]) + extent[0];
    xyzw.y = native_divide((float)id_glb_y, (float)brick_outer_dimension-1.0)*(extent[3]-extent[2]) + extent[2];
    xyzw.z = native_divide((float)id_glb_z, (float)brick_outer_dimension-1.0)*(extent[5]-extent[4]) + extent[4];
    xyzw.w = 0.0f;

    // Interpolate around positions using invrese distance weighting
    float4 point;
    float sum_intensity = 0;
    float sum_distance = 0;
    float dst;

    for (int i = 0; i < item_count; i++)
    {
        point = items[i];
        dst = fast_distance(xyzw.xyz, point.xyz);
        if (dst <= 0.0)
        {
            sum_intensity = point.w;
            sum_distance = 1.0;
            break;
        }
        else if (dst <= search_radius)
        {
            sum_intensity += native_divide(point.w, dst);
            sum_distance += native_divide(1.0f, dst);
        }
    }
    if (sum_distance > 0) xyzw.w = native_divide(sum_intensity, sum_distance);

    // Pass result to output array
    target[id_output] = xyzw.w;



    // Parallel reduction to find sum of items
    addition_array[id_output] = xyzw.w;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (unsigned int i = 256; i > 0; i >>= 1)
    {
        if (id_output < i)
        {
            addition_array[id_output] += addition_array[i + id_output];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (id_output == 0)
    {
        target[512] = addition_array[0];
    }
}
