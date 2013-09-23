#include "atlas.h"

Atlas::Atlas(FT_Face face, int height)
{
    verbosity = 1;

    FT_Set_Pixel_Sizes(face, 0, height);
    FT_GlyphSlot glyph = face->glyph;

    int roww = 0;
    int rowh = 0;
    tex_w = 0;
    tex_h = 0;

    memset(c, 0, sizeof(c));

    /* Find minimum size for a texture holding all visible ASCII characters */
    for(int i = 32; i < 128; i++)
    {
        if(FT_Load_Char(face, i, FT_LOAD_RENDER))
        {
            if (verbosity == 1) writeLog("[Atlas] Error before line "+QString::number(__LINE__));
            continue;
        }
        if(roww + glyph->bitmap.width + 1 >= 1024) {
            tex_w = std::max(tex_w, roww);
            tex_h += rowh;
            roww = 0;
            rowh = 0;
        }
        roww += glyph->bitmap.width + 1;
        rowh = std::max(rowh, glyph->bitmap.rows);
    }

    tex_w = std::max(tex_w, roww);
    tex_h += rowh;

    /* Create a texture that will be used to hold all ASCII glyphs */
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    //~ glUniform1i(text_uniform_tex, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_w, tex_h, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    /* We require 1 byte alignment when uploading texture data */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* Clamping to edges is important to prevent artifacts when scaling */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* Linear filtering usually looks best for text */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* Paste all glyph bitmaps into the texture, remembering the offset */
    int ox = 0; // x offset (in pixels)
    int oy = 0; // y offset (in pixels)
    rowh = 0;

    for(int i = 32; i < 128; i++) {
        if(FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            if (verbosity == 1) writeLog("[Atlas] Error before line "+QString::number(__LINE__));
            continue;
        }

        if(ox + glyph->bitmap.width + 1 >= 1024) {
            oy += rowh;
            rowh = 0;
            ox = 0;
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, glyph->bitmap.width, glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, glyph->bitmap.buffer);
        c[i].ax = glyph->advance.x >> 6;
        c[i].ay = glyph->advance.y >> 6;

        c[i].bw = glyph->bitmap.width;
        c[i].bh = glyph->bitmap.rows;

        c[i].bl = glyph->bitmap_left;
        c[i].bt = glyph->bitmap_top;

        c[i].tx = ox / (float)tex_w;
        c[i].ty = oy / (float)tex_h;

        rowh = std::max(rowh, glyph->bitmap.rows); // Ensure the lowest possible row height
        ox += glyph->bitmap.width + 1;
    }
}

Atlas::~Atlas()
{
    glDeleteTextures(1, &tex);
}

void Atlas::writeLog(QString str)
{
    writeToLogAndPrint(str.toStdString().c_str(), "riv.log", 1);
}
