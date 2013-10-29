int boundingBoxIntersect(float3 r_origin, float3 r_delta, float * bbox, float * t_near, float * t_far)
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

int boundingBoxIntersect2(float3 r_origin, float3 r_delta, __constant float * bbox, float * t_near, float * t_far)
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

int boundingBoxIntersectNorm(float3 r_origin, float3 r_delta, float * t_near, float * t_far)
{
    // This is simple ray-box intersection: http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm

    // Compute relative intersects
    float3 r_delta_inv = native_divide((float3)(1.0f),r_delta);
    float3 T1 = ((float3)(0.0f) - r_origin)*r_delta_inv;
    float3 T2 = ((float3)(2.0f) - r_origin)*r_delta_inv;

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

float4 sc2xyz( __constant float * A, float4 x)
{
    // This is an adapted matrix multiplication function

    float4 b;
    b.w = native_divide(1.0f, x.x*A[12] + x.y*A[13] + x.z*A[14] + x.w*A[15]);
    b.x = b.w * (x.x*A[0] + x.y*A[1] + x.z*A[2] + x.w*A[3]);
    b.y = b.w * (x.x*A[4] + x.y*A[5] + x.z*A[6] + x.w*A[7]);
    b.z = b.w * (x.x*A[8] + x.y*A[9] + x.z*A[10] + x.w*A[11]);

    return b;
}
