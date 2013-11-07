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

    return exp(-2.0f*(k.x*k.x + k.y*k.y + k.z*k.z)*param[3])*native_divide(1.0f,(a*b*c - a*e*e - b*f*f - c*d*d + 2.0f*d*e*f))*dot(k, Ak);
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
    __constant int * misc_int,
    __constant float * scalebar_rotation)
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));

    int2 ray_tex_dim = get_image_dim(ray_tex);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);

    float tsfOffsetLow = tsf_var[0];
    float tsfOffsetHigh = tsf_var[1];
    float dataOffsetLow = tsf_var[2];
    float dataOffsetHigh = tsf_var[3];
    float alpha = tsf_var[4];
    float brightness = tsf_var[5];

    int isLogActive = misc_int[2];
    int isSlicingActive = misc_int[4];

    if (isLogActive)
    {
        if (dataOffsetLow <= 0) dataOffsetLow = 0.01f;
        if (dataOffsetHigh <= 0) dataOffsetHigh = 0.01f;
        dataOffsetLow = log10(dataOffsetLow);
        dataOffsetHigh = log10(dataOffsetHigh);
    }

    // If the global id corresponds to a texel
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        float4 ray_near, rayFar;
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
            ray_near = sc2xyz(data_view_matrix, rayNearNdc);
            rayFar = sc2xyz(data_view_matrix, rayFarNdc);
            rayNearEdge = sc2xyz(data_view_matrix, rayNearNdcEdge);
            rayFarEdge = sc2xyz(data_view_matrix, rayFarNdcEdge);

            rayDelta = rayFar.xyz - ray_near.xyz;
            pixelRadiusNear = rayNearEdge.xyz - ray_near.xyz;
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
                hit = boundingBoxIntersect(ray_near.xyz, rayDelta.xyz, bbox, &t_near, &t_far);
            }
        }

        float4 color = (float4)(0.0f);
        float4 sample = (float4)(0.0f);

        float4 max_sample = read_imagef(tsf_tex, tsf_sampler, (float2)(1.0f, 0.5f));
        float4 min_sample = read_imagef(tsf_tex, tsf_sampler, (float2)(0.0f, 0.5f));

        if(hit)
        {
            // The geometry of the intersecting part of the ray
            float cone_diameter, step_length;
            float cone_diameter_low = 0.05; // Only acts as a scaling factor

            float3 rayBoxOrigin = ray_near.xyz + t_near * rayDelta.xyz;
            float3 rayBoxEnd = ray_near.xyz + t_far * rayDelta.xyz;
            float3 rayBoxDelta = rayBoxEnd - rayBoxOrigin;
            float3 direction = normalize(rayBoxDelta);
            float3 ray_add_box;
            float rayBoxLength = fast_length(rayBoxDelta);

            float3 ray_xyz_box = rayBoxOrigin;
            float val;

            if (isSlicingActive)
            {
                // Ray-plane intersection
                float4 center = (float4)(
                    data_view_extent[0] + 0.5*(data_view_extent[1] - data_view_extent[0]),
                    data_view_extent[2] + 0.5*(data_view_extent[3] - data_view_extent[2]),
                    data_view_extent[4] + 0.5*(data_view_extent[5] - data_view_extent[4]),
                    0);

                // Plane normals
                float4 normal[3];
                normal[0] = (float4)(1.0f, 0.0f, 0.0f, 0.0f);
                normal[1] = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
                normal[2] = (float4)(0.0f, 0.0f, 1.0f, 0.0f);

                float d[3];

                for (int i = 0; i < 3; i++)
                {
                    // Rotate plane normals in accordance with the relative scalebar
                    normal[i] = matrixMultiply4x4X1x4(scalebar_rotation, normal[i]);

                    // Compute number of meaningful intersections (i.e. not parallel to plane, intersection, and within bounding box)
                    float nominator = dot(center - (float4)(rayBoxOrigin, 0.0f), normal[i]);
                    float denominator = dot((float4)(rayBoxDelta, 0.0f), normal[i]);

                    if (denominator != 0.0f)  d[i] = nominator / denominator;
                    else d[i] = -1.0f;
                }


                // Sort intersections along ray
                float d_sorted[3];
                selectionSort(d, 3);

                // Accumulate color
                for (int i = 0; i < 3; i++)
                {
                    if ((d[i] >= 0.0f) && (d[i] <= 1.0f))
                    {
                        ray_xyz_box = rayBoxOrigin + d[i] * rayBoxDelta;

                        val = model(ray_xyz_box, parameters);

                        if(isLogActive)
                        {
                            if (val < 1.f) val = 1.f;
                            val = log10(val);
                        }

                        float2 tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((val - dataOffsetLow)/(dataOffsetHigh - dataOffsetLow)), 0.5f);
                        sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);

                        color.xyz += (1.0f - color.w)*sample.xyz*sample.w;
                        color.w += (1.0f - color.w)*sample.w;
                    }
                }
            }
            else
            {
                // Ray-volume intersection
                while ( fast_length(ray_xyz_box - rayBoxOrigin) < rayBoxLength )
                {
                    // Calculate the cone diameter at the current ray position
                    cone_diameter = (coneDiameterNear + length(ray_xyz_box - ray_near.xyz) * coneDiameterIncrement);

                    // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octtree levels)
                    step_length = cone_diameter;
                    ray_add_box = direction * step_length;

                    val = model(ray_xyz_box, parameters);

                    if(isLogActive)
                    {
                        if (val < 1.f) val = 1.f;
                        val = log10(val);
                    }

                    if ((val >= dataOffsetLow) && (val <= dataOffsetHigh))
                    {
                        float2 tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((val - dataOffsetLow)/(dataOffsetHigh - dataOffsetLow)), 0.5f);

                        sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);
                        sample.w *= alpha*native_divide(cone_diameter, cone_diameter_low);;

                        color.xyz += (1.0f - color.w)*sample.xyz*sample.w;
                        color.w += (1.0f - color.w)*sample.w;
                    }
                    else if (val > dataOffsetHigh)
                    {
                        sample = max_sample;
                        sample.w *= alpha*native_divide(cone_diameter, cone_diameter_low);;

                        color.xyz += (1.0f - color.w)*sample.xyz*sample.w;
                        color.w += (1.0f - color.w)*sample.w;
                    }

                    ray_xyz_box += ray_add_box;
                    if (color.w > 0.999f) break;

                }
                color *= brightness;
            }
        }
        write_imagef(ray_tex, id_glb, clamp(color, 0.0f, 1.0f));
    }
}


