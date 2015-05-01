/*
 * libibb.h:
 * - use the expansion board DISP-SW for the raspberry pi.
 *
 * Copyright (c) 2015 Dennis Binder.
 ***********************************************************************
 * This file is part of libibb:
 *
 *    ibb is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ibb is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */


#ifndef _IBB_H
#define _IBB_H



typedef unsigned char  UINT8;
typedef char           INT8;
typedef int            INT32;
typedef short          INT16;
typedef unsigned short UINT16;

// --------------------------------------------------------
// defines the encoder actions
#define ENCODER_FOREWARD      0x01
#define ENCODER_BACKWARD      0x02
#define ENCODER_NOTHING       0
// --------------------------------------------------------

// --------------------------------------------------------
// defines the thread refresh rate [ms]
#define THREADCYCLE     2   
// --------------------------------------------------------

// --------------------------------------------------------
// defines the flash rate [ms]
#define FLASHCYCLE      500   
// --------------------------------------------------------

// --------------------------------------------------------
// public functions
extern void  ibb_DisplayStart (void);
extern void  ibb_DisplayStop  (void);
extern void  ibb_DisplaySet   (int Z1, int Z2, int Z3, int Z4);
extern void  ibb_Flashing     (unsigned char flash1, unsigned char flash2, 
					           unsigned char flash3, unsigned char flash4);
extern UINT8 ibb_GetEncoder   (void);
extern UINT8 ibb_GetSwitch    (void);
// --------------------------------------------------------






#endif
