#pragma once

#include <stdint.h>
#include "rf_pa.h"

#define EEPROM_put(idx,T)	eeprom_update_block(idx,(uint8_t*)(&(T)),sizeof(T)) //;&(T)
#define EEPROM_get(idx,T)	eeprom_read_block(idx,(uint8_t*)(&(T)),sizeof(T)) //;&(T)

void eeprom_update_block(uint16_t idx, uint8_t *ptr, uint32_t len);
void eeprom_read_block(const uint16_t idx, uint8_t *ptr, uint32_t len);

int16_t flash_init(void);
void test_flash();

void eeprom_read_paCal(paCalibration_t* cal, uint8_t idx);
void eeprom_write_paCal(paCalibration_t* cal, uint8_t idx);

