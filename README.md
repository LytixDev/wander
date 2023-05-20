# Project in IDATT2104
Copyright © 2023 Nicolai Brand and Callum Grand under GPL v3 (see LICENSE.txt). .clang-format is licensed under GPL v2.

## Compile and run

The project is built using a `Make`. The project has three targets: `make`, `make gui` and `make client`.

- `make` builds the simulated mobile mesh.
- `make client` builds a client program that connects to the simulated mobile mesh.
- `make gui` builds a visualization of the simulated mobile mesh.
 
IMPORTANT: if you have already built the simulated mobile mesh using `make`, you need to `make clean` before you can `make gui`.


#### Dependencies

- `UNIX` - access to Unix-like operating system commands (find, rm, etc.). WSL works fine on Windows.
- `Make` - for building the source code.
- `cc` - a C11 compiler. Tested with gcc and clang.

#### Optional dependencies for the visualization (make gui)
- `OpenGL` - Graphics rendering library.
- `X11` - The X11 window system. Wayland users can use the Xwayland compatability layer. For Winodws and Mac OS users you need a forwarding layer.
- `glfw-x11` An OpenGL "wrapper" for X11

For debian-based systems, the added dependencies required to make the gui are:
- libglfw3-dev 
- libgl-dev 
- libglew-dev 
- libxxf86vm-dev 
- libxi-dev 
- libxinerama-dev

For arch-based systems, the added dependencies required to make the gui are:
- glfw-x11
- mesa
- glew
- libxxf86vm
- libxi
- libxinerama

For Mac-OS, the added dependencies required to make the gui are:
- glew
- glfw

#### Optional dependencies for further development
- `python`, `clang-format` - for formatting the source code.
- `bear` - a tool for generating compiler commands for C compilers. 
- `valgrind` - a tool for finding memory leaks and other undefined behavior. Used in `tests/run.sh`

### Compile the simulation and client

Without visualization:
```sh
$ make
```

With graphical visualization:
```sh
$ make gui
```

The client works irrespective of the whether the simulation is graphically visualized or not, altough it will be slower during the graphical visualization.
```sh
$ make client
```

### Run

To run the simulation:
```sh
$ ./ulsr
```

To run the simulation graphically:
```sh
$ ./gui
```

To make a dummy HTTP request through the simulated mesh network:
```sh
$ ./client
```

### Other commands
- `make debug` - compiles the source with the `-g` and `-DDEBUG` options.
- `make debug-verbose` - fancy printf debugging :-)
- `make clean` - not dirty.
- `make bear` - generates a file of the compiler options.
- `make format` - makes the code objectively pretty.
