__kernel void integrateImage(
    __read_only image2d_t source,
    __write_only image2d_t target,
    __local float * addition_array,
    sampler_t source_sampler,
    int direction
    )
{
    /* 
    * Parallel reduction of an image. Depending on the direction, all rows or columns are summed in chunks and written to a target. It is up to the host to determine how many times an image should be integrated in this way.  
    */
    
    // sum along w (rows, x): direction = 0
    // sum along h (columns, y): direction = 1
    
    int id_loc[2];
    id_loc[0] = get_local_id(0);
    id_loc[1] = get_local_id(1);
        
    int local_size[2];
    local_size[0]  = get_local_size(0);
    local_size[1]  = get_local_size(1);
    
    int id_glb[2];
    id_glb[0] = get_global_id(0);
    id_glb[1] = get_global_id(1);
    
    int source_dim[2];
    source_dim[0]  = get_image_dim(source).x;
    source_dim[1]  = get_image_dim(source).y;
    
    int2 id_out = (int2)(id_glb[direction]/local_size[direction], id_glb[!direction]);
    
    if (id_glb[direction] < source_dim[direction])
    {
        addition_array[id_loc[direction]] = read_imagef(source, source_sampler, (int2)(id_glb[direction], id_glb[!direction])).w;    
    }
    else
    {
        addition_array[id_loc[direction]] = 0.0f;
    }
    
    if (id_glb[direction] < source_dim[direction])
    {
        barrier(CLK_LOCAL_MEM_FENCE);
        for (unsigned int i = local_size[direction]/2; i > 0; i >>= 1)
        {
            if (id_loc[direction] < i)
            {
                addition_array[id_loc[direction]] += addition_array[i + id_loc[direction]];
            }
            barrier(CLK_LOCAL_MEM_FENCE);
        }        
        
        float4 sample;
        sample = (float4)(addition_array[direction]);

        // Write to target 
        if (id_loc[direction] == 0) write_imagef(target, id_out, sample);
    }
    else
    {
        float4 sample;
        sample = (float4)(addition_array[0]);

        if (id_loc[direction] == 0) write_imagef(target, id_out, sample);
    }
}
