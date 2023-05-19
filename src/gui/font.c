#include <stdio.h>
#include <stdlib.h>

#include "gui/font.h"
#include "lib/logger.h"

void init_free_type()
{
    if (FT_Init_FreeType(&ft)) {
	LOG_ERR("Could not init FreeType library");
	return;
    }
}

void load_font()
{
    if (FT_New_Face(ft, "include/static/open_sans_semibold.ttf", 0, &face)) {
	LOG_ERR("Could not load font");
	return;
    }
    FT_Set_Pixel_Sizes(face, 0, 11);
}

void clean_font()
{
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void render_text(const char *text, float x, float y, float size)
{
    FT_GlyphSlot slot = face->glyph;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const char *c;
    for (c = text; *c; ++c) {
	if (FT_Load_Char(face, *c, FT_LOAD_RENDER))
	    continue;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, slot->bitmap.width, slot->bitmap.rows, 0, GL_ALPHA,
		     GL_UNSIGNED_BYTE, slot->bitmap.buffer);

	float xpos = x + slot->bitmap_left * size;
	float ypos = y - (slot->bitmap.rows - slot->bitmap_top) * size;

	float w = slot->bitmap.width * size;
	float h = slot->bitmap.rows * size;

	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(xpos, ypos + h);
	glTexCoord2f(1, 0);
	glVertex2f(xpos + w, ypos + h);
	glTexCoord2f(1, 1);
	glVertex2f(xpos + w, ypos);
	glTexCoord2f(0, 1);
	glVertex2f(xpos, ypos);
	glEnd();

	x += (slot->advance.x >> 6) * size;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
    }
}