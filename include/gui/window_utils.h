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

/* Definitions */

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480

#define MAX_LONG 32
#define MIN_LONG 4

#define MAX_LAT 72
#define MIN_LAT 54

/* Methods */

/**
 * Method to convert longitude to x coordinate
 * @param longitude
 * @return x coordinate
 */
double longitude_to_x(double longitude);

/**
 * Method to convert latitude to y coordinate
 * @param latitude
 * @return y coordinate
 */
double latitude_to_y(double latitude);

/**
 * Method to convert x coordinate to longitude
 * @param x coordinate
 * @return longitude
 */
double x_to_longitude(double x);

/**
 * Method to convert y coordinate to latitude
 * @param y coordinate
 * @return latitude
 */
double y_to_latitude(double y);