__kernel void psum(
    __global float * data,
    __local float * addition_array,
    unsigned int read_size,
    unsigned int read_offset,
    unsigned int write_offset
    )
{
    // Parallel reduction. Takes N values from an input array and sums them and puts the final value in an output-array. N must be a power of two. This kernel can be invoked in a loop until the entire array has been reduced.

    int N = get_local_size(0);

    if (get_global_id(0) < read_size)
    {
        addition_array[get_local_id(0)] = data[get_global_id(0) + read_offset];
    }
    else addition_array[get_local_id(0)] = 0.0;

    barrier(CLK_LOCAL_MEM_FENCE);
    for (unsigned int i = N/2; i > 0; i >>= 1)
    {
        if (get_local_id(0) < i)
        {
            addition_array[get_local_id(0)] += addition_array[i + get_local_id(0)];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (get_local_id(0) == 0)
    {
        data[write_offset + get_group_id(0)] = addition_array[0];
    }
}
