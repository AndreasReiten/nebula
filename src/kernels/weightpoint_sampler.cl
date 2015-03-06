__kernel void weightpointSampler(
    __read_only image3d_t pool, sampler_t pool_sampler,
    __global uint * oct_index, __global uint * oct_brick,
    __constant float * data_extent,
    __constant float * data_view_extent,
    __constant int * misc_int,
    __global float * result,
    __local float * addition_array)
{
    int3 id_loc = (int3)(get_local_id(0), get_local_id(1), get_local_id(2));
    int3 id_glb = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));
    int3 id_grp = (int3)(get_group_id(0), get_group_id(1), get_group_id(2));
    int3 size_grp = (int3)(get_num_groups(0), get_num_groups(1), get_num_groups(2));
    int3 size_loc = (int3)(get_local_size(0), get_local_size(1), get_local_size(2));
    int3 size_glb = (int3)(get_global_size(0), get_global_size(1), get_global_size(2));
    
    int id_loc_linear = id_loc.x + id_loc.y*size_loc.x + id_loc.z*size_loc.x*size_loc.y;
    int id_result = id_grp.x + id_grp.y*size_grp.x + id_grp.z*size_grp.x*size_grp.y;

    if ((id_loc.x == 0) && (id_loc.y == 0) && (id_loc.z == 0))
    {
        result[id_result] = 0;
    }
    
    int4 pool_dim = get_image_dim(pool);
    
    int n_tree_levels = misc_int[0];
    int brick_dim = misc_int[1];
        
    // Some variables we will need during ray traversal
    uint index, brick, isMsd, isEmpty;
    float3 norm_pos;
    float4 lookup_pos;
    uint4 brick_id;
    int3 norm_index;

    // Merged bits in the octree can be read using these bitmasks:
    uint mask_msd_flag = ((1u << 1u) - 1u) << 31u;
    uint mask_data_flag = ((1 << 1) - 1) << 30;
    uint mask_child_index = ((1 << 30) - 1) << 0;
    uint mask_brick_id_x = ((1 << 10) - 1) << 20;
    uint mask_brick_id_y = ((1 << 10) - 1) << 10;
    uint mask_brick_id_z = ((1 << 10) - 1) << 0;
    
    // Sample the octree 
    // xyz position 
    float3 pos = (float3)(
                        data_view_extent[0] + (0.5+(float)id_glb.x)*((data_view_extent[1] - data_view_extent[0])/(size_glb.x-1)),
                        data_view_extent[2] + (0.5+(float)id_glb.y)*((data_view_extent[3] - data_view_extent[2])/(size_glb.y-1)),
                        data_view_extent[4] + (0.5+(float)id_glb.z)*((data_view_extent[5] - data_view_extent[4])/(size_glb.z-1)))

    // Descend into the octree data struget_global_id(0)cture
    // Index trackers for the traversal.
    index = 0;

     // We use a normalized convention during octree traversal. The normalized convention makes it easier to think about the octree traversal.
    norm_pos = native_divide( (float3)(pos.x - data_extent[0], pos.y - data_extent[2], pos.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;

    norm_index = convert_int3(norm_pos);
    norm_index = clamp(norm_index, 0, 1);

    // Traverse the octree
    for (int j = 0; j < n_tree_levels; j++)
    {
        brick = oct_index[index];
        isMsd = (brick & mask_msd_flag) >> 31;
        isEmpty = !((brick & mask_data_flag) >> 30);
        
        float voxel_size = (data_extent[1] - data_extent[0])/((float)((brick_dim-1) * (1 << j)));
        
        bool isLowEnough = (voxel_size > (data_view_extent[1] - data_view_extent[0])/(size_glb.x-1));   
        
        if (isMsd || isLowEnough)
        {
            if (isEmpty) addition_array[id_loc.y] = 0;

            // Sample brick
            brick = oct_brick[index];
            brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

            lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos, 0.0f)*3.5f , convert_float4(pool_dim));

            addition_array[id_loc.y] = read_imagef(pool, pool_sampler, lookup_pos).w;

            break;
        }
        else
        {
            // Descend to the next level
            index = (brick & mask_child_index);
            index += norm_index.x + norm_index.y*2 + norm_index.z*4;

            norm_pos = (norm_pos - (float3)((float)norm_index.x, (float)norm_index.y, (float)norm_index.z))*2.0f;
            norm_index = convert_int3(norm_pos);
            norm_index = clamp(norm_index, 0, 1);
        }
    }
        
    // Sum values in the local buffer (parallel reduction) and add them to the relevant position in the result vector
    barrier(CLK_LOCAL_MEM_FENCE);
    for (unsigned int i = (size_loc.x*size_loc.y*size_loc.z)/2; i > 0; i >>= 1)
    {
        if (get_local_id(1) < i)
        {
            addition_array[id_loc_linear] += addition_array[i + id_loc_linear];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if ((id_loc.x == 0) && (id_loc.y == 0) && (id_loc.z == 0))
    {
        result[id_result] = addition_array[0];
    }
}
