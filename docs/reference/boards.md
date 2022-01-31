# Boards

Boards are a collection of preconfigured Leaves to have a ready made solution, described in [Boards](boards.md) section. Goal is to give you a working solution without the need of develop your own code.

In order to include a board in your code, you just need to modify your `main.c` file including the appropriate header file in `components/grownode/boards library` folder and call the appropriate board configuration function:

```
	...
	//header include the board you want to start here
	#include "gn_blink.h"
	...
	//creates a new node
	gn_node_handle_t node = gn_node_create(config, "node");

	//the board to start
	gn_configure_blink(node);

	//finally, start node
	gn_node_start(node);

```
