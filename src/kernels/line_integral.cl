__kernel void integrateLine(
    __read_only image3d_t pool, sampler_t pool_sampler,
    __global uint * oct_index, __global uint * oct_brick,
    __constant float * data_extent,
    __constant int * misc_int,
    __global float * result,
    float3 base_pos, float3 a, float3 b, float3 c, int3 samples_abc,
    __local float * addition_array
    )
{
    /*
    * Sample and integrate a rectangular volume around a line
    *
    * Problem layout
    *(y)
    * ^ 
    * |W W W W W 
    * |G G G G G
    * |W W W W W
    * |G G G G G
    * |W W W W W
    * |G G G G G
    * |-------------------> (x)
    *
    * --------------------> (Results vector)
    *
    * WGs have dimension n^2 x 1. The WI position in the work matrix translates to an 
    * WGs are spawned in horizontal batches left to right:
    *
    * W W W W W 
    * G G G G G ->
    *
    * The sampling geometry is that of a rectangular prism with extent given by three vectors a*samples.x, b*samples.y, and c*size_glb_(0). 
    * The vector c is spanned by a line from A to B in the host program, and a and b are dictated by it. 
    **/
    
    int id_loc_effective = get_local_id(0) + get_local_id(1)*get_local_size(0);
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));
    int2 problem_size = (int2)(samples_abc.x*samples_abc.y, samples_abc.z);
    
    if ((id_glb.x < problem_size.x) && (id_glb.y < problem_size.y))
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
        
        /* Sample the octree */
        int i_a = id_loc_effective % samples_abc.x; 
        int i_b = id_loc_effective / samples_abc.x;
        int i_c = get_global_id(0);    

        // xyz position (function of global id)
        float3 pos = base_pos + (float3)(
        a.x*((float)i_a) + b.x*((float)i_a) + c.x*((float)i_a),
        a.y*((float)i_b) + b.y*((float)i_b) + c.y*((float)i_b),
        a.z*((float)i_c) + b.z*((float)i_c) + c.z*((float)i_c));
    
        // Descend into the octree data structure
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
                if (isEmpty) addition_array[id_loc_effective] = 0;
    
                // Sample brick
                brick = oct_brick[index];
                brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);
    
                lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos, 0.0f)*3.5f , convert_float4(pool_dim));
    
                addition_array[id_loc_effective] = read_imagef(pool, pool_sampler, lookup_pos).w;
    
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
    }
    else
    {
        addition_array[id_loc_effective] = 0;
    }

    // Sum values in the local buffer (parallel reduction) and add them to the relevant position in the result vector
    barrier(CLK_LOCAL_MEM_FENCE);
    for (unsigned int i = get_local_size(0)/2; i > 0; i >>= 1)
    {
        if (get_local_id(0) < i)
        {
            addition_array[get_local_id(0)] += addition_array[i + get_local_id(0)];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (get_local_id(0) == 0)
    {
        result[get_global_id(0)] += addition_array[0];
    }
}
