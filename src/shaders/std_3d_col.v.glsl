attribute  vec3 fragpos;
uniform  mat4 transform;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 1.0);
}

