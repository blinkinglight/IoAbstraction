/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <EepromAbstractionWire.h>
#include <Wire.h>

I2cAt24Eeprom::I2cAt24Eeprom(uint8_t address, uint8_t pageSize) {
	this->eepromAddr = address;
	this->pageSize = pageSize;
}

void writeAddressWire(uint8_t deviceAddr, uint16_t memAddr) {
	Wire.write(memAddr >> 8);
	Wire.write(memAddr & 0xff);
}

void waitForReady(uint8_t eeprom) {
	do {
		Wire.beginTransmission(eeprom);
	} while(Wire.endTransmission() != 0);
}

uint8_t I2cAt24Eeprom::readByte(EepromPosition position) {
	Wire.beginTransmission(eepromAddr);
	writeAddressWire(eepromAddr, position);
	Wire.endTransmission();

	uint8_t ret = 0;
	Wire.requestFrom(eepromAddr, (uint8_t)1, (uint8_t)true);
	if(Wire.available()) ret = (uint8_t)Wire.read();
	return ret;
} 

uint8_t I2cAt24Eeprom::read8(EepromPosition position) {
	waitForReady(eepromAddr);
    return readByte(position);
}

void I2cAt24Eeprom::write8(EepromPosition position, uint8_t val) {
	if(read8(position) == val) return;
	writeByte(position, val);
}

void I2cAt24Eeprom::writeByte(EepromPosition position, uint8_t val) {
	// before ANY write that is going to be committed (by ending i2c send)
	// then we must first wait for the device to be ready. In case a previous
	// write has not yet completed.
	waitForReady(eepromAddr);

	Wire.beginTransmission(eepromAddr);
	writeAddressWire(eepromAddr, position);
	Wire.write(val);
	Wire.endTransmission();
}

uint16_t I2cAt24Eeprom::read16(EepromPosition position) {
	waitForReady(eepromAddr);

	uint16_t ret = ((uint16_t)readByte(position++) << 8);
	ret |= readByte(position);
	return ret;
}

void I2cAt24Eeprom::write16(EepromPosition position, uint16_t val) {
	if(read16(position) == val) return;

	writeByte(position++, (val >> 8) & 0xff);
	writeByte(position, val & 0xff);
}

uint32_t I2cAt24Eeprom::read32(EepromPosition position) {
	waitForReady(eepromAddr);

	uint32_t ret = ((uint32_t)readByte(position++) << 24);
	ret |= ((uint32_t)readByte(position++) << 16);
	ret |= ((uint32_t)readByte(position++) << 8);
	ret |= (readByte(position));
	return ret;
}

void I2cAt24Eeprom::write32(EepromPosition position, uint32_t val) {
	if(read32(position) == val) return;

	writeByte(position++, val >> 24);
	writeByte(position++, (val >> 16) & 0xff);
	writeByte(position++, (val >> 8) & 0xff);
	writeByte(position, val & 0xff);
}

uint8_t I2cAt24Eeprom::findMaximumInPage(uint16_t destEeprom, uint8_t len) {
	// We can read/write in bulk, but do no exceed the page size or we will read / write
	// the wrong bytes
	int offs = destEeprom % pageSize;
	int currentGo = min(pageSize, offs + len) - offs;

	// dont exceed the buffer length of the  wire library
	return min(currentGo, 32);
}

void I2cAt24Eeprom::readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len) {
	int romOffset = 0;
	while(len) {
		waitForReady(eepromAddr);
		uint8_t currentGo = findMaximumInPage(romSrc + romOffset, len);

		Wire.beginTransmission(eepromAddr);
		writeAddressWire(eepromAddr, romSrc + romOffset);
		Wire.endTransmission();

		Wire.requestFrom(eepromAddr, (uint8_t)currentGo, (uint8_t)1);
		while(len && Wire.available()) {
			memDest[romOffset] = (uint8_t)Wire.read();
			--len;
			++romOffset;
		}
	}
}

void I2cAt24Eeprom::writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len) {
	int romOffset = 0;
	while(len > 0) {
		waitForReady(eepromAddr);
		int currentGo = findMaximumInPage(romDest + romOffset, len);
		Wire.beginTransmission(eepromAddr);
		writeAddressWire(eepromAddr, romDest + romOffset);
		while(currentGo) {
			Wire.write(memSrc[romOffset]);
			--currentGo;
			--len;
			++romOffset;
		}
		Wire.endTransmission();
	}
}
