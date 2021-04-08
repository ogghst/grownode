#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "GrowNodeController.h"

#define TASK_STACK_SIZE 8192

using namespace GrowNode::Controller;

extern "C" void app_main(void)
{

	GrowNodeController* board = GrowNodeController::getInstance();

	while (1) {
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}

}
