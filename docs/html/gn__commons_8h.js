var gn__commons_8h =
[
    [ "gn_leaf_parameter_event_t", "structgn__leaf__parameter__event__t.html", "structgn__leaf__parameter__event__t" ],
    [ "gn_node_event_t", "structgn__node__event__t.html", "structgn__node__event__t" ],
    [ "gn_leaf_descriptor_t", "structgn__leaf__descriptor__t.html", "structgn__leaf__descriptor__t" ],
    [ "gn_val_t", "uniongn__val__t.html", "uniongn__val__t" ],
    [ "GN_LEAF_DATA_SIZE", "gn__commons_8h.html#ad0d1b62a98d007235779dad6a4535f76", null ],
    [ "GN_LEAF_DESC_TYPE_SIZE", "gn__commons_8h.html#ad944f7538f019062a7a77ee2c58fd2c4", null ],
    [ "GN_LEAF_NAME_SIZE", "gn__commons_8h.html#a48278eb7fc42efe12aa42d14a2a4dac1", null ],
    [ "GN_LEAF_PARAM_NAME_SIZE", "gn__commons_8h.html#ada901cf84a29ea29f95c6c3b04ceb8a4", null ],
    [ "GN_NODE_DATA_SIZE", "gn__commons_8h.html#ab5fcb53a5f91fbe1761c64fc9b479554", null ],
    [ "GN_NODE_NAME_SIZE", "gn__commons_8h.html#a6bc4de805531c8ecaaa130b1f60d35d2", null ],
    [ "gn_config_handle_t", "gn__commons_8h.html#a18c5fddc25f0fb3b7b54941d5fd5a7cd", null ],
    [ "gn_display_container_t", "gn__commons_8h.html#ae43c023b9035f8108f183c4c22bd379b", null ],
    [ "gn_leaf_config_callback", "gn__commons_8h.html#a49ada5ca9d9069ea7dfd2838ce791b2b", null ],
    [ "gn_leaf_config_handle_t", "gn__commons_8h.html#ac267ca28d374926f5677e2fd61a24d40", null ],
    [ "gn_leaf_descriptor_handle_t", "gn__commons_8h.html#abec8ea226bf73658e6de4d9c75f5cc1a", null ],
    [ "gn_leaf_param_handle_t", "gn__commons_8h.html#a9149e2a43f5b7741a9c505ada717a1b6", null ],
    [ "gn_leaf_parameter_event_handle_t", "gn__commons_8h.html#ac8fc0c45b387e7714579df6d04777d97", null ],
    [ "gn_leaf_task_callback", "gn__commons_8h.html#a908e39475e093ae0eeba08e802827f5f", null ],
    [ "gn_node_config_handle_t", "gn__commons_8h.html#a5e88a7284994d06571e1318dfc464d39", null ],
    [ "gn_node_event_handle_t", "gn__commons_8h.html#a49ad31c392b0233fadffcaace7287df2", null ],
    [ "gn_config_status_t", "gn__commons_8h.html#a190c63cb05346feebeb594ef9313b092", [
      [ "GN_CONFIG_STATUS_NOT_INITIALIZED", "gn__commons_8h.html#a190c63cb05346feebeb594ef9313b092a6437eb2f05a6e1f524572401d366229e", null ],
      [ "GN_CONFIG_STATUS_INITIALIZING", "gn__commons_8h.html#a190c63cb05346feebeb594ef9313b092ab179d04c227739fe96009438061e1dca", null ],
      [ "GN_CONFIG_STATUS_ERROR", "gn__commons_8h.html#a190c63cb05346feebeb594ef9313b092adc333f10affc84b442bd44e7c9a10e08", null ],
      [ "GN_CONFIG_STATUS_NETWORK_ERROR", "gn__commons_8h.html#a190c63cb05346feebeb594ef9313b092ae0a641846fd1baef6b107f67b9a78016", null ],
      [ "GN_CONFIG_STATUS_SERVER_ERROR", "gn__commons_8h.html#a190c63cb05346feebeb594ef9313b092abaf461dba778c218f916fb22a94e3345", null ],
      [ "GN_CONFIG_STATUS_COMPLETED", "gn__commons_8h.html#a190c63cb05346feebeb594ef9313b092a06c94a0536ae1e36ef34df0e5cf9986b", null ],
      [ "GN_CONFIG_STATUS_STARTED", "gn__commons_8h.html#a190c63cb05346feebeb594ef9313b092a5e9ddf76d3ef63637306e33c95b0daa1", null ]
    ] ],
    [ "gn_err_t", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91", [
      [ "GN_RET_OK", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91ae21dcd194bfce897eb4e9a5d354cbe45", null ],
      [ "GN_RET_ERR", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91a08bf09068137673f9c21ebaef678c56a", null ],
      [ "GN_RET_ERR_INVALID_ARG", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91a196e3edc262e215c1bd168e2acb64d5c", null ],
      [ "GN_RET_ERR_LEAF_NOT_STARTED", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91a3d25ca19c9cb8866b27506982668b2db", null ],
      [ "GN_RET_ERR_NODE_NOT_STARTED", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91a70d4b7bdbcac32c06eeb17eaef154458", null ],
      [ "GN_RET_ERR_LEAF_PARAM_ACCESS_VIOLATION", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91a5bb97f3d651f5c8e877c9b6db982f7cb", null ],
      [ "GN_RET_ERR_EVENT_LOOP_ERROR", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91af3b2f67f09b5d81b57117168082ce94d", null ],
      [ "GN_RET_ERR_LEAF_NOT_FOUND", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91a424a42d9cefed0485098a08c0e224ca9", null ],
      [ "GN_RET_ERR_EVENT_NOT_SENT", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91acd16e63b6d49664735eb5e995dc53a68", null ],
      [ "GN_RET_ERR_MQTT_SUBSCRIBE", "gn__commons_8h.html#ac04e0e32194e4ebe5b315c2b6f2a2c91a801b0f353a89b6aac88a652cf06a6c7c", null ]
    ] ],
    [ "gn_leaf_param_access_t", "gn__commons_8h.html#aa932f36b70564f091d5eca91bc6bbbf5", [
      [ "GN_LEAF_PARAM_ACCESS_WRITE", "gn__commons_8h.html#aa932f36b70564f091d5eca91bc6bbbf5a0e483d0e2f392c0a8d20c94d5b9d3eab", null ],
      [ "GN_LEAF_PARAM_ACCESS_READ", "gn__commons_8h.html#aa932f36b70564f091d5eca91bc6bbbf5a3808d89fef84ecbbc927cab32c9fdaf1", null ],
      [ "GN_LEAF_PARAM_ACCESS_READWRITE", "gn__commons_8h.html#aa932f36b70564f091d5eca91bc6bbbf5a72eb8f60cfbb5a1af8dc49a566e107e4", null ]
    ] ],
    [ "gn_leaf_param_storage_t", "gn__commons_8h.html#aa13fd6921158900e01cd337e3e03ae5f", [
      [ "GN_LEAF_PARAM_STORAGE_PERSISTED", "gn__commons_8h.html#aa13fd6921158900e01cd337e3e03ae5faf298a5be940504e78322b7743a380ec8", null ],
      [ "GN_LEAF_PARAM_STORAGE_VOLATILE", "gn__commons_8h.html#aa13fd6921158900e01cd337e3e03ae5faf898f16353b9e5c77214eee6fdf75cab", null ]
    ] ],
    [ "gn_leaf_param_validator_result_t", "gn__commons_8h.html#a3f7ee1da85fdb3e756b682d447ff9458", [
      [ "GN_LEAF_PARAM_VALIDATOR_PASSED", "gn__commons_8h.html#a3f7ee1da85fdb3e756b682d447ff9458a734df3b79a1ca34a1d4820bbcd3749fb", null ],
      [ "GN_LEAF_PARAM_VALIDATOR_ABOVE_MAX", "gn__commons_8h.html#a3f7ee1da85fdb3e756b682d447ff9458a33869539ffff251a818de1c19c5e6956", null ],
      [ "GN_LEAF_PARAM_VALIDATOR_BELOW_MIN", "gn__commons_8h.html#a3f7ee1da85fdb3e756b682d447ff9458ac07f20a25708de521446ba50bf82dc07", null ],
      [ "GN_LEAF_PARAM_VALIDATOR_NOT_ALLOWED", "gn__commons_8h.html#a3f7ee1da85fdb3e756b682d447ff9458ae37d2c802342c6dc247506bf2da8a41d", null ],
      [ "GN_LEAF_PARAM_VALIDATOR_ERROR", "gn__commons_8h.html#a3f7ee1da85fdb3e756b682d447ff9458ab83fa53d67768469f89ce1dfd824f05b", null ]
    ] ],
    [ "gn_leaf_status_t", "gn__commons_8h.html#a6d51f2d997d81965c8c23d1a06c0af67", [
      [ "GN_LEAF_STATUS_NOT_INITIALIZED", "gn__commons_8h.html#a6d51f2d997d81965c8c23d1a06c0af67a119c29854bba8cdcc4675cf082be9051", null ],
      [ "GN_LEAF_STATUS_INITIALIZED", "gn__commons_8h.html#a6d51f2d997d81965c8c23d1a06c0af67af135414ab7efdc3168d63dfd632ed8a5", null ],
      [ "GN_LEAF_STATUS_ERROR", "gn__commons_8h.html#a6d51f2d997d81965c8c23d1a06c0af67a6a005a5fce0e9777d11a97b9d07ca772", null ]
    ] ],
    [ "gn_log_level_t", "gn__commons_8h.html#a3486d4e9b104a1d3ed1e6e6ac04d8da4", [
      [ "GN_LOG_DEBUG", "gn__commons_8h.html#a3486d4e9b104a1d3ed1e6e6ac04d8da4ab240dbff4d5982c985b6cd39eff868f8", null ],
      [ "GN_LOG_INFO", "gn__commons_8h.html#a3486d4e9b104a1d3ed1e6e6ac04d8da4ac84aa31b84a74169956364735146ae6c", null ],
      [ "GN_LOG_WARNING", "gn__commons_8h.html#a3486d4e9b104a1d3ed1e6e6ac04d8da4ae175b66a6244d806ffd51ae0aa8b3b27", null ],
      [ "GN_LOG_ERROR", "gn__commons_8h.html#a3486d4e9b104a1d3ed1e6e6ac04d8da4acfd768282a6f77bc0a034c18b8193de4", null ]
    ] ],
    [ "gn_server_status_t", "gn__commons_8h.html#a3c06b3b4456706647a30680c82e9e539", [
      [ "GN_SERVER_CONNECTED", "gn__commons_8h.html#a3c06b3b4456706647a30680c82e9e539ac3d5c757cc4ee226b95be9d0c618d2a0", null ],
      [ "GN_SERVER_DISCONNECTED", "gn__commons_8h.html#a3c06b3b4456706647a30680c82e9e539a647431bf2bc3cbfe6cae80f58331d7fd", null ]
    ] ],
    [ "gn_val_type_t", "gn__commons_8h.html#ad1014e416cafe9369c2d9f677465cf40", [
      [ "GN_VAL_TYPE_STRING", "gn__commons_8h.html#ad1014e416cafe9369c2d9f677465cf40acaa51bdf262791b1e6ba9d7c0fdb7d2a", null ],
      [ "GN_VAL_TYPE_BOOLEAN", "gn__commons_8h.html#ad1014e416cafe9369c2d9f677465cf40a49561128df0836e9980067737c03ae20", null ],
      [ "GN_VAL_TYPE_DOUBLE", "gn__commons_8h.html#ad1014e416cafe9369c2d9f677465cf40a6bf40bdc47870b47f7d3ef5bf40db497", null ]
    ] ],
    [ "gn_common_hash", "gn__commons_8h.html#a7c6fd08bb10b95a7e9dcfbe5171194bc", null ],
    [ "gn_common_hash_str", "gn__commons_8h.html#a83ff82e21c9118e10f218e2d51047326", null ],
    [ "gn_common_leaf_event_mask_param", "gn__commons_8h.html#ac81ceb87c3e79c545042968279fa9b33", null ]
];