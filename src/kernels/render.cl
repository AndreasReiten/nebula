int bounding_box_intersect(float3 r_origin, float3 r_delta, float * bbox, float * t_near, float * t_far)
{
    // This is simple ray-box intersection: http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm 
    
    // Compute relative intersects
    float3 r_delta_inv = native_divide((float3)(1.0f,1.0f,1.0f),r_delta);
    float3 T1 = ((float3)(bbox[0], bbox[2], bbox[4]) - r_origin)*r_delta_inv;
    float3 T2 = ((float3)(bbox[1], bbox[3], bbox[5]) - r_origin)*r_delta_inv;
    
    // Swap
    float3 t_min = min(T2, T1);
    float3 t_max = max(T2, T1);
    
    // Find largest Tmin and smallest Tmax
    float largest_t_min = max(max(t_min.x, t_min.y), max(t_min.x, t_min.z));
    float smallest_t_max = min(min(t_max.x, t_max.y), min(t_max.x, t_max.z));
    
    // Pass along and clamp to get correct start and stop factors
    *t_near = clamp(largest_t_min, 0.0f, 1.0f);
    *t_far = clamp(smallest_t_max, 0.0f, 1.0f);
    if (smallest_t_max < 0) return 0;
    return smallest_t_max > largest_t_min;
}


void sc2xyz(float4 * b, __constant float * A, float4 * x)
{
    // This is an adapted matrix multiplication function
    
    b->w = native_divide(1.0f, x->x*A[12] + x->y*A[13] + x->z*A[14] + x->w*A[15]);
    b->x = b->w * (x->x*A[0] + x->y*A[1] + x->z*A[2] + x->w*A[3]);
    b->y = b->w * (x->x*A[4] + x->y*A[5] + x->z*A[6] + x->w*A[7]);
    b->z = b->w * (x->x*A[8] + x->y*A[9] + x->z*A[10] + x->w*A[11]);
}



float TDS(float3 k, float3 em)
{
    // Calculate thermal diffuse scattering 
    
    float pi = 4.0*atan(1.0);
    
    float a = em.x * (2.0f - native_cos(pi*k.x)*(native_cos(pi*k.y) + native_cos(pi*k.z))) + (2.0f*em.z - em.x)*(1.0f - native_cos(pi*k.y)*native_cos(pi*k.z));
    
    float b = em.x * (2.0f - native_cos(pi*k.y)*(native_cos(pi*k.x) + native_cos(pi*k.z))) + (2.0f*em.z - em.x)*(1.0f - native_cos(pi*k.x)*native_cos(pi*k.z));
    
    float c = em.x * (2.0f - native_cos(pi*k.z)*(native_cos(pi*k.y) + native_cos(pi*k.x))) + (2.0f*em.z - em.x)*(1.0f - native_cos(pi*k.y)*native_cos(pi*k.x));
     
    float d = (em.y + em.z) * native_sin(pi*k.x) * native_sin(pi*k.y);
     
    float e = (em.y + em.z) * native_sin(pi*k.z) * native_sin(pi*k.y);
    
    float f = (em.y + em.z) * native_sin(pi*k.x) * native_sin(pi*k.z);
    
    
    
    float3 Ak = (float3)(
        (-k.x*e*e + f*k.y*e + d*k.z*e + b*c*k.x - c*d*k.y - b*f*k.z),
        (-k.y*f*f + e*k.x*f + d*k.z*f - c*d*k.x + a*c*k.y - a*e*k.z),
        (-k.z*d*d + e*k.x*d + f*k.y*d - b*f*k.x - a*e*k.y + a*b*k.z));
    return native_divide(1.0f,(a*b*c - a*e*e - b*f*f - c*d*d + 2.0f*d*e*f))*dot(k, Ak);
}


