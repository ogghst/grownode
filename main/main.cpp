#include <stdio.h>

#include "GrowNodeController.h"

#define TASK_STACK_SIZE 8192

extern "C" void app_main(void) {

	GrowNode::Controller::GrowNodeController c;

	//char buffer[30];

	//int i = 0;
	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		//sprintf(buffer, "loop - %i", i++);
		//c.logMessage(buffer);
	}

}
