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

    //~size_t n_points_brick = brick_dim*brick_dim*brick_dim;
    //~size_t n_brick_pool = 1 << pool_power;
    //~size_t n_points_pool = brick_dim*n_brick_pool;
    //~size_t n_points_offset = id*n_points_brick;
    //~size_t brick_xyz[3];
    //~size_t remainder;
    //~size_t id_x, id_y, id_z, i = 0;
//~
    //~remainder = id % (n_brick_pool*n_brick_pool);
    //~brick_xyz[2] = id / (n_brick_pool*n_brick_pool);
    //~brick_xyz[1] = remainder / n_brick_pool;
    //~brick_xyz[0] = remainder % n_brick_pool;
//~
    //~for (id_z = brick_xyz[2]*brick_dim; id_z < (brick_xyz[2]+1)*brick_dim; id_z++)
    //~{
        //~for (id_y = brick_xyz[1]*brick_dim; id_y < (brick_xyz[1]+1)*brick_dim; id_y++)
        //~{
            //~for (id_x = brick_xyz[0]*brick_dim; id_x < (brick_xyz[0]+1)*brick_dim; id_x++)
            //~{
                //~buf_out[id_x + id_y*n_points_pool + id_z*n_points_pool*n_points_pool] = buf_in[i + n_points_offset];
                //~i++;
            //~}
        //~}
    //~}




__kernel void voxelize(
    __global float4 * items,
    __constant float * extent,
    __global float * target,
    uint brick_outer_dimension,
    uint item_count,
    float search_radius,
    __local float * addition_array,
    int4 target_dimension,
    uint brick_count,
    __global float * target2
    )
{
    // Each Work Group is one brick. Each Work Item is one interpolation point in the brick.
    int4 id_loc = (int4)(get_local_id(0), get_local_id(1), get_local_id(2), 0);

    int id_output = id_loc.x + id_loc.y*brick_outer_dimension + id_loc.z*brick_outer_dimension*brick_outer_dimension;


    // Position of point
    float4 xyzw;
    float step_length = (extent[1] - extent[0]) / ((float)brick_outer_dimension - 1.0);
    xyzw.x = extent[0] + (float)id_loc.x * step_length;
    xyzw.y = extent[2] + (float)id_loc.y * step_length;
    xyzw.z = extent[4] + (float)id_loc.z * step_length;
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
        if (dst <= search_radius)
        {
            sum_intensity += native_divide(point.w, dst);
            sum_distance += native_divide(1.0f, dst);
        }
    }
    if (sum_distance > 0) xyzw.w = sum_intensity / sum_distance;

    // Pass result to output array
    target[id_output] = xyzw.w;
    target2[target_index(target_dimension, id_loc, brick_outer_dimension, brick_count)] = xyzw.w;

    // Parallel reduction to find if there is nonzero data
    if (xyzw.w != 0.0) addition_array[id_output] = 1.0;
    else  addition_array[id_output] = 0.0;

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
