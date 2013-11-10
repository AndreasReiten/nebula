uint target_index(int4 target_dimension, int4 id_loc ,uint brick_outer_dimension, uint brick_count)
{
    int4 target_brick_dimension = target_dimension/(int4)(brick_outer_dimension);

    int4 brick_offset;
    brick_offset.z = brick_count / (target_brick_dimension.x * target_brick_dimension.y);
    brick_offset.y = (brick_count % (target_brick_dimension.x * target_brick_dimension.y)) / target_brick_dimension.x;
    brick_offset.x = (brick_count % (target_brick_dimension.x * target_brick_dimension.y)) % target_brick_dimension.x;

    int4 index3d = brick_offset*(int4)(brick_outer_dimension) + id_loc;

    return index3d.x + index3d.y * target_dimension.x + index3d.z * target_dimension.x * target_dimension.y;
}

__kernel void voxelize(
    __global float4 * point_data,
    __constant int * point_data_offset,
    __constant int * point_data_count,
    __global float * brick_extent,
    __global float * cluster_pool,
    __global int * empty_check,
    __local float * addition_array,
    uint brick_outer_dimension,
    float search_radius
    )
{
    // Each Work Group is one brick. Each Work Item is one interpolation point in the brick.
    int4 id_loc = (int4)(get_local_id(0), get_local_id(1), get_local_id(2), 0);

    int id_output = id_loc.x + id_loc.y*brick_outer_dimension + id_loc.z*brick_outer_dimension*brick_outer_dimension;
    int id_wg = get_group_id(2);

    // Position of point
    float4 xyzw;
    float step_length = (extent[id_wg*6 + 1] - extent[id_wg*6 + 0]) / ((float)brick_outer_dimension - 1.0f);
    xyzw.x = extent[id_wg*6 + 0] + (float)id_loc.x * step_length;
    xyzw.y = extent[id_wg*6 + 2] + (float)id_loc.y * step_length;
    xyzw.z = extent[id_wg*6 + 4] + (float)id_loc.z * step_length;
    xyzw.w = 0.0f;

    // Interpolate around positions using invrese distance weighting
    float4 point;
    float sum_intensity = 0.0f;
    float sum_distance = 0.0f;
    float dst;

    for (int i = 0; i < point_data_count[id_wg]; i++)
    {
        point = point_data[point_data_offset[id_wg]i];
        dst = fast_distance(xyzw.xyz, point.xyz);
        if (dst <= 0.0f)
        {
            sum_intensity = point.w;
            sum_distance = 1.0f;
            break;
        }
        if (dst <= search_radius)
        {
            sum_intensity += native_divide(point.w, dst);
            sum_distance += native_divide(1.0f, dst);
        }
    }
    if (sum_distance > 0) xyzw.w = sum_intensity / sum_distance;

    // Pass result to output array
    pool_cluster[id_output] = xyzw.w;

    // Parallel reduction to find if there is nonzero data
    if (xyzw.w != 0.0f) addition_array[id_output] = 1.0f;
    else  addition_array[id_wg*brick_outer_dimension*brick_outer_dimension*brick_outer_dimension + id_output] = 0.0f;

    barrier(CLK_LOCAL_MEM_FENCE);
    for (unsigned int i = 256; i > 0; i >>= 1)
    {
        if (id_output < i)
        {
            addition_array[id_wg*brick_outer_dimension*brick_outer_dimension*brick_outer_dimension + id_output] += addition_array[i + id_output];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (id_output == 0)
    {
        empty_check[id_wg] = !addition_array[0];
    }
}

__kernel void voxelize_old(
    __global float4 * items,
    __constant float * extent,
    __global float * isempty,
    uint brick_outer_dimension,
    uint item_count,
    float search_radius,
    __local float * addition_array,
    int4 target_dimension,
    uint brick_count,
    __global float * pool
    )
{
    // Each Work Group is one brick. Each Work Item is one interpolation point in the brick.
    int4 id_loc = (int4)(get_local_id(0), get_local_id(1), get_local_id(2), 0);

    int id_output = id_loc.x + id_loc.y*brick_outer_dimension + id_loc.z*brick_outer_dimension*brick_outer_dimension;


    // Position of point
    float4 xyzw;
    float step_length = (extent[1] - extent[0]) / ((float)brick_outer_dimension - 1.0f);
    xyzw.x = extent[0] + (float)id_loc.x * step_length;
    xyzw.y = extent[2] + (float)id_loc.y * step_length;
    xyzw.z = extent[4] + (float)id_loc.z * step_length;
    xyzw.w = 0.0f;

    // Interpolate around positions using invrese distance weighting
    float4 point;
    float sum_intensity = 0.0f;
    float sum_distance = 0.0f;
    float dst;

    for (int i = 0; i < item_count; i++)
    {
        point = items[i];
        dst = fast_distance(xyzw.xyz, point.xyz);
        if (dst <= 0.0f)
        {
            sum_intensity = point.w;
            sum_distance = 1.0f;
            break;
        }
        if (dst <= search_radius)
        {
            sum_intensity += native_divide(point.w, dst);
            sum_distance += native_divide(1.0f, dst);
        }
    }
    if (sum_distance > 0) xyzw.w = sum_intensity / sum_distance;

    // Pass result to output array
    pool[target_index(target_dimension, id_loc, brick_outer_dimension, brick_count)] = xyzw.w;

    // Parallel reduction to find if there is nonzero data
    if (xyzw.w != 0.0f) addition_array[id_output] = 1.0f;
    else  addition_array[id_output] = 0.0f;

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
        isempty[0] = !addition_array[0];
    }
}
