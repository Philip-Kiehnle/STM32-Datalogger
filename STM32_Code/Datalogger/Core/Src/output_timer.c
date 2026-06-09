/*
 * output_timer.c
 *
 *  Created on: April 6, 2026
 *      Author: philip
 */


#include "stm32g4xx_ll_gpio.h"

#include "main.h"
#include "output_timer.h"


//*****************************************************************************
//
// The timebase for rate limit of relay switching.
//
//*****************************************************************************
volatile unsigned long relative_time_sec = 0;

#define NUM_TIMER_RELAYS 1

const uint16_t MIN_LOCK_TIME_SEC = 1;
const uint16_t MAX_LOCK_TIME_SEC = 10*60;  // 10min


typedef struct {
    bool     state;
    uint64_t last_action_time;
    uint32_t remaining_time_sec;
    uint32_t lock_time_sec;
    uint16_t lock_time_incr;
} relay_t;

volatile relay_t relay[NUM_TIMER_RELAYS] = {
    [0 ... NUM_TIMER_RELAYS-1] = {
        .lock_time_incr = MIN_LOCK_TIME_SEC  // prevent fast toggling
    }
};


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

uint32_t clamp(uint32_t value, uint32_t lower, uint32_t upper)
{
    return MAX(lower, MIN(value, upper));
}


void update_lock_incr(uint16_t relay_idx)
{
    uint32_t time_between_ON_actions = relative_time_sec - relay[relay_idx].last_action_time;
    if ( time_between_ON_actions < 5) {  // 5sec
        relay[relay_idx].lock_time_incr += 3;
    } else if ( time_between_ON_actions < 10) {  // 10sec
        relay[relay_idx].lock_time_incr += 2;
    } else if ( time_between_ON_actions < 60) {  // 60sec
        relay[relay_idx].lock_time_incr += 1;
    } else if ( time_between_ON_actions > (30*60)) {  // 30min
        relay[relay_idx].lock_time_incr = MIN_LOCK_TIME_SEC;
    }

    relay[relay_idx].lock_time_incr = clamp(relay[relay_idx].lock_time_incr, MIN_LOCK_TIME_SEC, MAX_LOCK_TIME_SEC);
    DBG_PRINTF("RL%d lock_time_incr=%d\n", relay_idx+1, relay[relay_idx].lock_time_incr);
}


void on_check(uint16_t relay_idx)
{
    if (   !relay[relay_idx].state
		&& relay[relay_idx].lock_time_sec == 0
    ) {
        output_set(relay_idx, true);
        update_lock_incr(relay_idx);
        relay[relay_idx].lock_time_sec = relay[relay_idx].lock_time_incr;
        relay[relay_idx].last_action_time = relative_time_sec;
    }
}


//*****************************************************************************
//
// Step function for relay control. Called every second.
//
//*****************************************************************************
void output_timer_step()
{
    relative_time_sec++;

    for (uint16_t relay_idx=0; relay_idx<NUM_TIMER_RELAYS; relay_idx++) {

        if (relay[relay_idx].lock_time_sec > 0) {
            relay[relay_idx].lock_time_sec--;
        }
        if (relay[relay_idx].remaining_time_sec > 0) {
        	relay[relay_idx].remaining_time_sec--;
            on_check(relay_idx);

        } else if (relay[relay_idx].state) {
            output_set(relay_idx, false);
        }
    }
}

//*****************************************************************************
//
// Set the output on or off.
//
//*****************************************************************************
void output_set(uint16_t relay_idx, bool bOn)
{
    DBG_PRINTF("output_set");
    relay[relay_idx].state = bOn;
    if (relay_idx == 0) {
        DBG_PRINTF(" nr 1 ");
        DBG_PRINTF((bOn) ? "ON" : "OFF");
        if (bOn) {
        	GPIOA->BRR = (1<<12);  // PA12=0 -> relay active
        } else {
        	GPIOA->BSRR = (1<<12);  // PA12=1 -> relay inactive
        }
    }
    DBG_PRINTF("\n");
}


//*****************************************************************************
//
// Set the timer
//
//*****************************************************************************
void output_timer_set(uint8_t relay_idx, int seconds)
{
    if (relay_idx < NUM_TIMER_RELAYS) {

        if ( relay[relay_idx].remaining_time_sec > 0 || relay[relay_idx].lock_time_sec == 0 ) {  // relay already on or not locked for new action
        	relay[relay_idx].remaining_time_sec = seconds;
        }

        if (relay[relay_idx].remaining_time_sec > 0) {
            on_check(relay_idx);

        } else if (relay[relay_idx].state) {
            output_set(relay_idx, false);
        }
    }
}


//*****************************************************************************
//
// Get the timer
//
//*****************************************************************************
uint32_t output_timer_get(uint8_t relay_idx)
{
    if (relay_idx < NUM_TIMER_RELAYS) {
        return relay[relay_idx].remaining_time_sec;
    }
    return 0;
}

