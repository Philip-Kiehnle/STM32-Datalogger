/*
 * output_timer.h
 *
 *  Created on: April 6, 2026
 *      Author: philip
 */

#include <stdbool.h>

void update_lock_incr(uint16_t relay_idx);
void output_timer_step();
void output_set(uint16_t relay_idx, bool bOn);
void output_timer_set(uint8_t relay_idx, int seconds);
uint32_t output_timer_get(uint8_t relay_idx);