__kernel void SVO_RAYTRACE(
    __write_only image2d_t ray_tex,
    __read_only image2d_t tsf_tex, 
    __read_only image3d_t bricks,      
    __global uint * oct_index,
    __global uint * oct_brick,
    sampler_t brick_sampler,  
    sampler_t tsf_sampler,
    __constant float * data_view_matrix,
    __constant float * data_extent,
    __constant float * data_view_extent,
    __constant float * tsf_var,
    __constant float * misc_float,
    __constant int * misc_int)
{
    /* Octtree volume rendering. FYI:
     * tsf_var(iables) = [
     *      tsf_offset_low,
     *      tsf_offset_high, 
     *      data_offset_low,
     *      data_offset_high,
     *      alpha,
     *      intensity_multiplier]
     * 
     * misc_float = [
     *      ]
     * 
     * misc_int = [
     *      octtree_levels,
     *      brick_dimension,
     *      ]
    */
    
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    
    int2 ray_tex_dim = get_image_dim(ray_tex);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);
    int4 bricks_dim = get_image_dim(bricks);
    
    int log = misc_int[2];
    
    float2 data_limits = (float2)(tsf_var[2], tsf_var[3]);
    if (log)
    {
        if (data_limits.x <= 0) data_limits.x = 0.01;
        if (data_limits.y <= 0) data_limits.y = 0.01;
        data_limits.x = log10(data_limits.x);
        data_limits.y = log10(data_limits.y);
    }
    
    
    
    // If the global id corresponds to a texel
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        float4 color = (float4)(0.0, 0.0, 0.0, 0.0);
        
        float4 ray_near, ray_far, ray_delta, ray_far_edge, ray_delta_edge;
        {
            // Normalized device coordinates (screen coordinates)
            float2 ndc = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f)/convert_float2(ray_tex_dim)) - 1.0f);
            float2 ndc_edge = (float2)(2.0f * (( convert_float2(id_glb) + (float2)(0.5f, 1.0f))/convert_float2(ray_tex_dim)) - 1.0f);
            
            // Ray origin and exit (screen coordinates)
            float4 ray_near_ndc = (float4)(ndc, -1.0f, 1.0f); // z = -1 corresponds to near plane
            float4 ray_far_ndc = (float4)(ndc, 1.0f, 1.0f); // z = 1 corresponds to far plane
            float4 ray_far_ndc_edge = (float4)(ndc_edge, 1.0f, 1.0f); // z = 1 corresponds to far plane
            
            // Ray entry point at near and far plane 
            sc2xyz(&ray_near, data_view_matrix, &ray_near_ndc);
            sc2xyz(&ray_far, data_view_matrix, &ray_far_ndc);
            sc2xyz(&ray_far_edge, data_view_matrix, &ray_far_ndc_edge);
        }
        if (misc_int[5])
        {
            ray_near = (float4)(data_view_matrix[3], data_view_matrix[7], data_view_matrix[11], 1.0f)*native_divide(1.0f, data_view_matrix[15]);
        }
        ray_delta = ray_far -  ray_near;
        
        // The cone width at the far plane
        float cone_width, cone_increment;
        // remove fabs, scale ray far and ray edge, multiply by 2 or so, 
        if (misc_int[5]) 
        {
            ray_delta_edge = ray_far_edge -  ray_near;
            ray_delta_edge = normalize(ray_delta_edge)*length(ray_delta);
            cone_width =  2.0*length(ray_delta_edge.xyz - ray_delta.xyz);
            cone_increment = cone_width/length(ray_delta.xyz);
        }
        else
        {
            cone_width =  2.0*length(ray_far_edge.xyz - ray_far.xyz);
            cone_increment = 0;
        }
        
        int hit;
        float t_near, t_far;
        {
            // Construct a bounding box from the intersect between data_view_extent and data_extent
            float bbox[6];
            
            bbox[0] = fmax(data_extent[0],data_view_extent[0]);
            bbox[1] = fmin(data_extent[1],data_view_extent[1]);
            bbox[2] = fmax(data_extent[2],data_view_extent[2]);
            bbox[3] = fmin(data_extent[3],data_view_extent[3]);
            bbox[4] = fmax(data_extent[4],data_view_extent[4]);
            bbox[5] = fmin(data_extent[5],data_view_extent[5]);
            
            // Does the ray for this pixel intersect bbox?
            if (!((bbox[0] >= bbox[1]) || (bbox[2] >= bbox[3]) || (bbox[4] >= bbox[5])))
            {
                hit = bounding_box_intersect(ray_near.xyz, ray_delta.xyz, bbox, &t_near, &t_far);
            }
        }
        
        if(hit)
        {
            float4 sample;
            
            // The step length is chosen such that there is roughly a set number of samples per voxel per ray
            // Ideally this is a number that changes based on the pseudo-level the ray is currently traversing (interpolation between two voxel sizes) 
            
            
            float norm_bbox[6];
            norm_bbox[0] = 0.0f;
            norm_bbox[1] = 2.0f;
            norm_bbox[2] = 0.0f;
            norm_bbox[3] = 2.0f;
            norm_bbox[4] = 0.0f;
            norm_bbox[5] = 2.0f;
            
            // Merged bits in the octtree can be read using these bitmasks:
            uint mask_msd_flag = ((1 << 1) - 1) << 31;
            uint mask_data_flag = ((1 << 1) - 1) << 30;
            uint mask_child_index = ((1 << 30) - 1) << 0;
            uint mask_brick_id_x = ((1 << 10) - 1) << 20;
            uint mask_brick_id_y = ((1 << 10) - 1) << 10;
            uint mask_brick_id_z = ((1 << 10) - 1) << 0;
            
            // Note that the origin and end are rounded to the nearest step length
            float3 ray_box_origin = ray_near.xyz + t_near * ray_delta.xyz;
            float3 ray_box_end = ray_near.xyz + t_far * ray_delta.xyz;
            float3 ray_box_delta = ray_box_end - ray_box_origin;
            
            
            
            float3 direction = normalize(ray_box_delta);
            float node_step_length, skip_length, intensity;
            
            // Traverse the octtree. For each step, descend into the octree until a) the resolution is appreciable or b) the final level of the octree is reached. During any descent, empty nodes might be found. In such case, the ray is advanced forward without sampling to the next sample that is not in said node.
            float voxel_size[20];
            for (int j = 0; j < 20; j++)
            {
                voxel_size[j] = (data_extent[1] - data_extent[0])/((float)((misc_int[1]-1) * (1 << j)));
            }
            
            float r, f, p;
            float r_low = voxel_size[misc_int[0]-1];
            float r_high = voxel_size[0];
            uint index, index_prev, brick, isMsd, isLowEnough, isEmpty;
            float2 tsf_position;
            float3 ray_box_xyz;
            float3 norm_xyz;
            float3 norm_xyz_prev;
            float3 ray_box_xyz_prev;
            float3 tmp_a;
            float3 tmp_b;
            float4 lookup_pos;
            uint4 brick_id;
            int3 norm_index;
            
            ray_box_xyz = ray_box_origin;
            ray_box_xyz_prev = (float3)(ray_box_xyz);
            float norm_step_length = native_divide(2.0f,(float)((1 << misc_int[0])*(misc_int[1]-1)))*0.25;
            
            float step_length = native_divide((data_extent[1] - data_extent[0]),(float)((1 << misc_int[0])*(misc_int[1]-1)))*0.25; 
            float3 ray_box_add = direction * step_length;
            
            int color_fetch_counter = 0;
            while ( fast_length(ray_box_xyz - ray_box_origin) < fast_length(ray_box_delta) )
            {
                if (color.w > 0.995) break;
                
                index = 0;
                index_prev = 0;
                
                if (misc_int[5]) r = length(ray_near + ray_box_xyz)*cone_increment*misc_float[5];
                else r = cone_width*misc_float[5];
                r = clamp(r, r_low, r_high);
                step_length = r*0.25; 
                ray_box_add = direction * step_length;
                
                norm_xyz = native_divide( (float3)(ray_box_xyz.x - data_extent[0], ray_box_xyz.y - data_extent[2], ray_box_xyz.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;
                
                norm_index = convert_int3(norm_xyz);
                norm_index = clamp(norm_index, 0, 1);
                
                
                if (((norm_xyz.x < 0) || (norm_xyz.x >= 2.0f)) || ((norm_xyz.y < 0) || (norm_xyz.y >= 2.0f)) || ((norm_xyz.z < 0) || (norm_xyz.z >= 2.0f)))
                {
                    ray_box_xyz += ray_box_add; 
                    continue;
                }
                
                // Descend down into the tree
                for (int j = 0; j < misc_int[0]; j++)
                {
                    isMsd = (oct_index[index] & mask_msd_flag) >> 31;
                    isEmpty = !((oct_index[index] & mask_data_flag) >> 30);
                    isLowEnough = (r > voxel_size[j]);
                    
                    if (isEmpty)
                    {
                        // Skip forward by calculating how many steps can be advanced before reachign the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;
                        if (misc_int[3])
                        {
                            sample = (float4)(1.0,1.0,1.0, 0.08);
                    
                            f = (1.0f - color.w)*sample.w;
                            //~ p = native_powr(f,0.5);
                            if (color_fetch_counter == 0) color = sample;
                            else color.xyz = mix(color.xyz, sample.xyz, (float3)(f));
                            color_fetch_counter++;
                            
                            color.w += f;
                        }
                        
                        
                        tmp_a = norm_xyz - 5.0f*direction;
                        tmp_b = 15.0f*direction;
                        hit = bounding_box_intersect(tmp_a, tmp_b, norm_bbox, &t_near, &t_far);
                        
                        if (hit)
                        {
                            skip_length = native_divide(fast_length((tmp_a + t_far*tmp_b) - norm_xyz), 2.0f);
                            skip_length = skip_length * voxel_size[j]* (misc_int[1]-1);
                            skip_length += step_length*0.01;
                            ray_box_xyz += skip_length * direction;
                        }
                        
                        break;
                    }
                    else if (isMsd || isLowEnough)
                    {
                        // Sample brick
                        
                        if (isLowEnough && (j >= 1))
                        {
                            /* Quadrilinear interpolation between two bricks */
                            
                            // The brick in the level above
                            brick = oct_brick[index_prev];
                            brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);
                            
                            
                            lookup_pos = native_divide(0.5 + convert_float4(brick_id * misc_int[1])  + (float4)(norm_xyz_prev, 0.0f)*3.5 , convert_float4(bricks_dim));
                            
                            float intensity_prev = read_imagef(bricks, brick_sampler, lookup_pos).w;
                            
                            // The brick in the current level
                            brick = oct_brick[index];
                            brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);
                            
                            lookup_pos = native_divide(0.5 + convert_float4(brick_id * misc_int[1])  + (float4)(norm_xyz, 0.0f)*3.5 , convert_float4(bricks_dim));
                            
                            float intensity_here = read_imagef(bricks, brick_sampler, lookup_pos).w;
                            
                            // Linear interpolation between the two intensities
                            intensity = intensity_prev + (intensity_here - intensity_prev)*native_divide(r - voxel_size[j-1], voxel_size[j] - voxel_size[j-1]);
                        }
                        else
                        {
                            brick = oct_brick[index];
                            brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);
                            
                            lookup_pos = native_divide(0.5 + convert_float4(brick_id * misc_int[1])  + (float4)(norm_xyz, 0.0f)*3.5 , convert_float4(bricks_dim));
                            
                            intensity = read_imagef(bricks, brick_sampler, lookup_pos).w;
                        }
                        
                        if (misc_int[3])
                        {
                            sample = (float4)(0.2,0.3,1.0, 1.00);
                    
                            float f = (1.0f - color.w)*sample.w;
                            //~ float p = native_powr(f,0.5);
                            
                            if (color_fetch_counter == 0) color = sample;
                            else color.xyz = mix(color.xyz, sample.xyz, (float3)(f));
                            color_fetch_counter++;
                            
                            color.w += f;
                        }
                        
                        
                        // Sample color
                        if(log)
                        {
                            intensity = log10(intensity); 
                        }
                        
                        tsf_position = (float2)(tsf_var[0] + (tsf_var[1] - tsf_var[0]) * ((intensity - data_limits.x)/(data_limits.y - data_limits.x)), 0.5f);
                        
                        sample = read_imagef(tsf_tex, tsf_sampler, tsf_position);
                        
                        sample.w *= tsf_var[4]; 
                        
                        float steps = native_divide(r, r_low);
                        float rest_step = fmod(steps, 1.0);
                        int cycles = (int) steps;
                        
                        // The first color should be that of the first sample
                        for (int k = 0; k < cycles; k++)
                        {
                            f = (1.0f - color.w)*sample.w;
                            //~ p = native_powr(f,0.5);
                            
                            if (color_fetch_counter == 0) color = sample;
                            else color.xyz = mix(color.xyz, sample.xyz, (float3)(f));
                            color_fetch_counter++;
                            
                            color.w += f;
                        }
                        if (rest_step > 0)
                        {
                            f = (1.0f - color.w)*sample.w*rest_step;
                            //~ p = native_powr(f,0.5);
                            
                            if (color_fetch_counter == 0) color = sample;
                            else color.xyz = mix(color.xyz, sample.xyz, (float3)(f));
                            color_fetch_counter++;
                            
                            color.w += f;
                        }
                    
                        ray_box_xyz += ray_box_add;
                        
                        break;
                        
                    }
                    else
                    {
                        // Save values from this level to enable quadrilinear interpolation between levels
                        index_prev = index;
                        norm_xyz_prev = norm_xyz;
                        
                        // Descend to the next level
                        index = (oct_index[index] & mask_child_index); 
                        
                        index += norm_index.x + norm_index.y*2 + norm_index.z*4;
                        
                        norm_xyz = (norm_xyz - convert_float(norm_index))*2.0f;
                        norm_index = convert_int3(norm_xyz);
                        norm_index = clamp(norm_index, 0, 1); 
                    }
                }
                
                if (fast_distance(ray_box_xyz, ray_box_xyz_prev) < step_length*0.5)
                {
                    ray_box_xyz += ray_box_add*0.5;
                }
                ray_box_xyz_prev = ray_box_xyz;
            }
            if (!misc_int[3])color *= tsf_var[5];
        }
        write_imagef(ray_tex, id_glb, clamp(color, 0.0, 1.0));
    }
}



