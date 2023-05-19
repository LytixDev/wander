#include <stdio.h>
#include <stdlib.h>

#include "gui/font.h"
#include "lib/logger.h"

void init_free_type(FT_Library *ft)
{
    if (FT_Init_FreeType(ft)) {
	LOG_ERR("Could not init FreeType library");
	return;
    }
}

void load_font(FT_Library *ft, FT_Face *face)
{
    if (FT_New_Face(*ft, "include/static/open_sans_semibold.ttf", 0, face)) {
	LOG_ERR("Could not load font");
	return;
    }

    FT_Set_Pixel_Sizes(*face, 0, 24);
}

void clean_font(FT_Library *ft, FT_Face *face)
{
    FT_Done_Face(*face);
    FT_Done_FreeType(*ft);
}

void render_text(FT_Face *face, const char *text, float x, float y, float scale)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    FT_GlyphSlot glyphSlot = (*face)->glyph;

    for (size_t i = 0; text[i]; ++i) {
	if (FT_Load_Char(*face, text[i], FT_LOAD_RENDER))
	    continue;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, glyphSlot->bitmap.width, glyphSlot->bitmap.rows, 0,
		     GL_RED, GL_UNSIGNED_BYTE, glyphSlot->bitmap.buffer);

	float xpos = x + glyphSlot->bitmap_left * scale;
	float ypos = y - glyphSlot->bitmap_top * scale;
	float w = glyphSlot->bitmap.width * scale;
	float h = glyphSlot->bitmap.rows * scale;

	GLfloat vertices[6][4] = { { xpos, ypos + h, 0.0, 0.0 },    { xpos, ypos, 0.0, 1.0 },
				   { xpos + w, ypos, 1.0, 1.0 },

				   { xpos, ypos + h, 0.0, 0.0 },    { xpos + w, ypos, 1.0, 1.0 },
				   { xpos + w, ypos + h, 1.0, 0.0 } };

	// Render glyph quad
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	x += (glyphSlot->advance.x >> 6) * scale;
	y += (glyphSlot->advance.y >> 6) * scale;
    }
}