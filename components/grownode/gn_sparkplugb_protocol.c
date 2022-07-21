// Copyright 2021 Nicola Muratori (nicola.muratori@gmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifdef __cplusplus
extern "C" {
#endif

#include <tahu.h>
#include <tahu.pb.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "esp_log.h"


#include "grownode.h"
#include "grownode_intl.h"
#include "gn_commons.h"

#define TAG "gn_sparkplugb_protocol"

#undef SPARKPLUG_DEBUG

//#define PB_WITHOUT_64BIT


enum alias_map {
    Next_Server = 0,
    Rebirth = 1,
    Reboot = 2,
    Dataset = 3,
    Node_Metric0 = 4,
    Node_Metric1 = 5,
    Node_Metric2 = 6,
    Device_Metric0 = 7,
    Device_Metric1 = 8,
    Device_Metric2 = 9,
    Device_Metric3 = 10,
    My_Custom_Motor = 11
};


/*
 * Helper function to publish a NBIRTH message.  The NBIRTH should include all 'node control' metrics that denote device capability.
 * In addition, it should include every node metric that may ever be published from this edge node.  If any NDATA messages arrive at
 * MQTT Engine that were not included in the NBIRTH, MQTT Engine will request a Rebirth from the device.
 */
void publish_node_birth(gn_node_handle_t node) {
    // Create the NBIRTH payload
    org_eclipse_tahu_protobuf_Payload nbirth_payload;

    // Initialize the sequence number for Sparkplug MQTT messages
    // This must be zero on every NBIRTH publish
    reset_sparkplug_sequence();
    get_next_payload(&nbirth_payload);
    nbirth_payload.uuid = strdup("MyUUID");

    // Add node control metrics
    //fprintf(stdout, "Adding metric: 'Node Control/Next Server'\n");
    bool next_server_value = false;
    add_simple_metric(&nbirth_payload, "Node Control/Next Server", true, Next_Server, METRIC_DATA_TYPE_BOOLEAN, false, false, &next_server_value, sizeof(next_server_value));
    //fprintf(stdout, "Adding metric: 'Node Control/Rebirth'\n");
    bool rebirth_value = false;
    add_simple_metric(&nbirth_payload, "Node Control/Rebirth", true, Rebirth, METRIC_DATA_TYPE_BOOLEAN, false, false, &rebirth_value, sizeof(rebirth_value));
    //fprintf(stdout, "Adding metric: 'Node Control/Reboot'\n");
    bool reboot_value = false;
    add_simple_metric(&nbirth_payload, "Node Control/Reboot", true, Reboot, METRIC_DATA_TYPE_BOOLEAN, false, false, &reboot_value, sizeof(reboot_value));

    // Add some regular node metrics
    //fprintf(stdout, "Adding metric: 'Node Metric0'\n");
    char nbirth_metric_zero_value[] = "hello node";
    add_simple_metric(&nbirth_payload, "Node Metric0", true, Node_Metric0, METRIC_DATA_TYPE_STRING, false, false, &nbirth_metric_zero_value, sizeof(nbirth_metric_zero_value));
    //fprintf(stdout, "Adding metric: 'Node Metric1'\n");
    bool nbirth_metric_one_value = true;
    add_simple_metric(&nbirth_payload, "Node Metric1", true, Node_Metric1, METRIC_DATA_TYPE_BOOLEAN, false, false, &nbirth_metric_one_value, sizeof(nbirth_metric_one_value));

    // Create a DataSet
    org_eclipse_tahu_protobuf_Payload_DataSet dataset = org_eclipse_tahu_protobuf_Payload_DataSet_init_default;
    uint32_t datatypes[] = { DATA_SET_DATA_TYPE_INT8,
        DATA_SET_DATA_TYPE_INT16,
        DATA_SET_DATA_TYPE_INT32 };
    const char *column_keys[] = { "Int8s",
        "Int16s",
        "Int32s" };
    org_eclipse_tahu_protobuf_Payload_DataSet_Row *row_data = (org_eclipse_tahu_protobuf_Payload_DataSet_Row *)
        calloc(2, sizeof(org_eclipse_tahu_protobuf_Payload_DataSet_Row));
    row_data[0].elements_count = 3;
    row_data[0].elements = (org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue *)
        calloc(3, sizeof(org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue));
    row_data[0].elements[0].which_value = org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue_int_value_tag;
    row_data[0].elements[0].value.int_value = 0;
    row_data[0].elements[1].which_value = org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue_int_value_tag;
    row_data[0].elements[1].value.int_value = 1;
    row_data[0].elements[2].which_value = org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue_int_value_tag;
    row_data[0].elements[2].value.int_value = 2;
    row_data[1].elements_count = 3;
    row_data[1].elements = (org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue *)
        calloc(3, sizeof(org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue));
    row_data[1].elements[0].which_value = org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue_int_value_tag;
    row_data[1].elements[0].value.int_value = 3;
    row_data[1].elements[1].which_value = org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue_int_value_tag;
    row_data[1].elements[1].value.int_value = 4;
    row_data[1].elements[2].which_value = org_eclipse_tahu_protobuf_Payload_DataSet_DataSetValue_int_value_tag;
    row_data[1].elements[2].value.int_value = 5;
    init_dataset(&dataset, 2, 3, datatypes, column_keys, row_data);

    // Create the a Metric with the DataSet value and add it to the payload
    //fprintf(stdout, "Adding metric: 'DataSet'\n");
    org_eclipse_tahu_protobuf_Payload_Metric dataset_metric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
    init_metric(&dataset_metric, "DataSet", true, Dataset, METRIC_DATA_TYPE_DATASET, false, false, &dataset, sizeof(dataset));
    add_metric_to_payload(&nbirth_payload, &dataset_metric);


    // Add a metric with a custom property
    //fprintf(stdout, "Adding metric: 'Node Metric2'\n");
    org_eclipse_tahu_protobuf_Payload_Metric prop_metric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
    uint32_t nbirth_metric_two_value = 13;
    init_metric(&prop_metric, "Node Metric2", true, Node_Metric2, METRIC_DATA_TYPE_INT16, false, false, &nbirth_metric_two_value, sizeof(nbirth_metric_two_value));
    org_eclipse_tahu_protobuf_Payload_PropertySet properties = org_eclipse_tahu_protobuf_Payload_PropertySet_init_default;
    add_property_to_set(&properties, "engUnit", PROPERTY_DATA_TYPE_STRING, "MyCustomUnits", sizeof("MyCustomUnits"));
    add_propertyset_to_metric(&prop_metric, &properties);
    add_metric_to_payload(&nbirth_payload, &prop_metric);

    /*
    // Create a metric called RPMs which is a member of the UDT definition - note aliases do not apply to UDT members
    org_eclipse_tahu_protobuf_Payload_Metric rpms_metric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
    uint32_t rpms_value = 0;
    init_metric(&rpms_metric, "RPMs", false, 0, METRIC_DATA_TYPE_INT32, false, false, &rpms_value, sizeof(rpms_value));

    // Create a metric called AMPs which is a member of the UDT definition - note aliases do not apply to UDT members
    org_eclipse_tahu_protobuf_Payload_Metric amps_metric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
    uint32_t amps_value = 0;
    init_metric(&amps_metric, "AMPs", false, 0, METRIC_DATA_TYPE_INT32, false, false, &amps_value, sizeof(amps_value));

    // Create a Template/UDT Parameter - this is purely for example of including parameters and is not actually used by UDT instances
    org_eclipse_tahu_protobuf_Payload_Template_Parameter parameter = org_eclipse_tahu_protobuf_Payload_Template_Parameter_init_default;
    parameter.name = strdup("Index");
    parameter.has_type = true;
    parameter.type = PARAMETER_DATA_TYPE_STRING;
    parameter.which_value = org_eclipse_tahu_protobuf_Payload_Template_Parameter_string_value_tag;
    parameter.value.string_value = strdup("0");

    // Create the UDT definition value which includes the UDT members and parameters
    org_eclipse_tahu_protobuf_Payload_Template udt_template = org_eclipse_tahu_protobuf_Payload_Template_init_default;
    udt_template.metrics_count = 1;
    udt_template.metrics = (org_eclipse_tahu_protobuf_Payload_Metric *)calloc(2, sizeof(org_eclipse_tahu_protobuf_Payload_Metric));
    udt_template.metrics[0] = rpms_metric;
    //udt_template.metrics[1] = amps_metric;

    udt_template.parameters_count = 1;
    udt_template.parameters = (org_eclipse_tahu_protobuf_Payload_Template_Parameter *)calloc(1, sizeof(org_eclipse_tahu_protobuf_Payload_Template_Parameter));
    udt_template.parameters[0] = parameter;
    udt_template.template_ref = NULL;
    udt_template.has_is_definition = true;
    udt_template.is_definition = true;



    // Create the root UDT definition and add the UDT definition value which includes the UDT members and parameters
    org_eclipse_tahu_protobuf_Payload_Metric metric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
    init_metric(&metric, "_types_/Custom_Motor", false, 0, METRIC_DATA_TYPE_TEMPLATE, false, false, &udt_template, sizeof(udt_template));


    // Add the UDT to the payload
    add_metric_to_payload(&nbirth_payload, &metric);
	*/

#ifdef SPARKPLUG_DEBUG
    // Print the payload for debug
    print_payload(&nbirth_payload);
#endif

    // Encode the payload into a binary format so it can be published in the MQTT message.
    // The binary_buffer must be large enough to hold the contents of the binary payload
    size_t buffer_length = 4096;
    uint8_t *binary_buffer = (uint8_t *)malloc(buffer_length * sizeof(uint8_t));
    size_t message_length = encode_payload(binary_buffer, buffer_length, &nbirth_payload);

    ESP_LOGI(TAG, "payload: %s", (char*)binary_buffer);

    // Publish the NBIRTH on the appropriate topic
    //mosquitto_publish(mosq, NULL, "spBv1.0/Sparkplug B Devices/NBIRTH/C Edge Node 1", message_length, binary_buffer, 0, false);
    esp_mqtt_client_publish( ((gn_node_handle_intl_t) node)->config->mqtt_client, "spBv1.0/Sparkplug/NBIRTH/TestNode", (char*)binary_buffer, message_length, 0, 0);


    // Free the memory
    free(binary_buffer);
    free(row_data);
    free_payload(&nbirth_payload);
}


gn_err_t gn_spb_test(gn_node_handle_t node) {

	publish_node_birth( node);

	return GN_RET_OK;
}


#ifdef __cplusplus
}
#endif


