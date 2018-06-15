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

#pragma once
#include <esp_bt.h>
#include <hcidefs.h>
#include <hcimsgs.h>
#include <gatt_api.h>
#include <initializer_list>
#include <type_traits>
#include <functional>
#pragma pack(1)

// Packet structure types

template<typename T> struct TransportPacket;

template<typename T>
struct TransportPacket {
	uint8_t hci_packet_type;
	T packet;
	
	void send_packet() {
		while (!esp_vhci_host_check_send_available());
		esp_vhci_host_send_packet((uint8_t*)this, sizeof(T) + 1);
		for (int i = 0; i < sizeof(T) + 1; i++) {
			Serial.print(((uint8_t*)this)[i], HEX);
			Serial.print(" ");
		}
		Serial.println();
	}

	//static_assert(std::is_standard_layout<T>::value, "packet struct not standard layout");
	//static_assert(alignof(T) == 1, "packet misaligned");
};

template<typename... T>
struct CommandPacket;

template<typename T, typename... U>
struct CommandPacket<T, U...> {
	uint16_t opcode;
	uint8_t len;
	T parameters;

	// Constructors
	CommandPacket(uint16_t _opcode, uint16_t _len, T _parameters) {
		opcode = _opcode;
		len = sizeof(T);
		parameters = _parameters;
	}
	CommandPacket(uint16_t _opcode, uint16_t _len, std::initializer_list<uint16_t> l) {
		opcode = _opcode;
		len = sizeof(T);
		auto begin = l.begin();
		for (auto&i : parameters) {
			i = *begin;
			begin++;
		}
	}
	CommandPacket(uint16_t _opcode, uint16_t _len) : CommandPacket(_opcode) { }
	CommandPacket(uint16_t _opcode) {
		opcode = _opcode;
		len = sizeof(T);
		parameters = T();
	}
};

template<>
struct CommandPacket<> {
	uint16_t opcode;
	uint8_t len;
};

// HCI Command Paramter Types

struct AdvParams {
	uint16_t interval_min = 0x0100;
	uint16_t interval_max = 0x0200;
	uint8_t adv_type = 0x00;
	uint8_t own_addr_type = 0;
	uint8_t peer_addr_type = 0;
	uint8_t peer_addr[6] = {};
	uint8_t channel_map = 0x03;
	uint8_t filter_policy = 0;
};

struct AdvData {
	uint8_t data_len = 0;
	char data[31] = {};

	void append_gap_data(const char gap_type, const char gap_data[]) {
		data_len += strlen(gap_data) + 2;
		data[strlen(data)] = strlen(gap_data) + 1;
		data[strlen(data)] = gap_type;
		strcat(data, gap_data);
	}
};

// ACL Data Packet Types

template<typename T>
struct ACLDataPacket {
	struct {
		uint16_t handle : 12;
		uint16_t pb : 2;
		uint16_t bc : 2;
	};
	uint16_t len;
	T data;

	ACLDataPacket(uint16_t _handle, uint16_t _pb, uint16_t _bc, uint16_t _len, T _data) {
		handle = _handle;
		pb = _pb;
		bc = _bc;
		len = sizeof(T);
		data = _data;
	}
	ACLDataPacket() {
		len = sizeof(T);
	}
};

template<typename T>
struct L2CAPPacket {
	uint16_t len;
	uint16_t ch_id;
	T payload;

	L2CAPPacket(uint16_t _len, uint16_t _ch_id, T _payload) {
		len = sizeof(T);
		ch_id = _ch_id;
		payload = _payload;
	}
	L2CAPPacket() {
		len = sizeof(T);
		ch_id = 0x0004;
	}
};

template<typename T>
struct ATTPacket {
	uint8_t opcode;
	T parameters;
};

struct ReadByGrpReq {
	uint16_t start_handle;
	uint16_t end_handle;
	union {
		uint16_t uuid16;
		uint8_t uuid128[16];
	};
};

struct ReadByTypeReq {
	uint16_t start_handle;
	uint16_t end_handle;
	union {
		uint16_t uuid16;
		uint8_t uuid128[16];
	};
};

struct ReadReq {
	uint16_t handle;
};

struct FindTypeValueReq {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t uuid;
	uint16_t service_uuid;
};

