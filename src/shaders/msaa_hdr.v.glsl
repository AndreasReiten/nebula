in vec2 position;
in vec2 texpos;
smooth out vec2 vertOutTexCoords;

void main(void)
{
    gl_Position =  vec4(position, 0.0, 1.0);
    vertOutTexCoords = texpos;
}

