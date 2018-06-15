/*
Copyright 2018 Tony kao
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/// <summary>
/// VERY rudimentary and hacky example of a BLE peripheral.
/// Broadcasts and accepts connection, has a single service
/// (Device Information Service profile) with a single characteristic
/// (Manufacturer Name String).
/// 
/// As much of the setup is done at compile time as possible using
/// C++ templates, with data structures allocated statically.
/// </summary>

#include <nvs_flash.h>
#include <cstring>
#include "HCIPacket.h"

#include "RingBufCPP.h"

// queue for send_packet calls
RingBufCPP<void(*)(), 64> ble_cb;

void setup() {
	pinMode(BUILTIN_LED, OUTPUT);
	Serial.begin(921600);
	nvs_flash_init();

	static const esp_vhci_host_callback_t cb = {
		// notify_host_send_available
		[]() {
		},
		// notify_host_recv
		[](uint8_t *data, uint16_t len) -> int {
			if (data[0] == HCIT_TYPE_EVENT) {
				const auto _event = *(EventHeader *)data;

				switch (_event.packet.eventcode) {
				case HCI_DISCONNECTION_COMP_EVT:
					//adv_enable_cmd.send_packet();
					ble_cb.add([]() {
						adv_enable_cmd.send_packet();
					});
					break;
				default:
					break;
				}
			}
			else if (data[0] == HCIT_TYPE_ACL_DATA) {
				auto header = *(ATTHeader *)(data);
				switch (header.packet.data.payload.opcode)
				{
				case GATT_REQ_MTU:
					Serial.println("GATT_REQ_MTU");
					ble_cb.add([]() {
						mtu_response.send_packet();
					});
					break;
				case GATT_REQ_READ_BY_GRP_TYPE: {
					Serial.println("GATT_REQ_READ_BY_GRP_TYPE");
					auto req = *(TransportPacket<ACLDataPacket<L2CAPPacket<ATTPacket<ReadByGrpReq>>>> *)(data);
					Serial.println();
					if (req.packet.data.payload.parameters.start_handle > 0x0002) {
						ble_cb.add([]() {
							static auto err = error_response;
							err.packet.data.payload.parameters.req_opcode = GATT_REQ_READ_BY_GRP_TYPE;
							err.packet.data.payload.parameters.req_handle = 0x0003;
							err.packet.data.payload.parameters.error_code = GATT_NOT_FOUND;
							err.send_packet();
						});
					}
					static TransportPacket<ACLDataPacket<L2CAPPacket<ATTPacket<ReadByGrpResponse>>>> response;
					response.hci_packet_type = HCIT_TYPE_ACL_DATA;
					response.packet.data.payload.opcode = GATT_RSP_READ_BY_GRP_TYPE;
					response.packet.data.payload.parameters.data = { 0x0001, 0x0003, 0x180a };
					ble_cb.add([]() {
						response.send_packet();
					});
					break;
				}
				case GATT_REQ_FIND_TYPE_VALUE:
					Serial.println("GATT_REQ_FIND_TYPE_VALUE");
					break;
				case GATT_REQ_READ_BY_TYPE: {
					Serial.println("GATT_REQ_READ_BY_TYPE");
					auto req = *(TransportPacket<ACLDataPacket<L2CAPPacket<ATTPacket<ReadByTypeReq>>>> *)(data);
					Serial.println(req.packet.data.payload.parameters.start_handle, HEX);
					Serial.println(req.packet.data.payload.parameters.end_handle, HEX);
					Serial.println(req.packet.data.payload.parameters.uuid16, HEX);

					if (req.packet.data.payload.parameters.uuid16 == 0x2803 && req.packet.data.payload.parameters.start_handle <= 0x0002)
					{
						ble_cb.add([]() {
							static TransportPacket<ACLDataPacket<L2CAPPacket<ATTPacket<ReadByTypeResponse<Characteristic>>>>> response;
							response.hci_packet_type = HCIT_TYPE_ACL_DATA;
							response.packet.data.payload.opcode = GATT_RSP_READ_BY_TYPE;
							response.packet.data.payload.parameters.data = { 0x0002, 0x02, 0x0003, 0x2A29 };
							response.send_packet();
						});
					}
					else {
						ble_cb.add([]() {
							static auto err = error_response;
							err.packet.data.payload.parameters.req_opcode = GATT_REQ_READ_BY_GRP_TYPE;
							err.packet.data.payload.parameters.req_handle = 0x0003;
							err.packet.data.payload.parameters.error_code = GATT_NOT_FOUND;
							err.send_packet();
						});

					}

					break;
				}
				case GATT_REQ_FIND_INFO:
					Serial.println("GATT_REQ_FIND_INFO");
					break;
				case GATT_REQ_READ: {
					struct {
						char name[100] = "CompanyName";
					} name;
					static TransportPacket<ACLDataPacket<L2CAPPacket<ATTPacket<ReadByTypeResponse<decltype(name)>>>>> response;
					response.hci_packet_type = HCIT_TYPE_ACL_DATA;
					response.packet.data.payload.opcode = GATT_RSP_READ;
					response.packet.data.payload.parameters.data = name;
					response.send_packet();
					break;
				}
				default:
					Serial.print("GATT_REQ_UNSUPPORTED: ");
					Serial.println(header.packet.data.payload.opcode, HEX);
					error_response.packet.data.payload.parameters.req_opcode = header.packet.data.payload.opcode;
					//error_response.send_packet();
					ble_cb.add([]() {
						error_response.send_packet();
					});
					break;
				}
			}
			return 0;
	} };
	esp_vhci_host_register_callback(&cb);

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT()
		;

	esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
	esp_bt_controller_init(&bt_cfg);
	esp_bt_controller_enable(ESP_BT_MODE_BLE);

	reset_cmd.send_packet();

	adv_param_cmd.send_packet();
	//adv_data_cmd.packet.parameters.append_gap_data(0xff, "\xff\xfftestdata");
	adv_data_cmd.packet.parameters.append_gap_data(0x09, "BLE_test");
	adv_data_cmd.send_packet();
	adv_enable_cmd.send_packet();
	write_data_length_cmd.send_packet();
}

void loop() {
	
	/// Send packets outside of the VHCI callbacks
	/// since they run on CPU0 and will hang forever
	/// waiting for the previous packet to finish.
	while (!ble_cb.isEmpty()) {
		void(*p)(void);
		ble_cb.pull(&p);
		p();
	}
}
