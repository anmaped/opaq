/*
Copyright 2013 Nusku Networks

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

#include "Arduino.h"
#include <Wire.h>

#ifdef TwoWire_h // Esure this code only gets built if you have Wire.h included in the main sketch

#include "SHA204.h"
#include "SHA204ReturnCodes.h"
#include "SHA204Definitions.h"
#include "SHA204I2C.h"

uint16_t SHA204I2C::SHA204_RESPONSE_TIMEOUT() {
	return SHA204_RESPONSE_TIMEOUT_VALUE;
}

SHA204I2C::SHA204I2C() {
	address = ((uint8_t) 0x64);
}

SHA204I2C::SHA204I2C(uint8_t deviceAddress) {
	address = deviceAddress;
}

void SHA204I2C::init() {
	chip_wakeup();
}

uint8_t SHA204I2C::receive_bytes(uint8_t count, uint8_t *data) {
	Serial.println("receive_bytes(uint8_t count, uint8_t *data)");
	uint8_t i;

	int available_bytes = Wire.requestFrom(deviceAddress(), count);
	if (available_bytes != count) {
		return I2C_FUNCTION_RETCODE_COMM_FAIL;
	}

	for (i = 0; i < count; i++) {
		while (!Wire.available()); // Wait for byte that is going to be read next
		*data++ = Wire.read(); // Store read value
	}

	return I2C_FUNCTION_RETCODE_SUCCESS;
}

uint8_t SHA204I2C::receive_byte(uint8_t *data) {
	Serial.println("receive_byte");

	int available_bytes = Wire.requestFrom(deviceAddress(), (uint8_t)1);
	if (available_bytes != 1) {
		return I2C_FUNCTION_RETCODE_COMM_FAIL;
	}
	while (!Wire.available()); // Wait for byte that is going to be read next
	*data++ = Wire.read(); // Store read value

	return I2C_FUNCTION_RETCODE_SUCCESS;
}

uint8_t SHA204I2C::send_byte(uint8_t value) {
	Serial.println("send_byte(uint8_t value)");
	return send_bytes(1, &value);
}

uint8_t SHA204I2C::send_bytes(uint8_t count, uint8_t *data) {
	int sent_bytes = Wire.write(data, count);

	if (count > 0 && sent_bytes == count) {
		return I2C_FUNCTION_RETCODE_SUCCESS;
	}

	return I2C_FUNCTION_RETCODE_COMM_FAIL;
}

int SHA204I2C::start_operation(uint8_t readWrite) {
	Serial.println("start_operation(uint8_t readWrite)");
	int written = Wire.write(&readWrite, (uint8_t)1);
	
	return written > 0;
}

uint8_t SHA204I2C::chip_wakeup() {
	Serial.println("chip_wakeup()");
	// This was the only way short of manually adjusting the SDA pin to wake up the device
	Wire.beginTransmission(deviceAddress());
	int i2c_status = Wire.endTransmission();
	if (i2c_status != 0) {
		return SHA204_COMM_FAIL;
	}

	return SHA204_SUCCESS;
}

uint8_t SHA204I2C::receive_response(uint8_t size, uint8_t *response) {
	Serial.println("receive_response(uint8_t size, uint8_t *response)");
	uint8_t count;
	uint8_t i2c_status;

	// Wire.beginTransmission(deviceAddress());
	// uint8_t sla = deviceAddress();
	// Wire.write(&sla, (uint8_t)1);
	// int status = Wire.endTransmission();
	// Serial.println(status);
	// Serial.println("receive_response -- after wire.endtransmission");

	// Receive count byte.
	i2c_status = receive_byte(response);
	if (i2c_status != I2C_FUNCTION_RETCODE_SUCCESS) {
		Serial.println("receive_response -- fail 1");
		return SHA204_COMM_FAIL;
	}

	count = response[SHA204_BUFFER_POS_COUNT];
	if ((count < SHA204_RSP_SIZE_MIN) || (count > size)) {
		Serial.println("receive_response -- fail 2");
		return SHA204_INVALID_SIZE;
	}

	i2c_status = receive_bytes(count - 1, &response[SHA204_BUFFER_POS_DATA]);

	if (i2c_status != I2C_FUNCTION_RETCODE_SUCCESS) {
		Serial.println("receive_response -- fail 3");
		return SHA204_COMM_FAIL;
	}
	
	return SHA204_SUCCESS;
}

uint8_t SHA204I2C::send(uint8_t word_address, uint8_t count, uint8_t *buffer) {
	Serial.println("send(uint8_t word_address, uint8_t count, uint8_t *buffer)");
	uint8_t i2c_status;

	Wire.beginTransmission(deviceAddress());

	start_operation(I2C_WRITE);

	i2c_status = send_bytes(1, &word_address);
	if (i2c_status != I2C_FUNCTION_RETCODE_SUCCESS) {
		Serial.println("send -- fail 1");
		return SHA204_COMM_FAIL;
	}

	if (count == 0) {
		return SHA204_SUCCESS;
	}

	i2c_status = send_bytes(count, buffer);

	if (i2c_status != I2C_FUNCTION_RETCODE_SUCCESS) {
		Serial.println("send -- fail 2");
		return SHA204_COMM_FAIL;
	}

	Wire.endTransmission();

	return SHA204_SUCCESS;
}


uint8_t SHA204I2C::send_command(uint8_t count, uint8_t *command) {
	Serial.println("send_command(uint8_t count, uint8_t *command)");
	return send(SHA204_I2C_PACKET_FUNCTION_NORMAL, count, command);
}

uint8_t SHA204I2C::sleep(void) {
	Serial.println("sleep(void)");
	return send(SHA204_I2C_PACKET_FUNCTION_SLEEP, 0, NULL);
}

uint8_t SHA204I2C::resync(uint8_t size, uint8_t *response) {
	Serial.println("resync(uint8_t size, uint8_t *response)");

	// Try to re-synchronize without sending a Wake token
	// (step 1 of the re-synchronization process).
	uint8_t nine_clocks = 0xFF;
	send_bytes(1, &nine_clocks);
	Wire.beginTransmission(deviceAddress());
	Wire.endTransmission();

	// Try to send a Reset IO command if re-sync succeeded.
	int ret_code = reset_io();

	if (ret_code == SHA204_SUCCESS) {
		return ret_code;
	}

	// We lost communication. Send a Wake pulse and try
	// to receive a response (steps 2 and 3 of the
	// re-synchronization process).
	sleep();
	ret_code = wakeup(response);

	// Translate a return value of success into one
	// that indicates that the device had to be woken up
	// and might have lost its TempKey.
	return (ret_code == SHA204_SUCCESS ? SHA204_RESYNC_WITH_WAKEUP : ret_code);
}

uint8_t SHA204I2C::reset_io() {
	Serial.println("reset_io()");
	return send(SHA204_I2C_PACKET_FUNCTION_RESET, 0, NULL);
}

#endif
