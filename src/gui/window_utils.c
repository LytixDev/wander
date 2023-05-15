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

#include <stdlib.h>

#include "gui/window_utils.h"

double longitude_to_x(double longitude)
{
    return ((longitude - MIN_LONG) * (SCREEN_WIDTH / (MAX_LONG - MIN_LONG)));
}

double latitude_to_y(double latitude)
{
    return ((latitude - MIN_LAT) * (SCREEN_HEIGHT / (MAX_LAT - MIN_LAT)));
}

double x_to_longitude(double x)
{
    return ((x / ((SCREEN_WIDTH) / (MAX_LONG - MIN_LONG))) + MIN_LONG);
}

double y_to_latitude(double y)
{
    return (((SCREEN_WIDTH - y) / ((SCREEN_WIDTH) / (MAX_LAT - MIN_LAT))) + MIN_LAT);
}