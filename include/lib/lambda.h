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

#ifndef LAMBDA_H
#define LAMBDA_H

/**
 * Macro to create an lambda function.
 * @param return_type The return type of the lambda function.
 * @param func_args The arguments of the lambda function.
 * @param func_body The body of the lambda function.
 * @return A pointer to the lambda function.
 */
#define LAMBDA(return_type, func_args, func_body) \
    __extension__ ({ return_type __lambda__  func_args func_body &__lambda__; })

#endif