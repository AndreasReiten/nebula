__kernel void integrateLine(
    __read_only image3d_t bricks,
    __global uint * oct_index,
    __global uint * oct_brick,
    sampler_t brick_sampler,
    __constant float * data_extent,
    __constant int * misc_int
)
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    
    int4 pool_dim = get_image_dim(bricks);
    
    int n_tree_levels = misc_int[0];
    int brick_dim = misc_int[1];
        
    // Some variables we will need during ray traversal
    float skip_length, intensity, voxel_size_this_lvl, voxel_size_prev_lvl;
    uint index_this_lvl, index_prev_lvl, brick, isMsd, isLowEnough, isEmpty;
    float3 pos, ray_add_box;
    float3 norm_pos_this_lvl, norm_pos_prev_lvl;
    float3 tmp_a, tmp_b;
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
    
    /* Traverse the octree. For each step, descend into the octree until a) The resolution is appreciable or b) The final level of the octree is reached. During any descent, empty nodes might be found. In such case, the ray is advanced forward without sampling to the next sample that is not in said node. Stuff inside this while loop is what really takes time and therefore should be optimized */
    
    pos = ;

    // Descend into the octree data structure
    // Index trackers for the traversal.
    index_this_lvl = 0;
    index_prev_lvl = 0;

     // We use a normalized convention during octree traversal. The normalized convention makes it easier to think about the octree traversal.
    norm_pos_this_lvl = native_divide( (float3)(pos.x - data_extent[0], pos.y - data_extent[2], pos.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;

    norm_index = convert_int3(norm_pos_this_lvl);
    norm_index = clamp(norm_index, 0, 1);

    // Traverse the octree
    for (int j = 0; j < n_tree_levels; j++)
    {
        voxel_size_this_lvl = (data_extent[1] - data_extent[0])/((float)((brick_dim-1) * (1 << j)));
        if (j > 0) voxel_size_prev_lvl = (data_extent[1] - data_extent[0])/((float)((brick_dim-1) * (1 << (j-1))));

        brick = oct_index[index_this_lvl];
        isMsd = (brick & mask_msd_flag) >> 31;
        isEmpty = !((brick & mask_data_flag) >> 30);

        if (isEmpty)
        {
            // Skip forward by calculating how many steps can be advanced before reaching the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;

            // This ugly shit needs to go
            tmp_a = norm_pos_this_lvl - 5.0f*direction;
            tmp_b = 15.0f*direction;
            hit = boundingBoxIntersectNorm(tmp_a, tmp_b, &t_near, &t_far);

            if (hit)
            {
                break;
            }
        }
        else if (isMsd)
        {
            // Sample brick
            brick = oct_brick[index_this_lvl];
            brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

            lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_this_lvl, 0.0f)*3.5f , convert_float4(pool_dim));

            intensity = read_imagef(bricks, brick_sampler, lookup_pos).w;
        }
        else
        {
            // Save values from this level to enable quadrilinear interpolation between levels
            index_prev_lvl = index_this_lvl;
            norm_pos_prev_lvl = norm_pos_this_lvl;

            // Descend to the next level
            index_this_lvl = (brick & mask_child_index);
            index_this_lvl += norm_index.x + norm_index.y*2 + norm_index.z*4;

            //norm_pos_this_lvl = (norm_pos_this_lvl - convert_float(norm_index))*2.0f;
            norm_pos_this_lvl = (norm_pos_this_lvl - (float3)((float)norm_index.x, (float)norm_index.y, (float)norm_index.z))*2.0f;
            norm_index = convert_int3(norm_pos_this_lvl);
            norm_index = clamp(norm_index, 0, 1);
        }
    }
}
