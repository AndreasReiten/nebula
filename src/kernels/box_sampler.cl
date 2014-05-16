__kernel void boxSample(
    __read_only image3d_t bricks,
    __global uint * oct_index,
    __global uint * oct_brick,
    __constant float * data_view_extent,
    sampler_t brick_sampler,
    unsigned int n_tree_levels,
    unsigned int brick_dim,
    __global float * output)
{
//     Variables needed for sampling
    int4 bricks_dim = get_image_dim(bricks);

    float3 pos;
    pos.x = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0])*native_divide((float)get_global_id(0)+0.5,(float)get_global_size(0));
    pos.y = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2])*native_divide((float)get_global_id(1)+0.5,(float)get_global_size(1));
    pos.z = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4])*native_divide((float)get_global_id(2)+0.5,(float)get_global_size(2));

    uint brick, isMsd, isLowEnough, isEmpty;
    float4 lookup_pos;
    uint4 brick_id;
    int3 norm_index;
    float intensity, intensity_prev_lvl, intensity_this_lvl;
    float voxel_size_prev_lvl, voxel_size_this_lvl;
    float sample_interdist = native_divide(data_view_extent[1] - data_view_extent[0], (float) get_local_size(0));
    float volume = sample_interdist*sample_interdist*sample_interdist;

    // Merged bits in the octtree can be read using these bitmasks
    uint mask_msd_flag = ((1u << 1u) - 1u) << 31u;
    uint mask_data_flag = ((1 << 1) - 1) << 30;
    uint mask_child_index = ((1 << 30) - 1) << 0;
    uint mask_brick_id_x = ((1 << 10) - 1) << 20;
    uint mask_brick_id_y = ((1 << 10) - 1) << 10;
    uint mask_brick_id_z = ((1 << 10) - 1) << 0;


    // Index trackers for sampling
    uint index_this_lvl = 0;
    uint index_prev_lvl = 0;

    // Normalized xyz coordinate
    float3 norm_pos_this_lvl = native_divide( (float3)(pos.x - data_view_extent[0], pos.y - data_view_extent[2], pos.z - data_view_extent[4]), (float3)(data_view_extent[1] - data_view_extent[0], data_view_extent[3] - data_view_extent[2], data_view_extent[5] - data_view_extent[4])) * 2.0f;
    float3 norm_pos_prev_lvl;

    // The octant index corresponding to norm_pos_this_lvl
    norm_index = convert_int3(norm_pos_this_lvl);
    norm_index = clamp(norm_index, 0, 1);

    // Traverse the octtree
    for (int j = 0; j < n_tree_levels; j++)
    {
        voxel_size_this_lvl = (data_view_extent[1] - data_view_extent[0])/((float)((brick_dim-1) * (1 << j)));
        if (j > 0) voxel_size_prev_lvl = (data_view_extent[1] - data_view_extent[0])/((float)((brick_dim-1) * (1 << (j-1))));

        isMsd = (oct_index[index_this_lvl] & mask_msd_flag) >> 31;
        isEmpty = !((oct_index[index_this_lvl] & mask_data_flag) >> 30);
        isLowEnough = (sample_interdist > voxel_size_this_lvl);

        if (isMsd || isLowEnough || isEmpty)
        {
            // Sample brick
            if (j >= 1)
            {
                /* Quadrilinear interpolation between two bricks */

                // The brick in the level above
                brick = oct_brick[index_prev_lvl];
                brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_prev_lvl, 0.0f)*3.5f , convert_float4(bricks_dim));

                intensity_prev_lvl = read_imagef(bricks, brick_sampler, lookup_pos).w;

                // The brick in the current level
                brick = oct_brick[index_this_lvl];
                brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_this_lvl, 0.0f)*3.5f , convert_float4(bricks_dim));

                if (isEmpty) intensity_this_lvl = 0;
                else intensity_this_lvl = read_imagef(bricks, brick_sampler, lookup_pos).w;

                // Linear interpolation between the two intensities
                clamp(sample_interdist, voxel_size_this_lvl, voxel_size_prev_lvl);

                intensity = intensity_prev_lvl + (intensity_this_lvl - intensity_prev_lvl)*native_divide(sample_interdist - voxel_size_prev_lvl, voxel_size_this_lvl - voxel_size_prev_lvl);
            }
            else
            {
                brick = oct_brick[index_this_lvl];
                brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_this_lvl, 0.0f)*3.5f , convert_float4(bricks_dim));

                intensity = read_imagef(bricks, brick_sampler, lookup_pos).w;
            }

            output[get_global_id(0) + get_global_id(1)*get_global_size(0) + get_global_id(2)*get_global_size(0)*get_global_size(1)] = intensity / volume;
            break;
        }
        else
        {
            // Save values from this level to enable quadrilinear interpolation between levels
            index_prev_lvl = index_this_lvl;
            norm_pos_prev_lvl = norm_pos_this_lvl;

            // Prepare to descend to the next level
            index_this_lvl = (oct_index[index_this_lvl] & mask_child_index);
            index_this_lvl += norm_index.x + norm_index.y*2 + norm_index.z*4;

            norm_pos_this_lvl = (norm_pos_this_lvl - (float3)((float)norm_index.x, (float)norm_index.y, (float)norm_index.z))*2.0f;
            norm_index = convert_int3(norm_pos_this_lvl);
            norm_index = clamp(norm_index, 0, 1);
        }
    }
//    unsigned int id = get_global_id(0) + get_global_id(1)*get_global_size(0) + get_global_id(2)*get_global_size(0)*get_global_size(1);
//    output[id] = 1.0f;
}
