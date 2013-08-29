uniform sampler2D top_texture;
uniform sampler2D bot_texture;
in vec2 f_texpos;
out vec4 fcolor;

void main ()
{
	vec4 top = texture2D(top_texture, f_texpos);
	vec4 bot = texture2D(bot_texture, f_texpos);
    
    int blending = 2;
    
    if ( blending == 0 )
	{
		// Additive blending (strong result, high overexposure)
		fcolor = min(top + bot, 1.0);
	}
	else if ( blending == 1 )
	{
		// Screen blending (mild result, medium overexposure)
		fcolor = clamp((top + bot) - (top * bot), 0.0, 1.0);
		fcolor.w = 1.0;
	}
	else
	{
		fcolor = bot;
	}
}
