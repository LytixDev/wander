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
#include <math.h>

#include "gui/window.h"
#include "lib/common.h"
#include "ulsr/impl.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) 
{
    glViewport(0.0f, 0.0f, width, height);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    glOrtho(0, width, 0, height, -1, 1);

    glMatrixMode(GL_MODELVIEW);
}

GLFWwindow *window_create() 
{
    GLFWwindow *window;

    if (!glfwInit()) {
	fprintf(stderr, "Failed to initialize window.\n");
	return NULL;
    }

    window = glfwCreateWindow(SIMULATION_WIDTH, SIMULATION_LENGTH, "ULSR Simulation", NULL, NULL);

    GLFWimage images[1]; 
    images[0].pixels = stbi_load("./include/static/icon.png", &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(window, 1, images); 
    stbi_image_free(images[0].pixels);

    if (!window) {
        glfwTerminate();
        return NULL;
    }

    int width = 0; 
    int height = 0;

    glfwGetFramebufferSize(window, &width, &height);

    glfwMakeContextCurrent(window);

    glViewport(0.0f, 0.0f, width, height);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    glOrtho(0, width, 0, height, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return window;
}

static void draw_circle(float center_x, float center_y, float radius) {
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_LINE_LOOP);
    u16 i;
    for (i = 0; i < 360; i++) {
        float theta = i * (3.14159265358979323846 / 180.0);
        float x = center_x + radius * cos(theta);
        float y = center_y + radius * sin(theta);
        glVertex2f(x, y);
    }
    glEnd();
}

static void draw_node_coords()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_POINT);
    glEnableClientState(GL_COLOR_ARRAY);
    glPointSize(10);

    GLfloat colors[] = {
        0, 0, 255,
        0, 0, 255,
        0, 0, 255
    };

    glColorPointer(3, GL_FLOAT, 0, colors);

    // For loop here to iterate through all nodes
    for (u16 i = 0; i < MESH_NODE_COUNT; i++) {
        GLfloat point_vertex[] = {
                coords[i].x,
                coords[i].y,
                0
            };
        glVertexPointer(2, GL_FLOAT, 0, point_vertex);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_POINT);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_target_coords()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_POINT);
    glEnableClientState(GL_COLOR_ARRAY);
    glPointSize(20);

    GLfloat point_vertex[] = {
            target_coords.x,
            target_coords.y,
            0
        };

    GLfloat colors[] = {
            255, 0, 0,
            255, 0, 0,
            255, 0, 0
        };
    
    glColorPointer(3, GL_FLOAT, 0, colors);
    glVertexPointer(2, GL_FLOAT, 0, point_vertex);
    glDrawArrays(GL_POINTS, 0, 1);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_POINT);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_ranges()
{
    float current_time = glfwGetTime();
    float radius = STARTING_RING_RADIUS + (RING_SPEED * current_time);
    
    if (radius > SIMULATION_NODE_RANGE) {
        glfwSetTime(0);
    }

    for (u16 i = 0; i < MESH_NODE_COUNT; i++) {
        draw_circle(coords[i].x, coords[i].y, radius);
    }
}

void window_update(GLFWwindow *window)
{
    glClear(GL_COLOR_BUFFER_BIT);

    draw_node_coords();

    draw_target_coords();

    draw_ranges();

    glfwSwapBuffers(window);

    glfwPollEvents();
}