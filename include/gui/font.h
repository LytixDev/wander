#ifdef GUI
#ifndef FONT_H
#define FONT_H

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <stdlib.h>

extern FT_Library ft;
extern FT_Face face;

void init_free_type();

void load_font();

void clean_font();

void render_text(const char *text, float x, float y, float scale);

#endif /* FONT_H */
#endif /* GUI */
