kernel void integratePlane(
    read_only image3d_t pool, sampler_t pool_sampler,
    global uint * oct_index, global uint * oct_brick,
    constant float * data_extent,
    constant int * misc_int,
    global float * result,
    float3 base_pos, float3 a, float3 b, float3 c, int3 samples_abc,
    local float * addition_array
)
{
    /*
    * Sample and integrate a rectangular volume onto its "primary" face
    * WGs are 1x1xNUM
    **/

    int3 id_loc = (int3)(get_local_id(0), get_local_id(1), get_local_id(2));
    int3 id_glb = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));
    int3 id_grp = (int3)(get_group_id(0), get_group_id(1), get_group_id(2));
    int3 size_grp = (int3)(get_num_groups(0), get_num_groups(1), get_num_groups(2));
    int3 size_loc = (int3)(get_local_size(0), get_local_size(1), get_local_size(2));
    int3 size_glb = (int3)(get_global_size(0), get_global_size(1), get_global_size(2));

    int3 size_problem = (int3)(samples_abc.x, samples_abc.y, samples_abc.z);

    int id_loc_linear = id_loc.z;
    int id_result = id_grp.x + id_grp.y * size_grp.x;
    int size_loc_linear = size_loc.x * size_loc.y * size_loc.z;

    if (id_loc_linear == 0)
    {
        result[id_result] = 0;
    }

    for (int iter = 0; iter < (size_problem.z / size_loc_linear + 1); iter++)
    {
        int3 id_problem = (int3)(id_grp.x, id_grp.y, id_loc_linear + iter * size_loc_linear);

        if ((id_problem.z < size_problem.z))
        {
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

            // Sample the octtree
            // xyz position
            float3 pos = base_pos + (float3)(
                             a.x * ((float)id_problem.x) + b.x * ((float)id_problem.y) + c.x * ((float)id_problem.z),
                             a.y * ((float)id_problem.x) + b.y * ((float)id_problem.y) + c.y * ((float)id_problem.z),
                             a.z * ((float)id_problem.x) + b.z * ((float)id_problem.y) + c.z * ((float)id_problem.z));

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

                if (isMsd)
                {
                    if (isEmpty)
                    {
                        addition_array[id_loc_linear] = 0;
                    }
                    else
                    {
                        // Sample brick
                        brick = oct_brick[index];
                        brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                        lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos, 0.0f) * 3.5f , convert_float4(pool_dim));

                        addition_array[id_loc_linear] = read_imagef(pool, pool_sampler, lookup_pos).w;
                    }

                    break;
                }
                else
                {
                    // Descend to the next level
                    index = (brick & mask_child_index);
                    index += norm_index.x + norm_index.y * 2 + norm_index.z * 4;

                    norm_pos = (norm_pos - (float3)((float)norm_index.x, (float)norm_index.y, (float)norm_index.z)) * 2.0f;
                    norm_index = convert_int3(norm_pos);
                    norm_index = clamp(norm_index, 0, 1);
                }
            }
        }
        else
        {
            addition_array[id_loc_linear] = 0;
        }



        // Sum values in the local buffer (parallel reduction) and add them to the relevant position in the result vector
        barrier(CLK_LOCAL_MEM_FENCE);

        for (unsigned int i = size_loc_linear / 2; i > 0; i >>= 1)
        {
            if (id_loc_linear < i)
            {
                addition_array[id_loc_linear] += addition_array[i + id_loc_linear];
            }

            barrier(CLK_LOCAL_MEM_FENCE);
        }


        if (id_loc_linear == 0)
        {
            result[id_result] += addition_array[0];
        }
    }
}