__kernel void modelWorkload(
    int2 ray_tex_dim,
    __global float * glb_work,
    __local float * loc_work,
    __constant float * data_view_matrix,
    __constant float * data_extent,
    __constant float * data_view_extent,
    __constant float * scalebar_matrix)
{
/* Estimate the number of intensity and color fetches that are needed for
 * the given combination input parameters */
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));
    int2 id_loc = (int2)(get_local_id(0),get_local_id(1));
    int2 size_loc = (int2)(get_local_size(0),get_local_size(1));

    int id = id_loc.x + id_loc.y * size_loc.x;
    loc_work[id] = 0.0f;

    // If the global id corresponds to a texel
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        float4 ray_near, rayFar;
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
            ray_near = sc2xyz(data_view_matrix, rayNearNdc);
            rayFar = sc2xyz(data_view_matrix, rayFarNdc);
            rayNearEdge = sc2xyz(data_view_matrix, rayNearNdcEdge);
            rayFarEdge = sc2xyz(data_view_matrix, rayFarNdcEdge);

            rayDelta = rayFar.xyz - ray_near.xyz;
            pixelRadiusNear = rayNearEdge.xyz - ray_near.xyz;
            pixelRadiusFar = rayFarEdge.xyz - rayFar.xyz;

            // The ray is treated as a cone of a certain diameter.
            float3 a1Near = native_divide(dot(pixelRadiusNear, rayDelta),dot(rayDelta,rayDelta))*rayDelta;
            float3 a2Near = pixelRadiusNear - a1Near;

            float3 a1Far = native_divide(dot(pixelRadiusFar, rayDelta),dot(rayDelta,rayDelta))*rayDelta;
            float3 a2Far = pixelRadiusFar - a1Far;

            // The geometry of the cone
            coneDiameterIncrement = 2.0f*native_divide( length(a2Far - a2Near), length(rayDelta - a1Near + a1Far) );
            coneDiameterNear = 2.0f*length(a2Near); // small approximation
        }

        int hit;
        float t_near, t_far;
        {
            // Does the ray for this pixel intersect bbox?
            if (!((data_view_extent[0] >= data_view_extent[1]) || (data_view_extent[2] >= data_view_extent[3]) || (data_view_extent[4] >= data_view_extent[5])))
            {
                hit = boundingBoxIntersect2(ray_near.xyz, rayDelta.xyz, data_view_extent, &t_near, &t_far);
            }
        }

        if(hit)
        {
            // The geometry of the intersecting part of the ray
            float3 rayBoxOrigin = ray_near.xyz + t_near * rayDelta.xyz;
            float3 rayBoxEnd = ray_near.xyz + t_far * rayDelta.xyz;
            float3 rayBoxDelta = rayBoxEnd - rayBoxOrigin;
            float rayBoxLength = fast_length(rayBoxDelta);

            float coneDiameterOrigin = (coneDiameterNear + length(rayBoxOrigin - ray_near.xyz) * coneDiameterIncrement);
            float coneDiameterEnd = (coneDiameterNear + length(rayBoxEnd - ray_near.xyz) * coneDiameterIncrement);

            float coneDiameterAvg = (coneDiameterOrigin + coneDiameterEnd) * 0.5;
            loc_work[id] = native_divide(rayBoxLength, coneDiameterAvg);
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
