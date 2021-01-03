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

#ifndef BoardEvents_H
#define BoardEvents_H

#include "Events.h"

const EventBits_t xBitStaConnected        = ( 1 << 0 );
const EventBits_t xBitStaDisconnected     = ( 1 << 1 );
const EventBits_t xBitScanDone            = ( 1 << 2 );
const EventBits_t xBitAPStarted           = ( 1 << 3 );
const EventBits_t xBitAPStopped           = ( 1 << 4 );

/**
 * EventBits_t is 16 or 32 bits long.
 * See it's documentation for more information.
 */
const EventBits_t xBitALL = 0xFFFF;

#endif
