# WANDER
### Wireless mesh Ad-hoc Network protocol for Triangulation Evasion and Routing

Copyright © 2023 Nicolai Brand and Callum Grand under GPL v3 (see LICENSE.txt). .clang-format is licensed under GPL v2.

## Table of Contents
- [Compile and run](#compile)
- [Functionality](#functionality)
- [Further development](#further-development)
- [Known bugs](#known-bugs)


## Compile and run

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


## Functionality

### Protocol
Definitions:
- client: the device that connects to the mobile mesh network, sends requests and expects a response.
- internet café: the external internet that a device in the mobile mesh network can connect to and make internet requests from if its in range.
- node: a device (f.ex drone) in the mobile mesh network

Problem statement:
I (the client) want to visit a website, but I do not want anyone to know I visited this website. In order to solve this, I buy a bunch of drones and connect them in a mobile mesh network. When I send a request to visit a website, the request is sent to a device in the mesh network. This request is bounced trough the network before it ends up in the hand of a device that is in range to connect to an internet café. This device makes the request to the website through internet café. The response from the request is sent back trough the network until it finally is received by me (the client). In doing this, both the café and the website should not be able to trace that the request came from me making triangulation not possible.

Abstract:
We have created our own protocol for both internal communication within a mobile mesh network and external connection to the client who uses the mesh network. The protocol defines how data is parsed and routed, but leaves the details for how data is physically sent to the implementation of the protocol. This means that the protocol does not care whether data is sent trough ethernet, wi-fi or bluetooth. The protocol defines send and recv function pointers that the implementation of the protocol has to implement. The protocol is stateless, meaning once a packet is dealth with by a device in the network it is forgotten about and no state is saved.


Due to time limitations, we have made some simplifications / assumptions to be able to create a finished protocol.

Simplifications:
- The client connecting to the mobile mesh network is only able to send HTTP requests.
- The mobile mesh network will never get new devices, but it may lose devices.
- Only one client.
- Only one internet café.
- The network only operates in two dimensions: x and y.

Assumptions:
- A mesh device in range of the internet café will always be connected on the internet café.
- The mesh device that the client first connects to will always be in range of the client. I.E: that device will never move.
- Every mesh device knows how to connect to every other mesh device based on the ID of the other device.

How it works:
When a node is initialized it creates two threads.
Thread one reads, parses and re-sends incoming packets. It also makes requests to the outside world (café, client) if needed.
Thread two polls every other known node in the network to see if they are in range and can be communicated with.

When the network first starts, the nodes have no knowledge of the network topology. This is continiously built trough polling and finding nearby nodes. When nearby nodes are found, the nodes starts to build routes and increase its understanding of the topology. Note: nodes can at any time move it positio, and so the topology of the network is not constant and the nodes need to continiously update its understanding as changes happen.

Routes are built using a flood fill algorithm. When deciding a route, a node will chose randomly between all the routes it thinks will work. The time a route take will always be the time the longest route take. This is a security feature in order to ...

Nodes can also route packets correctly despite having no routes. This is done using what we have coined "bogo-routing". 

### Simulation
The simulation environment in the project is quite advanced. It is possible to simulate a large number of nodes in a mobile mesh network, and to simulate the movement of these nodes. The simulation environment is also capable of simulating the transmission of messages between nodes in the mesh network. The simulation environment is also capable of simulating the transmission of HTTP requests to external servers through the mesh network. In the simulation, the nodes are agnostic to the fact that they are simulated, and they behave as if they were real nodes in a real mobile mesh network.

### GUI
The GUI is made from scratch using OpenGL (bad idea) without any component library (even worse). The GUI is also capable of visualizing the simulated mesh network in real time and allows the user to interact with the mesh network through a graphical user interface. It is fast, but volatile, and seg faults may happen :-).

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
- Press the node again to deselect it.

### Implementation
The project only uses the C-standard library and the POSIX library. There are no third party code other than the GUI and the language itself.

The project is written in C11 and compiles with gcc and clang. The project is developed with Linux in mind, but it should be possible to compile and run the project on other operating systems as well.

## Further development
- In the future, packets in and out of the network should not be bound to TCP, but rather be raw IP-packets.
- The GUI is generally volatile and may seg fault.
- The GUI is currently fixed size, in the future it should be possible to resize the GUI and move around in the mesh network.
- There are also many bugs in the code that need to be fixed, some known and others unknown. We have written up a list of all bugs we have come across, expect this list to grow.

## Known bugs
- The whole GUI is buggy and largely needs to be rewritten as there are multiple causes for segfaults and memory leaks.
- Packets and routes are not always freed correctly in the simulation, which causes memory leaks. This is however not a problem in a real world scenario, as the packets are freed when they are sent as it would go through a socket.
