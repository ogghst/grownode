# Event subsystem

Main application works in a RTOS task. Leaves works in dedicated tasks. Networking and other ESP-IDF services has their own tasks as well. This means that all communication through those components must be done using the RTOS task messaging features and higher ESP-IDF abstractions.

GrowNode uses [Event Loop library](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_event.html) from ESP-IDF to list, subscribe and publish events. It declares one base event `GN_BASE_EVENT` and a `gn_event_id_t` enumeration where all event types are listed.

## Subscribing events

In order to grab a specific event you can rely on ESP-IDF event loop functions. In order to recall the proper event loop from a Leaf or Node, `gn_leaf_get_event_loop()` and `gn_node_get_event_loop()` are provided. See example:

```
//register for events
esp_event_handler_instance_register_with(gn_leaf_get_event_loop(leaf_config), GN_BASE_EVENT,  
	GN_EVENT_ANY_ID, gn_leaf_led_status_event_handler, leaf_config, NULL);

```

This code creates a subscription for all events (`GN_EVENT_ANY_ID`), calling `gn_leaf_led_status_event_handler` callback once an event is triggered, and pass the `leaf_config` pointer in the context.

Those functions returns currently the same event loop, different implementations are made for future needs.

## Publishing events

Event are published using straight esp event loop functionalities: 

```
esp_event_post_to(event_loop, GN_BASE_EVENT,
		GN_NET_CONNECTED_EVENT, NULL, 0,
		portMAX_DELAY);
```

## Listening for event

Event callbacks shall implement `esp_event_handler_t` syntax. Payload is dependent on the event type triggered: 

```
void gn_pump_control_task_event_handler(void *handler_args,
		esp_event_base_t base, int32_t event_id, void *event_data) {

	gn_leaf_parameter_event_t *evt = (gn_leaf_parameter_event_t*) event_data;
	switch (event_id) {
	case GN_LEAF_PARAM_CHANGED_EVENT:
...
```

## Acquiring events in leaf code

Events addressed to leaves from other leaves and from network has a special, direct way to be processed, that improves GrowNode engine performances, using RTOS queues. Leaves that has just to wait for an event can implement an infinite loop where an `xQueueReceive()` function waits for events for a specific time window. If no events are presents in the queue, the queue releases the control to the leaf: 

```
gn_leaf_parameter_event_t evt;
	
if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt, 
	pdMS_TO_TICKS(100)) == pdPASS) {
	
	//event arrived for this node
	switch (evt.id) {

		//parameter change
		case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

		ESP_LOGD(TAG, "request to update param %s, data = '%s'",
			evt.param_name, evt.data);
	...
```
