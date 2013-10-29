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

    float tsfOffsetLow = tsf_var[0];
    float tsfOffsetHigh = tsf_var[1];
    float dataOffsetLow = tsf_var[2];
    float dataOffsetHigh = tsf_var[3];
    float alpha = tsf_var[4];
    float brightness = tsf_var[5];

    if (isLogActive)
    {
        if (dataOffsetLow <= 0.0f) dataOffsetLow = 0.01f;
        if (dataOffsetHigh <= 0.0f) dataOffsetHigh = 0.01f;
        dataOffsetLow = log10(dataOffsetLow);
        dataOffsetHigh = log10(dataOffsetHigh);
    }

    // If the global id corresponds to a texel, then check if its associated ray hits our cubic bounding box. If it does - traverse along the intersecing ray segment and accumulate color
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        /*
         * Find the geometry of the ray
         * */
        float4 rayNear, rayFar;
        float3 rayDelta;
        float coneDiameterIncrement;
        float coneDiameterNear;
        {
            float4 rayNearEdge, rayFarEdge;
            float3 pixelRadiusNear, pixelRadiusFar;

            // Normalized device coordinates (ndc) of the pixel and its edge (in screen coordinates)
            float2 ndc = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f)/convert_float2(ray_tex_dim)) - 1.0f);
            float2 ndcEdge = (float2)(2.0f * (( convert_float2(id_glb) + (float2)(1.0f, 1.0f))/convert_float2(ray_tex_dim)) - 1.0f);

            // Ray origin and exit point (screen coordinates)
            // z = 1 corresponds to far plane
            // z = -1 corresponds to near plane
            float4 rayNearNdc = (float4)(ndc, -1.0f, 1.0f);
            float4 rayFarNdc = (float4)(ndc, 1.0f, 1.0f);

            float4 rayNearNdcEdge = (float4)(ndcEdge, -1.0f, 1.0f);
            float4 rayFarNdcEdge = (float4)(ndcEdge, 1.0f, 1.0f);

            // Ray entry point at near and far plane
            rayNear = sc2xyz(data_view_matrix, rayNearNdc);
            rayFar = sc2xyz(data_view_matrix, rayFarNdc);
            rayNearEdge = sc2xyz(data_view_matrix, rayNearNdcEdge);
            rayFarEdge = sc2xyz(data_view_matrix, rayFarNdcEdge);

            rayDelta = rayFar.xyz - rayNear.xyz;
            pixelRadiusNear = rayNearEdge.xyz - rayNear.xyz;
            pixelRadiusFar = rayFarEdge.xyz - rayFar.xyz;

            // The ray is treated as a cone of a certain diameter. In a perspective projection, this diameter typically increases along the direction of ray propagation. We calculate the diameter width incrementation per unit length by rejection of the pixel_radius vector onto the central rayDelta vector
            float3 a1Near = native_divide(dot(pixelRadiusNear, rayDelta),dot(rayDelta,rayDelta))*rayDelta;
            float3 a2Near = pixelRadiusNear - a1Near;

            float3 a1Far = native_divide(dot(pixelRadiusFar, rayDelta),dot(rayDelta,rayDelta))*rayDelta;
            float3 a2Far = pixelRadiusFar - a1Far;

            // The geometry of the cone
            coneDiameterIncrement = 2.0f*native_divide( length(a2Far - a2Near), length(rayDelta - a1Near + a1Far) );
            coneDiameterNear = 2.0f*length(a2Near); // small approximation
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
                hit = boundingBoxIntersect(rayNear.xyz, rayDelta.xyz, bbox, &t_near, &t_far);
            }
        }
        float4 sample = (float4)(0.0f);
        float4 color = (float4)(0.0f);

        // In the case that the ray actually hits the bounding box, prepare for volume sampling and color accumulation
        if(hit)
        {
            // The geometry of the intersecting part of the ray
            float3 rayBoxOrigin = rayNear.xyz + t_near * rayDelta.xyz;
            float3 rayBoxEnd = rayNear.xyz + t_far * rayDelta.xyz;
            float3 rayBoxDelta = rayBoxEnd - rayBoxOrigin;

            float3 direction = normalize(rayBoxDelta);

            // Some variables we will need during ray traversal
            float skipLength, intensity, voxelSize, voxelSizeUp;
            float coneDiameter, stepLength;
            float coneDiameterLow = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (numOctLevels-1))));
            float cone_diameter_high = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (0))));
            uint index, index_prev, brick, isMsd, isLowEnough, isEmpty;
            float2 tsfPosition;
            float3 rayBoxXyz, rayBoxXyzPrev, rayBoxAdd;
            float3 normXyz, normXyzPrev;
            float3 tmp_a, tmp_b;
            float4 lookupPos;
            uint4 brickId;
            int3 normIndex;

            // The traversal coordinate. We keep track of the previous position as well
            rayBoxXyz = rayBoxOrigin;
            rayBoxXyzPrev = rayBoxOrigin;

            // Merged bits in the octtree can be read using these bitmasks:
            uint mask_msd_flag = ((1u << 1u) - 1u) << 31u;
            uint mask_data_flag = ((1 << 1) - 1) << 30;
            uint mask_child_index = ((1 << 30) - 1) << 0;
            uint mask_brick_id_x = ((1 << 10) - 1) << 20;
            uint mask_brick_id_y = ((1 << 10) - 1) << 10;
            uint mask_brick_id_z = ((1 << 10) - 1) << 0;

            /* Traverse the octtree. For each step, descend into the octree until a) The resolution is appreciable or b) The final level of the octree is reached. During any descent, empty nodes might be found. In such case, the ray is advanced forward without sampling to the next sample that is not in said node. Stuff inside this while loop is what really takes time and therefore should be optimized
             * */
            while ( fast_length(rayBoxXyz - rayBoxOrigin) < fast_length(rayBoxDelta) )
            {
                // Break off early if the accumulated alpha is high enough
                if (color.w > 0.995f) break;

                // Index trackers for the traversal.
                index = 0;
                index_prev = 0;

                // Calculate the cone diameter at the current ray position
                coneDiameter = (coneDiameterNear + length(rayBoxXyz - rayNear.xyz) * coneDiameterIncrement);
                coneDiameter = clamp(coneDiameter, coneDiameterLow, cone_diameter_high);

                // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octtree levels)
                stepLength = coneDiameter * 0.25f;
                rayBoxAdd = direction * stepLength;

                 // We use a normalized convention during octtree traversal. The normalized convention makes it easier to think about the octtree traversal.
                normXyz = native_divide( (float3)(rayBoxXyz.x - data_extent[0], rayBoxXyz.y - data_extent[2], rayBoxXyz.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;

                normIndex = convert_int3(normXyz);
                normIndex = clamp(normIndex, 0, 1);

                // Traverse the octtree
                for (int j = 0; j < numOctLevels; j++)
                {
                    voxelSize = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << j)));
                    if (j > 0) voxelSizeUp = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (j-1))));

                    isMsd = (oct_index[index] & mask_msd_flag) >> 31;
                    isEmpty = !((oct_index[index] & mask_data_flag) >> 30);
                    isLowEnough = (coneDiameter > voxelSize);

                    if (isEmpty)
                    {
                        // Skip forward by calculating how many steps can be advanced before reaching the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;

                        if (isDsActive)
                        {
                            sample = (float4)(1.0f,1.0f,1.0f, 0.08f);
                            color.xyz = color.xyz +(1.0f - color.w)*sample.xyz*sample.w;
                            color.w = color.w +(1.0f - color.w)*sample.w;
                        }

                        // This ugly shit needs to go
                        tmp_a = normXyz - 5.0f*direction;
                        tmp_b = 15.0f*direction;
                        hit = boundingBoxIntersectNorm(tmp_a, tmp_b, &t_near, &t_far);

                        if (hit)
                        {
                            skipLength = 0.01f * coneDiameterLow + 0.5f * fast_length((tmp_a + t_far*tmp_b) - normXyz) * voxelSize * (brickSize-1);
                            rayBoxXyz += skipLength * direction;
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
                            brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                            lookupPos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(normXyzPrev, 0.0f)*3.5f , convert_float4(bricks_dim));

                            float intensityPrev = read_imagef(bricks, brick_sampler, lookupPos).w;

                            // The brick in the current level
                            brick = oct_brick[index];
                            brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                            lookupPos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(normXyz, 0.0f)*3.5f , convert_float4(bricks_dim));

                            float intensityHere = read_imagef(bricks, brick_sampler, lookupPos).w;

                            // Linear interpolation between the two intensities
                            intensity = intensityPrev + (intensityHere - intensityPrev)*native_divide(coneDiameter - voxelSizeUp, voxelSize - voxelSizeUp);
                        }
                        else
                        {
                            /* Quadrilinear interpolation between two bricks. Shit!*/

                            brick = oct_brick[index];
                            brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                            lookupPos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(normXyz, 0.0f)*3.5f , convert_float4(bricks_dim));

                            intensity = read_imagef(bricks, brick_sampler, lookupPos).w;
                        }

                        if (isDsActive)
                        {
                            sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                            color.xyz = color.xyz +(1.f - color.w)*sample.xyz*sample.w;
                            color.w = color.w +(1.f - color.w)*sample.w;
                            break;
                        }


                        // Sample color
                        if(isLogActive)
                        {
                            intensity = log10(intensity);
                        }

                        tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((intensity - dataOffsetLow)/(dataOffsetHigh - dataOffsetLow)), 0.5f);

                        sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);

                        sample.w *= alpha*native_divide(coneDiameter, coneDiameterLow);

                        color.xyz += (1.f - color.w)*sample.xyz*sample.w;
                        color.w += (1.f - color.w)*sample.w;

                        rayBoxXyz += rayBoxAdd;
                        break;
                    }
                    else
                    {
                        // Save values from this level to enable quadrilinear interpolation between levels
                        index_prev = index;
                        normXyzPrev = normXyz;

                        // Descend to the next level
                        index = (oct_index[index] & mask_child_index);
                        index += normIndex.x + normIndex.y*2 + normIndex.z*4;

                        //normXyz = (normXyz - convert_float(normIndex))*2.0f;
            normXyz = (normXyz - (float3)((float)normIndex.x, (float)normIndex.y, (float)normIndex.z))*2.0f;
                        normIndex = convert_int3(normXyz);
                        normIndex = clamp(normIndex, 0, 1);
                    }
                }

                // This shit shouldnt be needed
                if (fast_distance(rayBoxXyz, rayBoxXyzPrev) < stepLength*0.5f)
                {
                    rayBoxXyz += rayBoxAdd*0.5f;
                }
                rayBoxXyzPrev = rayBoxXyz;
            }
            if (!isDsActive)color *= brightness;
        }
        write_imagef(ray_tex, id_glb, clamp(color, 0.0f, 1.0f));
    }
}

