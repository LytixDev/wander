# WANDER
### Wireless mesh Ad-hoc Network protocol for Triangulation Evasion and Routing

Copyright © 2023 Nicolai Brand and Callum Grand under GPL v3 (see LICENSE.txt). .clang-format is licensed under GPL v2.

## Table of Contents
- [Compile and run](#compile)
- [Functionality](#functionality)
- [Further development](#further-development)
- [Known bugs](#known-bugs)


## Compile and run {#compile}

The project is built using a `Make`. The project has three targets: `make`, `make gui`, `make gui_macos` and `make client`.

- `make` builds the simulated mobile mesh.
- `make client` builds a client program that connects to the simulated mobile mesh.
- `make gui` and `make gui_macos` builds a visualization of the simulated mobile mesh.
 
IMPORTANT: if you have already built the simulated mobile mesh using `make`, you need to `make clean` before you can `make gui` or `make gui_macos`.


### Dependencies
- `UNIX` - access to Unix-like operating system commands (find, rm, etc.). WSL works fine on Windows.
- `Make` - for building the source code.
- `cc` - a C11 compiler. Tested and successfully compiles using gcc and clang.

#### Optional dependencies for the visualization (make gui and make gui_macos)
- `OpenGL` - Graphics rendering library.
For Linux, BSD and Windows:
- `X11` - The X11 window system. Wayland users can use the Xwayland compatability layer. For Winodws users you need a window server like VcXsrv.

For Mac OS (homebrew), the added dependencies required to make the gui are:
- glew
- glfw

For debian-based systems (apt), the added dependencies required to make the gui are:
- libglfw3-dev 
- libgl-dev 
- libglew-dev 
- libxxf86vm-dev 
- libxi-dev 
- libxinerama-dev

For arch-based systems (pacman), the added dependencies required to make the gui are:
- glfw-x11
- mesa
- glew
- libxxf86vm
- libxi
- libxinerama

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
On Mac OS:
```sh
$ make gui_macos
```

The client works irrespective of the whether the simulation is graphically visualized or not.
```sh
$ make client
```

### Run

To run the simulation:
```sh
$ ./wander
```

To run the simulation with a graphical visualization:
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


## Functionality {#functionality}

### Protocol
This project is a work in progress as far as the routing protocol is concerned. The current functionality is as follows:
- The ability to send and receive messages between nodes in the mesh network.
- The ability to send and receive HTTP requests to external servers through the mesh network.
- Routing of messages between nodes in the mesh network without initial knowledge of the network topology.
- Routing protocol that always finds a path to the destination node if one exists.
- Triangulation evasion by using random paths between nodes in the mesh network.
- Routing tables built by using flooding.
- Routing protocol that uses a combination of the routing table and random paths to obfuscate triangulation.

### Simulation
The simulation environment in the project is quite advanced. It is possible to simulate a large number of nodes in a mesh network, and to simulate the movement of these nodes. The simulation environment is also capable of simulating the transmission of messages between nodes in the mesh network. The simulation environment is also capable of simulating the transmission of HTTP requests to external servers through the mesh network. In the simulation, the nodes are agnostic to the fact that they are simulated, and they behave as if they were real nodes in a real mesh network.

### GUI
The GUI is made completely from scratch without any component library, including the text that is rendered using OpenGL. The GUI is also capable of visualizing the simulated mesh network in real time and allows the user to interact with the mesh network through a graphical user interface.

Filtering options for the GUI:
- Internal packets
  - Hello packets
  - Routing packets
  - Routing done packets
  - Data packets 
  - Purge packets (not implemented in the protocol yet)
- Request types
  - Send request
  - Receive request
  - All requests
- Show failed requests

To select a node:
- Press on a node.

To deselect a node:
- Press on the selected node.

To move a node:
- Select a node.
- Press where you want to move the node.        

### Implementation
For the ease of use for testers and future developers, the project only uses the C-standard library and the POSIX library, except for the GUI. The project is written in C11 and compiles with gcc and clang. The project is developed on Linux, but it should be possible to compile and run the project on other operating systems as well. The GUI requires OpenGL and GLFW, and is developed for Linux and Mac-OS.


## Further development {#further-development}
- In the future, packets in and out of the network should not be bound to TCP, but rather be raw IP-packets.
- The GUI is currently fixed size, in the future it should be possible to resize the GUI and move around in the mesh network.
- There are also many bugs in the code that need to be fixed, some known and others unknown. We have written up a list of all bugs we have come across, expect this list to grow.

## Known bugs {#known-bugs}
- The whole GUI is buggy and largely needs to be rewritten as there are multiple causes for segfaults and memory leaks.
- Packets are not always freed correctly, which causes memory leaks. This is however not a problem in a real world scenario, as the packets are freed when they are sent as it would go through a socket.