struct FindInfoReq {
	uint16_t start_handle;
	uint16_t end_handle;
};


struct PrimaryServiceGroup {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t uuid;
};

struct Characteristic {
	uint16_t handle;
	uint8_t property;
	uint16_t value_handle;
	uint16_t uuid;
};
struct HandlesInfo {
	uint16_t start_handle;
	uint16_t end_handle;
};

struct InfoData16 {
	uint16_t handle;
	uint16_t uuid;
};

struct InfoData128 {
	uint16_t handle;
	uint8_t uuid[16];
};


struct ReadByGrpResponse {

	uint8_t len = 6;
	PrimaryServiceGroup data;
};

template<typename T>
struct ReadByTypeResponse {
	uint8_t len = sizeof(T);
	T data;
};

struct FindTypeValueResponse {
	HandlesInfo list;
};

struct FindInfoResponse {
	uint8_t format;
	InfoData16 data;
};

struct ErrorResponse {
	uint8_t req_opcode;
	uint16_t req_handle;
	uint8_t error_code;
};

struct ServiceAttribute {
	uint16_t attr_handle;
	uint16_t uuid;
	uint16_t service_uuid;
};
template<typename T>
struct EventPacket {
	uint8_t eventcode;
	uint8_t len;
	T parameters;
};

struct CommandCompleteParams {
	uint8_t num_cmds;
	uint16_t opcode;
	uint8_t status;
};

// Packet instantiations

TransportPacket< CommandPacket<> > reset_cmd = TransportPacket< CommandPacket<> >{
	HCIT_TYPE_COMMAND,
	HCI_RESET,
	0
};


TransportPacket< CommandPacket<AdvParams> > adv_param_cmd = {
	HCIT_TYPE_COMMAND,
	{
		HCI_BLE_WRITE_ADV_PARAMS,
		HCIC_PARAM_SIZE_BLE_WRITE_ADV_PARAMS,
		AdvParams()
	}
};

TransportPacket< CommandPacket<AdvData> > adv_data_cmd = {
	HCIT_TYPE_COMMAND,
	{
		HCI_BLE_WRITE_ADV_DATA,
		HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA + 1
	}
};

TransportPacket< CommandPacket<uint8_t> > adv_enable_cmd = {
	HCIT_TYPE_COMMAND,
	{
		HCI_BLE_WRITE_ADV_ENABLE,
		HCIC_PARAM_SIZE_WRITE_ADV_ENABLE,
		1 // en = 1
	}
};

TransportPacket< CommandPacket<uint16_t[2]> > write_data_length_cmd = {
	HCIT_TYPE_COMMAND,
	{
		HCI_BLE_WRITE_DEFAULT_DATA_LENGTH,
		4,
		{
			0x00FB, // tx size
			0x0148 // tx time
		}
	}
};

// Data Packet Instances
using ATTHeader = TransportPacket<ACLDataPacket< L2CAPPacket< ATTPacket<uint8_t> > > > ;
using FindInformationReqPacket = TransportPacket<ACLDataPacket< L2CAPPacket< ATTPacket<FindInfoReq> > > > ;
TransportPacket<ACLDataPacket< L2CAPPacket< ATTPacket<ErrorResponse> > > > error_response = {
	HCIT_TYPE_ACL_DATA,
	{
		0, // handle
		0, // pb
		0, // bc
		sizeof(L2CAPPacket< ATTPacket<ErrorResponse> >),
		{
			sizeof(ATTPacket<ErrorResponse>),
			0x0004,
			{
				GATT_RSP_ERROR,
				0,
				0,
				GATT_REQ_NOT_SUPPORTED
			}
		}
	}
};

TransportPacket<ACLDataPacket< L2CAPPacket< ATTPacket<uint16_t> > > > mtu_response = {
	HCIT_TYPE_ACL_DATA,
	{
		0, // handle
		0, // pb
		0, // bc
		sizeof(L2CAPPacket< ATTPacket<ErrorResponse> >),
		{
			sizeof(ATTPacket<ErrorResponse>),
			0x0004,
			{
				GATT_RSP_MTU,
				517
			}
		}
	}
};

using CommandCompleteEvent = TransportPacket< EventPacket<CommandCompleteParams> > ;
using EventHeader = TransportPacket< EventPacket<uint8_t> >  ;

#pragma pack()
