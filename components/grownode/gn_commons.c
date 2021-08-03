#ifdef __cplusplus
extern "C" {
#endif

//#include "esp_log.h"
#include "gn_commons.h"
#include "grownode_intl.h"

//static const char *TAG = "gn_commons";

inline size_t gn_common_leaf_event_mask_param(gn_leaf_event_handle_t evt,
		gn_leaf_param_handle_t param) {

	if (!evt || !param)
		return 1;

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) param->leaf_config;

	if (strcmp(evt->leaf_name, _leaf_config->name) == 0
			&& strcmp(evt->param_name, param->name) == 0)
		return 0;
	return 1;
}

#ifdef __cplusplus
}
#endif //__cplusplus

