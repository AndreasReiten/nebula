int bounding_box_intersect(float3 r_origin, float3 r_delta, float * bbox, float * t_near, float * t_far)
{
    // This is simple ray-box intersection: http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm 
    
    // Compute relative intersects
    float3 r_delta_inv = native_divide((float3)(1.0f),r_delta);
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

int bounding_box_intersect_oct(float3 r_origin, float3 r_delta, float * t_near, float * t_far)
{
    // This is simple ray-box intersection: http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm 
    
    // Compute relative intersects
    float3 r_delta_inv = native_divide((float3)(1.0f),r_delta);
    float3 T1 = ((float3)(0.0) - r_origin)*r_delta_inv;
    float3 T2 = ((float3)(2.0) - r_origin)*r_delta_inv;
    
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



float model(float3 k, __constant float * param)
{
    // Calculate the model
    
    float a = param[0] * (2.0f - native_cos(k.x)*(native_cos(k.y) + native_cos(k.z))) + (2.0f*param[2] - param[0])*(1.0f - native_cos(k.y)*native_cos(k.z));
    
    float b = param[0] * (2.0f - native_cos(k.y)*(native_cos(k.x) + native_cos(k.z))) + (2.0f*param[2] - param[0])*(1.0f - native_cos(k.x)*native_cos(k.z));
    
    float c = param[0] * (2.0f - native_cos(k.z)*(native_cos(k.y) + native_cos(k.x))) + (2.0f*param[2] - param[0])*(1.0f - native_cos(k.y)*native_cos(k.x));
     
    float d = (param[1] + param[2]) * native_sin(k.x) * native_sin(k.y);
     
    float e = (param[1] + param[2]) * native_sin(k.z) * native_sin(k.y);
    
    float f = (param[1] + param[2]) * native_sin(k.x) * native_sin(k.z);
    
    
    float3 Ak = (float3)(
        (-k.x*e*e + f*k.y*e + d*k.z*e + b*c*k.x - c*d*k.y - b*f*k.z),
        (-k.y*f*f + e*k.x*f + d*k.z*f - c*d*k.x + a*c*k.y - a*e*k.z),
        (-k.z*d*d + e*k.x*d + f*k.y*d - b*f*k.x - a*e*k.y + a*b*k.z));
        
    return exp(-2.0*(k.x*k.x + k.y*k.y + k.z*k.z)*param[3])*native_divide(1.0f,(a*b*c - a*e*e - b*f*f - c*d*d + 2.0f*d*e*f))*dot(k, Ak);
}


__kernel void svoRayTrace(
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
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    
    int2 ray_tex_dim = get_image_dim(ray_tex);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);
    int4 bricks_dim = get_image_dim(bricks);

    int numOctLevels = misc_int[0];
    int brickSize = misc_int[1];
    int isLogActive = misc_int[2];
    int isDsActive = misc_int[3];
    float step_length_factor = misc_float[5];

    float tsf_offset_low = tsf_var[0];
    float tsf_offset_high = tsf_var[1]; 
    float data_offset_low = tsf_var[2];
    float data_offset_high = tsf_var[3];
    float alpha = tsf_var[4];
    float brightness = tsf_var[5];
    
    float2 data_limits = (float2)(data_offset_low, data_offset_high);

    if (isLogActive)
    {
        if (data_limits.x <= 0) data_limits.x = 0.01;
        if (data_limits.y <= 0) data_limits.y = 0.01;
        data_limits.x = log10(data_limits.x);
        data_limits.y = log10(data_limits.y);
    }
    
    // If the global id corresponds to a texel, then check if its associated ray hits our cubic bounding box. If it does - traverse along the intersecing ray segment and accumulate color 
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        /*
         * Find the geometry of the ray
         * */
        float4 ray_near, ray_far;
        float3 ray_delta;
        float cone_diameter_increment;
        float cone_diameter_near;
        {
            float4 ray_near_edge, ray_far_edge;
            float3 pixel_radius_near, pixel_radius_far;
            
            // Normalized device coordinates (ndc) of the pixel and its edge (in screen coordinates)
            float2 ndc = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f)/convert_float2(ray_tex_dim)) - 1.0f);
            float2 ndc_edge = (float2)(2.0f * (( convert_float2(id_glb) + (float2)(1.0f, 1.0f))/convert_float2(ray_tex_dim)) - 1.0f);
            
            // Ray origin and exit point (screen coordinates)
            // z = 1 corresponds to far plane
            // z = -1 corresponds to near plane
            float4 ray_near_ndc = (float4)(ndc, -1.0f, 1.0f);
            float4 ray_far_ndc = (float4)(ndc, 1.0f, 1.0f);

            float4 ray_near_ndc_edge = (float4)(ndc_edge, -1.0f, 1.0f); 
            float4 ray_far_ndc_edge = (float4)(ndc_edge, 1.0f, 1.0f); 
            
            // Ray entry point at near and far plane 
            sc2xyz(&ray_near, data_view_matrix, &ray_near_ndc);
            sc2xyz(&ray_far, data_view_matrix, &ray_far_ndc);
            sc2xyz(&ray_near_edge, data_view_matrix, &ray_near_ndc_edge);
            sc2xyz(&ray_far_edge, data_view_matrix, &ray_far_ndc_edge);

            ray_delta = ray_far.xyz - ray_near.xyz;
            pixel_radius_near = ray_near_edge.xyz - ray_near.xyz;
            pixel_radius_far = ray_far_edge.xyz - ray_far.xyz;
        
            // The ray is treated as a cone of a certain diameter. In a perspective projection, this diameter typically increases along the direction of ray propagation. We calculate the diameter width incrementation per unit length by rejection of the pixel_radius vector onto the central ray_delta vector
            float3 a1_near = native_divide(dot(pixel_radius_near, ray_delta),dot(ray_delta,ray_delta))*ray_delta;
            float3 a2_near = pixel_radius_near - a1_near;
            
            float3 a1_far = native_divide(dot(pixel_radius_far, ray_delta),dot(ray_delta,ray_delta))*ray_delta;
            float3 a2_far = pixel_radius_far - a1_far;

            // The geometry of the cone
            cone_diameter_increment = 2.0*native_divide( length(a2_far - a2_near), length(ray_delta - a1_near + a1_far) );
            cone_diameter_near = 2.0*length(a2_near); // small approximation
        }

        // To limit resource spending we limit the ray to the intersection between itself and a bounding box 
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
            
            // Does the ray intersect?
            if (!((bbox[0] >= bbox[1]) || (bbox[2] >= bbox[3]) || (bbox[4] >= bbox[5])))
            {
                hit = bounding_box_intersect(ray_near.xyz, ray_delta.xyz, bbox, &t_near, &t_far);
            }
        }
        float4 sample = (float4)(0.0, 0.0, 0.0, 0.0);
        float4 color = (float4)(0.0, 0.0, 0.0, 0.0);
            
        // In the case that the ray actually hits the bounding box, prepare for volume sampling and color accumulation
        if(hit)
        {
            // The geometry of the intersecting part of the ray
            float3 ray_box_origin = ray_near.xyz + t_near * ray_delta.xyz;
            float3 ray_box_end = ray_near.xyz + t_far * ray_delta.xyz;
            float3 ray_box_delta = ray_box_end - ray_box_origin;

            float3 direction = normalize(ray_box_delta);

            // Some variables we will need during ray traversal
            float skip_length, intensity, voxel_size, voxel_size_up;
            float cone_diameter, f, step_length;
            float cone_diameter_low = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (numOctLevels-1))));
            float cone_diameter_high = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (0))));
            uint index, index_prev, brick, isMsd, isLowEnough, isEmpty;
            float2 tsf_position;
            float3 ray_box_xyz, ray_box_xyz_prev, ray_box_add;
            float3 norm_xyz, norm_xyz_prev;
            float3 tmp_a, tmp_b;
            float4 lookup_pos;
            uint4 brick_id;
            int3 norm_index;
            
            // The traversal coordinate. We keep track of the previous position as well
            ray_box_xyz = ray_box_origin;
            ray_box_xyz_prev = ray_box_origin;
            
            // A counter for the number of texture fetches from the brick pool
            int color_fetch_counter = 0;
            bool isColorAccumulated = 0;
            
            // Merged bits in the octtree can be read using these bitmasks:
            uint mask_msd_flag = ((1 << 1) - 1) << 31;
            uint mask_data_flag = ((1 << 1) - 1) << 30;
            uint mask_child_index = ((1 << 30) - 1) << 0;
            uint mask_brick_id_x = ((1 << 10) - 1) << 20;
            uint mask_brick_id_y = ((1 << 10) - 1) << 10;
            uint mask_brick_id_z = ((1 << 10) - 1) << 0;

            /* Traverse the octtree. For each step, descend into the octree until a) The resolution is appreciable or b) The final level of the octree is reached. During any descent, empty nodes might be found. In such case, the ray is advanced forward without sampling to the next sample that is not in said node. Stuff inside this while loop is what really takes time and therefore should be optimized
             * */
            while ( fast_length(ray_box_xyz - ray_box_origin) < fast_length(ray_box_delta) )
            {
                // Break off early if the accumulated alpha is high enough
                if (color.w > 0.995) break;

                // Index trackers for the traversal. 
                index = 0; 
                index_prev = 0;

                // Calculate the cone diameter at the current ray position
                cone_diameter = (cone_diameter_near + length(ray_box_xyz - ray_near) * cone_diameter_increment) * step_length_factor;
                cone_diameter = clamp(cone_diameter, cone_diameter_low, cone_diameter_high);

                // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octtree levels) 
                step_length = cone_diameter * 0.25; 
                ray_box_add = direction * step_length;

                 // We use a normalized convention during octtree traversal. The normalized convention makes it easier to think about the octtree traversal.  
                norm_xyz = native_divide( (float3)(ray_box_xyz.x - data_extent[0], ray_box_xyz.y - data_extent[2], ray_box_xyz.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;
                
                norm_index = convert_int3(norm_xyz);
                norm_index = clamp(norm_index, 0, 1);

                // Traverse the octtree
                for (int j = 0; j < numOctLevels; j++)
                {
                    voxel_size = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << j)));
                    if (j > 0) voxel_size_up = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (j-1))));

                    isMsd = (oct_index[index] & mask_msd_flag) >> 31;
                    isEmpty = !((oct_index[index] & mask_data_flag) >> 30);
                    isLowEnough = (cone_diameter > voxel_size);
                    
                    if (isEmpty)
                    {
                        // Skip forward by calculating how many steps can be advanced before reaching the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;

                        if (isDsActive)
                        {
                            sample = (float4)(1.0,1.0,1.0, 0.08);
                            color.xyz = color.xyz +(1 - color.w)*sample.xyz*sample.w;
                            color.w = color.w +(1 - color.w)*sample.w;
                        }
                        
                        // This ugly shit needs to go
                        tmp_a = norm_xyz - 5.0f*direction;
                        tmp_b = 15.0f*direction;
                        hit = bounding_box_intersect_oct(tmp_a, tmp_b, &t_near, &t_far);
                        
                        if (hit)
                        {
                            skip_length = 0.01 * cone_diameter_low + 0.5 * fast_length((tmp_a + t_far*tmp_b) - norm_xyz) * voxel_size * (brickSize-1);
                            ray_box_xyz += skip_length * direction;
                            break;
                        }
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
                            
                            lookup_pos = native_divide(0.5 + convert_float4(brick_id * brickSize)  + (float4)(norm_xyz_prev, 0.0f)*3.5 , convert_float4(bricks_dim));
                            
                            float intensity_prev = read_imagef(bricks, brick_sampler, lookup_pos).w;
                            
                            // The brick in the current level
                            brick = oct_brick[index];
                            brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);
                            
                            lookup_pos = native_divide(0.5 + convert_float4(brick_id * brickSize)  + (float4)(norm_xyz, 0.0f)*3.5 , convert_float4(bricks_dim));
                            
                            float intensity_here = read_imagef(bricks, brick_sampler, lookup_pos).w;
                            
                            // Linear interpolation between the two intensities
                            intensity = intensity_prev + (intensity_here - intensity_prev)*native_divide(cone_diameter - voxel_size_up, voxel_size - voxel_size_up);
                        }
                        else
                        {
                            /* Quadrilinear interpolation between two bricks. Shit!*/
                            
                            brick = oct_brick[index];
                            brick_id = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);
                            
                            lookup_pos = native_divide(0.5 + convert_float4(brick_id * brickSize)  + (float4)(norm_xyz, 0.0f)*3.5 , convert_float4(bricks_dim));
                            
                            intensity = read_imagef(bricks, brick_sampler, lookup_pos).w;
                        }
                        
                        if (isDsActive)
                        {
                            sample = (float4)(0.2,0.3,1.0, 1.00);
                            color.xyz = color.xyz +(1 - color.w)*sample.xyz*sample.w;
                            color.w = color.w +(1 - color.w)*sample.w;
                            break;
                        }
                        
                        
                        // Sample color
                        if(isLogActive)
                        {
                            intensity = log10(intensity); 
                        }
                        
                        tsf_position = (float2)(tsf_offset_low + (tsf_offset_high - tsf_offset_low) * ((intensity - data_limits.x)/(data_limits.y - data_limits.x)), 0.5f);
                        
                        sample = read_imagef(tsf_tex, tsf_sampler, tsf_position);
                        
                        sample.w *= alpha; 
                        
                        float steps = native_divide(cone_diameter, cone_diameter_low);
                        float rest_step = fmod(steps, 1.0);
                        int cycles = (int) steps;

                        // This shit needs to get analytic
                        for (int k = 0; k < cycles; k++)
                        {
                            color.xyz = color.xyz +(1 - color.w)*sample.xyz*sample.w;
                            color.w = color.w +(1 - color.w)*sample.w;
                        }
                        if (rest_step > 0)
                        {
                            color.xyz = color.xyz +(1 - color.w)*sample.xyz*sample.w;
                            color.w = color.w +(1 - color.w)*sample.w;
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

                // This shit shouldnt be needed
                if (fast_distance(ray_box_xyz, ray_box_xyz_prev) < step_length*0.5)
                {
                    ray_box_xyz += ray_box_add*0.5;
                }
                ray_box_xyz_prev = ray_box_xyz;
            }
            if (!isDsActive)color *= brightness;
        }
        write_imagef(ray_tex, id_glb, clamp(color, 0.0, 1.0));
    }
}



