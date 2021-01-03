/**
This file is part of ESP32BoardManager esp-idf component
(https://github.com/CalinRadoni/ESP32BoardManager)
Copyright (C) 2019 by Calin Radoni

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EventGroupHandler_H
#define EventGroupHandler_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

class EventGroupHandler
{
public:
    EventGroupHandler(void);
    virtual ~EventGroupHandler(void);

    /**
     * @brief Create the event group
     *
     * This function creates a new event group.
     * Required memory is automatically dynamically allocated.
     *
     * Call the Destroy function to free memory and destroy the event group.
     */
    bool Create(void);

    /**
     * @brief Destroy the event group
     *
     * Destroys the event group and frees the allocated memory.
     */
    void Destroy(void);

    /**
     * @brief Clear event bits
     *
     * @attention This function should NOT be called from an interrupt !
     */
    void ClearBits(EventBits_t);

    /**
     * @brief Get event bits
     *
     * @attention This function should NOT be called from an interrupt !
     */
    EventBits_t GetBits(void);

    /**
     * @brief Set event bits
     *
     * @attention This function should NOT be called from an interrupt !
     */
    void SetBits(EventBits_t);

    /**
     * @brief Wait for any bit
     *
     * Waits maximum `ms` miliseconds for any bit to be set and clears them before returning.
     *
     * Returns (EventBits_t)0 if the event group is not created.
     *
     * Returns the value of the event group at the time either the bits being waited for became set,
     * or the block time expired.
     *
     * @attention This function should NOT be called from an interrupt !
     */
    EventBits_t WaitForAnyBit(EventBits_t eventBits, uint32_t ms);

    /**
     * @brief Wait for all bits
     *
     * Waits maximum `ms` miliseconds  for all bits to be set and clears them before returning.
     *
     * Returns (EventBits_t)0 if the event group is not created.
     *
     * Returns the value of the event group at the time either the bits being waited for became set,
     * or the block time expired.
     *
     * @attention This function should NOT be called from an interrupt !
     */
    EventBits_t WaitForAllBits(EventBits_t eventBits, uint32_t ms);

private:

    EventGroupHandle_t xEvents;
};

#endif
