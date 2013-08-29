#ifndef ATLAS_H
#define ATLAS_H

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
/* GL and CL */
#ifdef _WIN32
#define GLEW_STATIC
#endif
#include <GL/glew.h>
/* Freetype */
#include <ft2build.h>
#include FT_FREETYPE_H

/**
 * The atlas class holds a texture that contains the visible US-ASCII characters
 * of a certain font rendered with a certain character height.
 * It also contains an array that contains all the information necessary to
 * generate the appropriate vertex and texture coordinates for each character.
 *
 * After the constructor is run, you don't need to use any FreeType functions anymore.
 */
 
struct glyphinfo {
    float ax; // advance x (in pixels)
    float ay; // advance y (in pixels)

    float bw; // width (in pixels)
    float bh; // height (in pixels)

    float bl; // left (in pixels)
    float bt; // top (in pixels)

    float tx; // x offset of glyph in texture coordinates
    float ty; // y offset of glyph in texture coordinates
};
 
class Atlas {
    private:
        
        
    public:
        GLuint tex;    // texture object

        int tex_w; // width of texture in pixels
        int tex_h; // height of texture in pixels
    
        Atlas(FT_Face face, int height);
        ~Atlas();
        glyphinfo c[128]; // character information
};

#endif