__kernel void modelRayTrace(
    __write_only image2d_t ray_tex,
    __read_only image2d_t tsf_tex, 
    sampler_t tsf_sampler,
    __constant float * data_view_matrix,
    __constant float * data_extent,
    __constant float * data_view_extent,
    __constant float * tsf_var,
    __constant float * parameters,
    __constant int * misc_int)
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    
    int2 ray_tex_dim = get_image_dim(ray_tex);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);

    float tsf_offset_low = tsf_var[0];
    float tsf_offset_high = tsf_var[1]; 
    float data_offset_low = tsf_var[2];
    float data_offset_high = tsf_var[3];
    float alpha = tsf_var[4];
    float brightness = tsf_var[5];
    
    int isLogActive = misc_int[2];
    int isPerspectiveActive = misc_int[5];
    
    float2 data_limits = (float2)(data_offset_low, data_offset_high);
    if (isLogActive)
    {
        if (data_limits.x <= 0) data_limits.x = 0.01;
        if (data_limits.y <= 0) data_limits.y = 0.01;
        data_limits.x = log10(data_limits.x);
        data_limits.y = log10(data_limits.y);
    }
    
    // If the global id corresponds to a texel
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        float4 ray_near, ray_far;
        float3 ray_delta;
        {
            // Normalized device coordinates (ndc) of the pixel and its edge (in screen coordinates)
            float2 ndc = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f)/convert_float2(ray_tex_dim)) - 1.0f);
            
            // Ray origin and exit point (screen coordinates)
            // z = 1 corresponds to far plane
            // z = -1 corresponds to near plane
            float4 ray_near_ndc = (float4)(ndc, -1.0f, 1.0f);
            float4 ray_far_ndc = (float4)(ndc, 1.0f, 1.0f);

            // Ray entry point at near and far plane 
            sc2xyz(&ray_near, data_view_matrix, &ray_near_ndc);
            sc2xyz(&ray_far, data_view_matrix, &ray_far_ndc);

            ray_delta = ray_far.xyz - ray_near.xyz;
        }
        
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

        if(hit)
        {
            // The geometry of the intersecting part of the ray
            float3 ray_box_origin = ray_near.xyz + t_near * ray_delta.xyz;
            float3 ray_box_end = ray_near.xyz + t_far * ray_delta.xyz;
            float3 ray_box_delta = ray_box_end - ray_box_origin;
            float3 ray_box_add = normalize(ray_box_delta)*native_divide(data_view_extent[1]-data_view_extent[0], 400.0);
            
            float ray_box_length = fast_length(ray_box_delta);
            int cycles = (int) (ray_box_length/fast_length(ray_box_add));

            float3 xyz;
            float val;
            
            for (int i = 0; i < cycles; i++)
            {
                xyz = ray_box_origin.xyz + i*ray_box_add.xyz;
                val = model(xyz, parameters);
                
                if(isLogActive)
                {
                    if (val < 1) val = 1;
                    val = log10(val); 
                }
                
                float2 tsf_position = (float2)(tsf_offset_low + (tsf_offset_high - tsf_offset_low) * ((val - data_limits.x)/(data_limits.y - data_limits.x)), 0.5f);
                
                sample = read_imagef(tsf_tex, tsf_sampler, tsf_position);
                sample.w *= alpha;

                color.xyz = color.xyz +(1 - color.w)*sample.xyz*sample.w;
                color.w = color.w +(1 - color.w)*sample.w;
                
                if (color.w > 0.999) break;
            }
            color *= brightness;
        }
        write_imagef(ray_tex, id_glb, clamp(color, 0.0, 1.0));
    }
}