__kernel void svoWorkload(
    int2 ray_tex_dim,
    __global float * glb_work,
    __local float * loc_work,
    __global uint * oct_index,
    __global uint * oct_brick,
    __constant float * data_view_matrix,
    __constant float * data_extent,
    __constant float * data_view_extent,
    __constant int * misc_int)
{
/* Estimate the number of intensity and color fetches that are needed for
 * the given combination input parameters */
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    int2 id_loc = (int2)(get_local_id(0),get_local_id(1));
    int2 size_loc = (int2)(get_local_size(0),get_local_size(1));

    int id = id_loc.x + id_loc.y * size_loc.x;
    loc_work[id] = 0.0f;


    int numOctLevels = misc_int[0];
    int brickSize = misc_int[1];
    int isLogActive = misc_int[2];
    int isDsActive = misc_int[3];


    // If the global id corresponds to a texel, then check if its associated ray hits our cubic bounding box. If it does - traverse along the intersecing ray segment and accumulate color
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        /*
         * Find the geometry of the ray
         * */
        float4 rayNear, rayFar;
        float3 rayDelta;
        float coneDiameterIncrement;
        float coneDiameterNear;
        {
            float4 rayNearEdge, rayFarEdge;
            float3 pixelRadiusNear, pixelRadiusFar;

            // Normalized device coordinates (ndc) of the pixel and its edge (in screen coordinates)
            float2 ndc = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f)/convert_float2(ray_tex_dim)) - 1.0f);
            float2 ndcEdge = (float2)(2.0f * (( convert_float2(id_glb) + (float2)(1.0f, 1.0f))/convert_float2(ray_tex_dim)) - 1.0f);

            // Ray origin and exit point (screen coordinates)
            float4 rayNearNdc = (float4)(ndc, -1.0f, 1.0f);
            float4 rayFarNdc = (float4)(ndc, 1.0f, 1.0f);

            float4 rayNearNdcEdge = (float4)(ndcEdge, -1.0f, 1.0f);
            float4 rayFarNdcEdge = (float4)(ndcEdge, 1.0f, 1.0f);

            // Ray entry point at near and far plane
            rayNear = sc2xyz(data_view_matrix, rayNearNdc);
            rayFar = sc2xyz(data_view_matrix, rayFarNdc);
            rayNearEdge = sc2xyz(data_view_matrix, rayNearNdcEdge);
            rayFarEdge = sc2xyz(data_view_matrix, rayFarNdcEdge);

            rayDelta = rayFar.xyz - rayNear.xyz;
            pixelRadiusNear = rayNearEdge.xyz - rayNear.xyz;
            pixelRadiusFar = rayFarEdge.xyz - rayFar.xyz;

            // The ray is treated as a cone of a certain diameter. In a perspective projection, this diameter typically increases along the direction of ray propagation. We calculate the diameter width incrementation per unit length by rejection of the pixel_radius vector onto the central rayDelta vector
            float3 a1Near = native_divide(dot(pixelRadiusNear, rayDelta),dot(rayDelta,rayDelta))*rayDelta;
            float3 a2Near = pixelRadiusNear - a1Near;

            float3 a1Far = native_divide(dot(pixelRadiusFar, rayDelta),dot(rayDelta,rayDelta))*rayDelta;
            float3 a2Far = pixelRadiusFar - a1Far;

            // The geometry of the cone
            coneDiameterIncrement = 2.0f*native_divide( length(a2Far - a2Near), length(rayDelta - a1Near + a1Far) );
            coneDiameterNear = 2.0f*length(a2Near); // small approximation
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
                hit = boundingBoxIntersect(rayNear.xyz, rayDelta.xyz, bbox, &t_near, &t_far);
            }
        }
        float4 sample = (float4)(0.0f);
        float4 color = (float4)(0.0f);

        // In the case that the ray actually hits the bounding box, prepare for volume sampling and color accumulation
        if(hit)
        {
            // The geometry of the intersecting part of the ray
            float3 rayBoxOrigin = rayNear.xyz + t_near * rayDelta.xyz;
            float3 rayBoxEnd = rayNear.xyz + t_far * rayDelta.xyz;
            float3 rayBoxDelta = rayBoxEnd - rayBoxOrigin;

            float3 direction = normalize(rayBoxDelta);

            // Some variables we will need during ray traversal
            float skipLength, voxelSize, voxelSizeUp;
            float coneDiameter, stepLength;
            float coneDiameterLow = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (numOctLevels-1))));
            float cone_diameter_high = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (0))));
            uint index, index_prev, brick, isMsd, isLowEnough, isEmpty;
            float3 rayBoxXyz, rayBoxXyzPrev, rayBoxAdd;
            float3 normXyz, normXyzPrev;
            float3 tmp_a, tmp_b;
            float4 lookupPos;
            uint4 brickId;
            int3 normIndex;

            // The traversal coordinate. We keep track of the previous position as well
            rayBoxXyz = rayBoxOrigin;
            rayBoxXyzPrev = rayBoxOrigin;

            // Merged bits in the octtree can be read using these bitmasks:
            uint mask_msd_flag = ((1u << 1u) - 1u) << 31u;
            uint mask_data_flag = ((1 << 1) - 1) << 30;
            uint mask_child_index = ((1 << 30) - 1) << 0;
            uint mask_brick_id_x = ((1 << 10) - 1) << 20;
            uint mask_brick_id_y = ((1 << 10) - 1) << 10;
            uint mask_brick_id_z = ((1 << 10) - 1) << 0;

            /* Traverse the octtree. For each step, descend into the octree until a) The resolution is appreciable or b) The final level of the octree is reached. During any descent, empty nodes might be found. In such case, the ray is advanced forward without sampling to the next sample that is not in said node. Stuff inside this while loop is what really takes time and therefore should be optimized
             * */
            while ( fast_length(rayBoxXyz - rayBoxOrigin) < fast_length(rayBoxDelta) )
            {
                // Break off early if the accumulated alpha is high enough
                if (color.w > 0.995f) break;

                // Index trackers for the traversal.
                index = 0;
                index_prev = 0;

                // Calculate the cone diameter at the current ray position
                coneDiameter = (coneDiameterNear + length(rayBoxXyz - rayNear.xyz) * coneDiameterIncrement);
                coneDiameter = clamp(coneDiameter, coneDiameterLow, cone_diameter_high);

                // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octtree levels)
                stepLength = coneDiameter * 0.25f;
                rayBoxAdd = direction * stepLength;

                 // We use a normalized convention during octtree traversal. The normalized convention makes it easier to think about the octtree traversal.
                normXyz = native_divide( (float3)(rayBoxXyz.x - data_extent[0], rayBoxXyz.y - data_extent[2], rayBoxXyz.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;

                normIndex = convert_int3(normXyz);
                normIndex = clamp(normIndex, 0, 1);

                // Traverse the octtree
                for (int j = 0; j < numOctLevels; j++)
                {
                    voxelSize = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << j)));
                    if (j > 0) voxelSizeUp = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (j-1))));

                    isMsd = (oct_index[index] & mask_msd_flag) >> 31;
                    isEmpty = !((oct_index[index] & mask_data_flag) >> 30);
                    isLowEnough = (coneDiameter > voxelSize);

                    if (isEmpty)
                    {
                        // Skip forward by calculating how many steps can be advanced before reaching the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;

                        if (isDsActive)
                        {
                            sample = (float4)(1.0f,1.0f,1.0f, 0.08f);
                            color.xyz = color.xyz +(1.0f - color.w)*sample.xyz*sample.w;
                            color.w = color.w +(1.0f - color.w)*sample.w;
                        }

                        // This ugly shit needs to go
                        tmp_a = normXyz - 5.0f*direction;
                        tmp_b = 15.0f*direction;
                        hit = boundingBoxIntersectNorm(tmp_a, tmp_b, &t_near, &t_far);

                        if (hit)
                        {
                            skipLength = 0.01f * coneDiameterLow + 0.5f * fast_length((tmp_a + t_far*tmp_b) - normXyz) * voxelSize * (brickSize-1);
                            rayBoxXyz += skipLength * direction;
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
                            brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);



                            // The brick in the current level
                            brick = oct_brick[index];
                            brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);
                        }
                        else
                        {
                            /* Quadrilinear interpolation between two bricks. Shit!*/

                            brick = oct_brick[index];
                            brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);
                        }

                        if (isDsActive)
                        {
                            sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                            color.xyz = color.xyz +(1.f - color.w)*sample.xyz*sample.w;
                            color.w = color.w +(1.f - color.w)*sample.w;
                            loc_work[id] += 1.0f;
                            break;
                        }

                        loc_work[id] += 1.0f;
                        rayBoxXyz += rayBoxAdd;
                        break;
                    }
                    else
                    {
                        // Save values from this level to enable quadrilinear interpolation between levels
                        index_prev = index;
                        normXyzPrev = normXyz;

                        // Descend to the next level
                        index = (oct_index[index] & mask_child_index);
                        index += normIndex.x + normIndex.y*2 + normIndex.z*4;

                        //normXyz = (normXyz - convert_float(normIndex))*2.0f;
            normXyz = (normXyz - (float3)((float)normIndex.x, (float)normIndex.y, (float)normIndex.z))*2.0f;
                        normIndex = convert_int3(normXyz);
                        normIndex = clamp(normIndex, 0, 1);
                    }
                }

                // This shit shouldnt be needed
                if (fast_distance(rayBoxXyz, rayBoxXyzPrev) < stepLength*0.5f)
                {
                    rayBoxXyz += rayBoxAdd*0.5f;
                }
                rayBoxXyzPrev = rayBoxXyz;
            }
        }
    }

    // Parallel reduction to sum up the work
    barrier(CLK_LOCAL_MEM_FENCE);
    for (unsigned int i = (size_loc.x * size_loc.y)/2; i > 0; i >>= 1)
    {
        if (id < i)
        {
            loc_work[id] += loc_work[i + id];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (id == 0)
    {
        glb_work[get_group_id(0) +  get_group_id(1) * get_num_groups(0)] = loc_work[0];
    }
}