__kernel void FUNCTION_RAYTRACE(
    __write_only image2d_t ray_tex,
    __read_only image2d_t tsf_tex, 
    sampler_t tsf_sampler,
    __constant float * data_view_matrix,
    __constant float * data_extent,
    __constant float * data_view_extent,
    __constant float * tsf_var,
    __constant float * misc_float,
    __constant int * misc_int)
{
    float pi = 4.0*atan(1.0);
    
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    
    int2 ray_tex_dim = get_image_dim(ray_tex);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);
    
    int log = misc_int[2];
    
    float2 data_limits = (float2)(tsf_var[2], tsf_var[3]);
    if (log)
    {
        if (data_limits.x <= 0) data_limits.x = 0.01;
        if (data_limits.y <= 0) data_limits.y = 0.01;
        data_limits.x = log10(data_limits.x);
        data_limits.y = log10(data_limits.y);
    }
    
    
    
    // If the global id corresponds to a texel
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        float4 ray_far, ray_near, ray_delta;
        {
            // Normalized device coordinates (screen coordinates)
            float2 ndc = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f)/convert_float2(ray_tex_dim)) - 1.0f);
            
            // Ray origin and exit (screen coordinates)
            float4 ray_far_ndc = (float4)(ndc, 1.0f, 1.0f); // z = 1 corresponds to far plane
            float4 ray_near_ndc = (float4)(ndc, -1.0f, 1.0f); // z = -1 corresponds to near plane
            
            // Ray entry point at near and far plane 
            sc2xyz(&ray_far, data_view_matrix, &ray_far_ndc);
            sc2xyz(&ray_near, data_view_matrix, &ray_near_ndc);
        }
        if (misc_int[5])
        {
            ray_near = (float4)(data_view_matrix[3], data_view_matrix[7], data_view_matrix[11], 1.0f)*native_divide(1.0f, data_view_matrix[15]);
        }
        
        ray_delta = ray_far - ray_near;
        
        int hit;
        float t_near, t_far;
        {
            // Construct a bounding box from the intersect between data_view_extent and data_extent
            float bbox[6];
            
            bbox[0] = data_view_extent[0];
            bbox[1] = data_view_extent[1];
            bbox[2] = data_view_extent[2];
            bbox[3] = data_view_extent[3];
            bbox[4] = data_view_extent[4];
            bbox[5] = data_view_extent[5];
            
            // Does the ray for this pixel intersect bbox?
            if (!((bbox[0] >= bbox[1]) || (bbox[2] >= bbox[3]) || (bbox[4] >= bbox[5])))
            {
                hit = bounding_box_intersect(ray_near.xyz, ray_delta.xyz, bbox, &t_near, &t_far);
            }
        }
        
        float4 color = (float4)(0.0, 0.0, 0.0, 0.0);
        float4 sample = (float4)(0.0, 0.0, 0.0, 0.0);
        float scaling = native_divide((data_view_extent[1] - data_view_extent[0]),(data_extent[1] - data_extent[0]));
        
        if(hit)
        {
            float4 ray_box_origin, ray_box_delta, ray_box_end, ray_box_add;
            ray_box_origin = ray_near + t_near * ray_delta;
            ray_box_delta = (t_far - t_near) * ray_delta;
            ray_box_end = ray_box_origin + ray_box_delta;
            ray_box_add = normalize(ray_box_delta)*native_divide(data_view_extent[1]-data_view_extent[0], 400.0);
            
            float ray_box_length = fast_length(ray_box_delta.xyz);
            int cycles = (int) (ray_box_length/fast_length(ray_box_add));

            
            float3 xyz;
            float val;
            int func = 1;
            int traversal_order = 0;
            
            for (int i = 0; i < cycles; i++)
            {
                /* Get xyz position of ray */
                // Front-to-back
                if (traversal_order == 0)
                {
                    xyz = ray_box_origin.xyz + i*ray_box_add.xyz;
                }
                // Back-to-front
                else if (traversal_order == 1)
                {
                    xyz = ray_box_end.xyz - i*ray_box_add.xyz;
                }
                    
                val = exp(-2.0*pi*(xyz.x*xyz.x + xyz.y*xyz.y + xyz.z*xyz.z)*misc_float[3])*TDS(xyz, (float3)(misc_float[0],misc_float[1],misc_float[2]));
                
                
                if(log)
                {
                    val = log10(val); 
                }
                
                
                // GET CORRESPONDING RGBA VALUE
                if (val >= data_limits.x)
                {
                    float2 tsf_position = (float2)(tsf_var[0] + (tsf_var[1] - tsf_var[0]) * ((val - data_limits.x)/(data_limits.y - data_limits.x)), 0.5f);
                    
                    //~ tsf_position = clamp(tsf_position,0.0f,1.0f);
                    
                    //~ tsf_position = clamp(tsf_position, 0.0, 0.9); // should be superfluous
                    
                    sample = read_imagef(tsf_tex, tsf_sampler, tsf_position);
                    
                    
                    
                    if (traversal_order == 0)
                    {
                        sample.w *= tsf_var[4]*scaling;
                        
                        if(func == 0)
                        {
                            color.xyz += (1.0f - color.w)*sample.xyz;
                            color.w += (1.0f - color.w)*sample.w;
                        }
                        else if(func == 1)
                        {
                            float f = (1.0f - color.w)*sample.w;
                            float p = native_powr(f,0.5);
                            
                            color.xyz = mix(color.xyz, sample.xyz, (float3)(p)); 
                            color.w += f;
                        }
                        else if(func == 2)
                        {
                            color.xyz = mix(color.xyz, sample.xyz, (float3)(1.0f- color.w)*sample.w);
                            color.w += sample.w;
                        }
                        
                        if (color.w > 0.999) break;
                    }
                    else if (traversal_order == 1)
                    {
                        if(func == 0)
                        {
                            float a = sample.w*tsf_var[4];
                            color = mix(color, sample, (float4)(a));
                        }
                    }
                    
                }
            }
            
            color *= tsf_var[5];
        }
        
        write_imagef(ray_tex, id_glb, clamp(color, 0.0, 1.0));
    }
}
