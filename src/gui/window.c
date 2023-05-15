/*
 *  Copyright (C) 2023 Nicolai Brand, Callum Gran
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include "gui/window.h"
#include "gui/window_utils.h"

GLFWwindow *window_create()
{
    GLFWwindow *window;

    if (!glfwInit()) {
	fprintf(stderr, "Failed to initialize window.\n");
	return NULL;
    }

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Gogool Maps", NULL, NULL);

    GLFWimage images[1];
    images[0].pixels = stbi_load("./resources/icon.png", &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(window, 1, images);
    stbi_image_free(images[0].pixels);

    if (!window) {
	glfwTerminate();
	return NULL;
    }

    glfwMakeContextCurrent(window);

    glViewport(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    glOrtho(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT, 0, 1);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    // TODO: This pointer will be used as the window's user pointer. It will be used to pass data to
    // the window's callbacks.
    void *pointer = NULL;

    glfwSetWindowUserPointer(window, (void *)pointer);

    return window;
}

void window_update(GLFWwindow *window)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(1);

    // TODO: This pointer will be used as the window's user pointer. It will be used to pass data to
    // the window's callbacks.
    void *pointer = (void *)glfwGetWindowUserPointer(window);

    // For loop here to iterate through all nodes

    GLfloat point_vertex[] = { longitude_to_x(/* Insert node here) */ 0),
			       latitude_to_y(/* Insert node here */ 0), 0 };

    glVertexPointer(2, GL_FLOAT, 0, point_vertex);
    glDrawArrays(GL_POINTS, 0, 1);

    glDisable(GL_POINT_SMOOTH);
    glDisableClientState(GL_VERTEX_ARRAY);

    glfwSwapBuffers(window);

    glfwPollEvents();
}