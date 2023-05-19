#ifndef FONT_H
#define FONT_H

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <stdlib.h>

void init_free_type(FT_Library *ft);

void load_font(FT_Library *ft, FT_Face *face);

void render_text(FT_Face *face, const char *text, float x, float y, float scale);

#endif /* FONT_H */