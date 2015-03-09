__kernel void rectCopyFloat(
    __global float * buffer,
    int2 buffer_size,
    int2 buffer_origin,
    int buffer_row_pitch,
    __global float * copy,
    int2 copy_size,
    int2 copy_origin,
    int copy_row_pitch,
    int2 region)
{
    // Use to copy a rectangular region from a buffer into another buffer.

    if ((get_global_id(0) + buffer_origin.x < buffer_size.x) && (get_global_id(1) + buffer_origin.y < buffer_size.y) && (get_global_id(0) + copy_origin.x < copy_size.x) && (get_global_id(1) + copy_origin.y < copy_size.y))
    {
        int2 buffer_id = (int2)(buffer_origin.x + get_global_id(0), buffer_origin.y + get_global_id(1));

        int2 copy_id = (int2)(copy_origin.x + get_global_id(0), copy_origin.y + get_global_id(1));

        copy[copy_id.y * copy_row_pitch + copy_id.x] = buffer[buffer_id.y * buffer_row_pitch + buffer_id.x];
    }
}



