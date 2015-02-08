uniform sampler2D texture;
varying vec2 f_texpos;
//varying vec4 f_bounds;
//varying float f_pixel_size;
//varying vec2 f_center;

void main(void)
{
    vec2 flipped_texpos = vec2(f_texpos.x, 1.0 - f_texpos.y); // The OpenGL convention (origin at the bottom-left corner) is different than in 2D applications (origin at the top-left corner)
    gl_FragColor = texture2D(texture, flipped_texpos);
 
//    vec4 dark = vec4(0.0,0.0,0.0,1.0);
//    vec4 blue = vec4(0.0,0.0,1.0,1.0);
//    vec4 purple = vec4(1.0,0.0,1.0,1.0);
    
    // Width of inner border
//    float border_width = min(f_bounds.z - f_bounds.x, f_bounds.w - f_bounds.y) * 0.05;
    
    // If outside of bounds
//    if ((f_texpos.x < f_bounds.x) || (f_texpos.x > f_bounds.z) || (f_texpos.y < f_bounds.y) || (f_texpos.y > f_bounds.w))
//    {
//        float average = 0.21 * color.x + 0.72 * color.y + 0.07 * color.z;
        
//        color.xyz = vec3(average);
        
//        color = mix(color, dark, 1.0 - color.w*0.5);
//    }
    // Draw center
//    else if (((f_texpos.x < f_center.x + f_pixel_size*1.0) && (f_texpos.x > f_center.x - f_pixel_size*1.0)) || ((f_texpos.y < f_center.y + f_pixel_size*1.0) && (f_texpos.y > f_center.y - f_pixel_size*1.0)))
//    {
//        color = dark;
//    }

    // Else within bounds
//    if ((f_texpos.x > f_bounds.x - f_pixel_size) && (f_texpos.x < f_bounds.x))
//    {
//        color = 1.0 - color;
//        color = mix(color,blue,0.5);
//    }
//    else if ((f_texpos.x < f_bounds.z + f_pixel_size) && (f_texpos.x > f_bounds.z))
//    {
//        color = 1.0 - color;
//        color = mix(color,blue,0.5);
//    }
//    else if ((f_texpos.y > f_bounds.y - f_pixel_size) && (f_texpos.y < f_bounds.y))
//    {
//        color = 1.0 - color;
//        color = mix(color,blue,0.5);
//    }
//    else if ((f_texpos.y < f_bounds.w + f_pixel_size) && (f_texpos.y > f_bounds.w))
//    {
//        color = 1.0 - color;
//        color = mix(color,blue,0.5);
//    }
    

    
    // Draw border
//    if ((f_texpos.x < f_pixel_size) || (f_texpos.x > 1.0 - f_pixel_size) || (f_texpos.y < f_pixel_size) || (f_texpos.y > 1.0 - f_pixel_size))
//    {
//        color = 1.0 - color;
//        color = mix(color,dark,0.5);
//    }
    
//    gl_FragColor = color;
}
