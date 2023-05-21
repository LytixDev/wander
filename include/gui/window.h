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
#ifdef GUI
#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include "font.h"
#include "wander/impl.h"

#define STARTING_RING_RADIUS 50.0f
#define RING_SPEED 100.0f
#define TOOLBAR_HEIGHT 50.0f
#define TOOLBAR_ITEM_COUNT 5
#define FILTERBAR_WIDTH 140.0f
#define FILTERBAR_ITEM_COUNT 3

static char *toolbar_items[] = { "Hello Packets", "Data Packets", "Purge Packets",
				 "Routing Packets", "Routing Done Packets" };

static char *request_filter[] = { "Recv", "Send", "All" };

/**
 * Method to create a window
 * @return a pointer to the window
 */
GLFWwindow *window_create();

/**
 * Method to update the window
 * @param window the window to update
 */
void window_update(GLFWwindow *window);

/**
 * Method to destroy the window
 * @param window the window to destroy
 */
void window_destroy(GLFWwindow *window);

#endif
#endif
