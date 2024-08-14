meshtastic --begin-edit
meshtastic --set device.role ROUTER --set device.node_info_broadcast_secs	14400
meshtastic --set-owner '38LBUSLM8Ro_F4HVV_GrandRatz_869.4625' --set-owner-short 702f
meshtastic --set lora.region EU_868 --set lora.modem_preset LONG_MODERATE --set lora.tx_power 27 --set lora.ignore_mqtt true --set lora.sx126x_rx_boosted_gain true --set lora.override_frequency 869.4625
meshtastic --seturl https://meshtastic.org/e/#ChYSAQEaCUZyX0JhbGlzZSgBMAE6AggQChESAQEaCEZyX0VNQ09NKAEwAQoSEgEBGglGcl9CbGFCbGEoATABChASAQEaB0ZyX1RlY2goATABCg4SAQEaBWFkbWluKAEwARIYCAEQBzgDQAdIAVAbWAJoAXWaXVlEwAYB
meshtastic --set position.fixed_position true --setlat 45.325776 --setlon 5.63658087 --setalt 830 --pos-fields ALTITUDE ALTITUDE_MSL --set position.position_broadcast_secs 14400
meshtastic --set power.on_battery_shutdown_after_secs	14400
meshtastic --set neighbor_info.enabled false --set neighbor_info.update_interval 14400
meshtastic --set telemetry.device_update_interval 14400
meshtastic --set serial.txd 8 --set serial.rxd 9 --set serial.mode PROTO --set serial.enabled true --set serial.baud BAUD_115200
meshtastic --set device.ledHeartbeatDisabled true
meshtastic --commit-edit
