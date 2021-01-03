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

#include "Events.h"

// -----------------------------------------------------------------------------

EventGroupHandler::EventGroupHandler(void)
{
    xEvents = NULL;
}

EventGroupHandler::~EventGroupHandler(void)
{
    Destroy();
}

bool EventGroupHandler::Create(void)
{
    if (xEvents != NULL)
        return true;

    xEvents = xEventGroupCreate();
    if (xEvents == NULL)
        return false;

    return true;
}

void EventGroupHandler::Destroy(void)
{
    if (xEvents != NULL) {
        vEventGroupDelete(xEvents);
        xEvents = NULL;
    }
}

void EventGroupHandler::ClearBits(EventBits_t eventBits)
{
    if (xEvents != NULL)
        xEventGroupClearBits(xEvents, eventBits);
}

EventBits_t EventGroupHandler::GetBits(void)
{
    if (xEvents == NULL)
        return (EventBits_t)0;

    return xEventGroupGetBits(xEvents);
}

void EventGroupHandler::SetBits(EventBits_t eventBits)
{
    if (xEvents != NULL)
        xEventGroupSetBits(xEvents, eventBits);
}

EventBits_t EventGroupHandler::WaitForAnyBit(EventBits_t eventBits, uint32_t ms)
{
    if (xEvents == NULL)
        return (EventBits_t)0;

    TickType_t ticksToWait = ms / portTICK_PERIOD_MS;
    EventBits_t bits = xEventGroupWaitBits(xEvents, eventBits, pdTRUE, pdFALSE, ticksToWait);
    return bits;
}

EventBits_t EventGroupHandler::WaitForAllBits(EventBits_t eventBits, uint32_t ms)
{
    if (xEvents == NULL)
        return (EventBits_t)0;

    TickType_t ticksToWait = ms / portTICK_PERIOD_MS;
    EventBits_t bits = xEventGroupWaitBits(xEvents, eventBits, pdTRUE, pdTRUE, ticksToWait);
    return bits;
}
