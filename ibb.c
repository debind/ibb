/*
 * libibb.c:
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

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "stdlib.h"

#include <wiringPi.h>

#include "ibb.h"


//******************************************************************************
// DEFINES
//******************************************************************************


// --------------------------------------------------------
// defines the directions of the encoder and the switch
#define ENC_FOREWARD(x)       ((x) & ENCODER_FOREWARD)
#define ENC_BACKWARD(x)       ((x) & ENCODER_BACKWARD)
// --------------------------------------------------------

// --------------------------------------------------------
// states for the encoder statemchine
enum ENCODER_STATE
{
	ENC_IDLE  = 0,
	ENC_FOREW = 1,
	ENC_BACKW = 2,
	ENC_EQUAL = 3
};
// --------------------------------------------------------

// --------------------------------------------------------
// macro that tells if we have to switch off the led because
// the falshing mode is on.
#define ZIFFER1_FLASH   ((iFlashStatus == 0) && (sThreadDispCmd.aFlash[0] == 1))
#define ZIFFER2_FLASH   ((iFlashStatus == 0) && (sThreadDispCmd.aFlash[1] == 1))
#define ZIFFER3_FLASH   ((iFlashStatus == 0) && (sThreadDispCmd.aFlash[2] == 1))
#define ZIFFER4_FLASH   ((iFlashStatus == 0) && (sThreadDispCmd.aFlash[3] == 1))
// --------------------------------------------------------

// --------------------------------------------------------
// commmands that are executed within the display thread.
enum eDisplayCmdtag
{
	IDLE = 0,
	EXIT
}EDISPLAYCMD;
// --------------------------------------------------------

// --------------------------------------------------------
// Each information needed within the display thread to display,
// flash, etc the 7-segment elements.
typedef struct sDispCmdtag
{
	int iDispTrg;
	int iDispCmd;
	int aZiffern[4];
	int aFlash[4];
}SDISPCMD;
// --------------------------------------------------------


//******************************************************************************
// static/local functions
//******************************************************************************
static  void  configIO       (void);
static void*  thread_Display (void* text);
static  void  disp_SetZ0toZ3 (UINT8 u8Z0, UINT8 u8Z1, UINT8 u8Z2, UINT8 u8Z3);
static  void  disp_latch     (UINT8 u8Value);
static  void  disp_SetZ0toZ3 (UINT8 u8Z0, UINT8 u8Z1, UINT8 u8Z2, UINT8 u8Z3);
static  void  disp_SetBCD    (UINT8 u8A, UINT8 u8B, UINT8 u8C, UINT8 u8D);
static  UINT8 enc_u8Proc     (void);

//******************************************************************************
// global variables
//******************************************************************************
SDISPCMD  sMainDispCmd;
int*      piStatus;
pthread_t threadDisplay;
UINT8     u8EncoderState = ENC_IDLE;
UINT8     u8SwitchD1 = 0;
UINT8     u8Encoder  = 0;
INT32     i32EncoderValue;
INT32     i32EncoderValueOld;

//******************************************************************************
// thread updating the display cyclically
//******************************************************************************
static void* thread_Display(void* text)
{
	SDISPCMD sThreadDispCmd = {0};
	int iFlashCnt=0;
	int iRunningZiffer;
	int iStatus;
	int iExit;
	int iFlashStatus;
	int iZiffer;
	
	iStatus = 0;
	iExit   = 0;
	iFlashStatus = 0;
	iRunningZiffer = 1;

	while (iExit == 0)
	{
		// --------------------------------------------------------
		// flashing
		iFlashCnt++;
		if (iFlashCnt > FLASHCYCLE/THREADCYCLE)   iFlashCnt = 0;
		if (iFlashCnt < FLASHCYCLE/THREADCYCLE/2) iFlashStatus = 1;
		else                                      iFlashStatus = 0;
		// --------------------------------------------------------

		// --------------------------------------------------------
		// Display Update
		if (sMainDispCmd.iDispTrg == 0)
		{
		}
		else if (sMainDispCmd.iDispCmd == EXIT)
		{
			iExit = 1;
		}
		else
		{
			// Update whole Display Command
			sThreadDispCmd = sMainDispCmd;
			sMainDispCmd.iDispTrg = 0;
		}
		// --------------------------------------------------------

		// --------------------------------------------------------
		// Update 7-Segment elements
		switch(iRunningZiffer++)
		{
			case 1:
				iZiffer = sThreadDispCmd.aZiffern[0];
				if (ZIFFER1_FLASH) disp_SetBCD(1,1,1,1);
				else               disp_SetBCD((iZiffer) & 0x01, (iZiffer>>1) & 0x01, (iZiffer>>2) & 0x01, (iZiffer>>3) & 0x01);				
				disp_latch(0);
				disp_SetZ0toZ3(1,0,0,0);  
				disp_latch(1);
				break;
			case 2:
				iZiffer = sThreadDispCmd.aZiffern[1];
				if (ZIFFER2_FLASH) disp_SetBCD(1,1,1,1);
				else               disp_SetBCD((iZiffer) & 0x01, (iZiffer>>1) & 0x01, (iZiffer>>2) & 0x01, (iZiffer>>3) & 0x01);				
				disp_latch(0);
				disp_SetZ0toZ3(0,1,0,0);  
				disp_latch(1);
				break;
			case 3:
				iZiffer = sThreadDispCmd.aZiffern[2];
				if (ZIFFER3_FLASH) disp_SetBCD(1,1,1,1);
				else               disp_SetBCD((iZiffer) & 0x01, (iZiffer>>1) & 0x01, (iZiffer>>2) & 0x01, (iZiffer>>3) & 0x01);				
				disp_latch(0);
				disp_SetZ0toZ3(0,0,1,0);  
				disp_latch(1);
				break;
			case 4:
				iZiffer = sThreadDispCmd.aZiffern[3];
				if (ZIFFER4_FLASH) disp_SetBCD(1,1,1,1);
				else               disp_SetBCD((iZiffer) & 0x01, (iZiffer>>1) & 0x01, (iZiffer>>2) & 0x01, (iZiffer>>3) & 0x01);				
				disp_latch(0);
				disp_SetZ0toZ3(0,0,0,1);  
				disp_latch(1);
				break;
		}
		if (iRunningZiffer == 5) iRunningZiffer = 1;
		// --------------------------------------------------------

		// --------------------------------------------------------
		// update the encoder
		u8Encoder = enc_u8Proc();
		if (ENC_FOREWARD(u8Encoder)) i32EncoderValue++;
		if (ENC_BACKWARD(u8Encoder)) i32EncoderValue--;
		// --------------------------------------------------------

		// --------------------------------------------------------
		// refresh rate
		usleep(THREADCYCLE*1000ul);
		// --------------------------------------------------------
	}
	pthread_exit(&iStatus);
}



//******************************************************************************
// Set the display numbers
//******************************************************************************
void ibb_DisplaySet(int Z1, int Z2, int Z3, int Z4)
{
	sMainDispCmd.aZiffern[0] = Z1;
	sMainDispCmd.aZiffern[1] = Z2;
	sMainDispCmd.aZiffern[2] = Z3;
	sMainDispCmd.aZiffern[3] = Z4;
	sMainDispCmd.iDispCmd = 0;
	sMainDispCmd.iDispTrg = 1;
}

//******************************************************************************
// Get the Encoder Direction: 0 = nothing, 1=foreward, 2=backward
//******************************************************************************
UINT8 ibb_GetEncoder(void)
{
	UINT8 u8RetVal;
	if      (i32EncoderValue > i32EncoderValueOld) u8RetVal = 1;
	else if (i32EncoderValue < i32EncoderValueOld) u8RetVal = 2;
	else                                           u8RetVal = 0;

	i32EncoderValueOld = i32EncoderValue;
	return u8RetVal;
}

//******************************************************************************
// Return the state of the switch
//******************************************************************************
UINT8 ibb_GetSwitch(void)
{
	UINT8 u8Switch;
	UINT8 u8RetVal;
	u8Switch = digitalRead(1);

	if ((u8Switch == 1) && (u8SwitchD1 == 0)) u8RetVal = 1;
	else                                      u8RetVal = 0;

	u8SwitchD1 = u8Switch;
	return u8RetVal;
}

//******************************************************************************
// activate flashing of the display numbers.
// 1 = flashing, 0 = constantly on
//******************************************************************************
void ibb_Flashing(unsigned char flash1, unsigned char flash2, unsigned char flash3, unsigned char flash4 )
{
	// --------------------------------------------------------
	// write the flashing bits into the display command struct.
	sMainDispCmd.aFlash[0] = flash1;
	sMainDispCmd.aFlash[1] = flash2;
	sMainDispCmd.aFlash[2] = flash3;
	sMainDispCmd.aFlash[3] = flash4;
	// --------------------------------------------------------

	// --------------------------------------------------------
	// Trigger the display thread so that it updates the contents
	sMainDispCmd.iDispCmd  = 0; // 0 = no command, 1 = exit 
	sMainDispCmd.iDispTrg  = 1;
	// --------------------------------------------------------
}

//******************************************************************************
// start the display
//******************************************************************************
void ibb_DisplayStart(void)
{
	if (wiringPiSetup() == -1) return;

	// --------------------------------------------------------
	// configure the gpios for use
	configIO();
	// --------------------------------------------------------

	// --------------------------------------------------------
	// setup startup display
	sMainDispCmd.aFlash[0]   = 0;
	sMainDispCmd.aFlash[1]   = 0;
	sMainDispCmd.aFlash[2]   = 0;
	sMainDispCmd.aFlash[3]   = 0;
	sMainDispCmd.aZiffern[0] = 0;
	sMainDispCmd.aZiffern[1] = 0;
	sMainDispCmd.aZiffern[2] = 0;
	sMainDispCmd.aZiffern[3] = 0;
	sMainDispCmd.iDispCmd    = 0;
	sMainDispCmd.iDispTrg    = 1;
	// --------------------------------------------------------

	// --------------------------------------------------------
	// run the thread which updates the display cyclically.
	if (pthread_create(&threadDisplay, NULL, thread_Display, NULL))
	{
		fprintf(stderr, "pthread: pthread_create() failed.\n");
	}
	// --------------------------------------------------------

	i32EncoderValueOld = i32EncoderValue;
}


//******************************************************************************
// stop the display
//******************************************************************************
void ibb_DisplayStop(void)
{
	// --------------------------------------------------------
	// set the display off
	ibb_DisplaySet(15,15,15,15);
	usleep(10000);
	// --------------------------------------------------------

	// --------------------------------------------------------
	// try to finish the thread
	sMainDispCmd.iDispCmd = EXIT;
	sMainDispCmd.iDispTrg = 1;
	// --------------------------------------------------------

	// --------------------------------------------------------
	// wait for the thread to finish
	if (pthread_join(threadDisplay, (void*)&piStatus))
	{
		fprintf(stderr, "pthread: pthread_join() failed.\n");
	}
	// --------------------------------------------------------
}


//******************************************************************************
// configure the gpios for the DISP-SW board.
//******************************************************************************
static void configIO(void)
{
  pinMode(8,  OUTPUT);
  pinMode(9,  OUTPUT);
  pinMode(7,  OUTPUT);
  pinMode(0,  OUTPUT);
  pinMode(2,  OUTPUT);
  pinMode(3,  OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(15, INPUT);
  pinMode(16, INPUT);
  pinMode(1 , INPUT);
}


//******************************************************************************
// activate the current number 
//******************************************************************************
static void disp_latch(UINT8 u8Value)
{
  digitalWrite(7, u8Value);
}

//******************************************************************************
// set the 7-segment elements on or off.
//******************************************************************************
static void disp_SetZ0toZ3(UINT8 u8Z0, UINT8 u8Z1, UINT8 u8Z2, UINT8 u8Z3)
{
	digitalWrite(14, u8Z3);
	digitalWrite(13, u8Z0);
	digitalWrite(12, u8Z1);
	digitalWrite(3 , u8Z2);
}

//******************************************************************************
// set the BCD-number pin by pin
//******************************************************************************
static void disp_SetBCD(UINT8 u8A, UINT8 u8B, UINT8 u8C, UINT8 u8D)
{
	digitalWrite(8, u8A);
	digitalWrite(2, u8B);
	digitalWrite(0, u8C);
	digitalWrite(9, u8D);
}


//******************************************************************************
// Update the encoder statemachine
//******************************************************************************
UINT8 enc_u8Proc(void)
{
	UINT8 u8Action=ENC_IDLE;
	UINT8 u8Encoder=0;
	u8Encoder  = digitalRead(15)<<1;
	u8Encoder |= digitalRead(16);

	switch (u8EncoderState)
	{
		case ENC_IDLE:
			if      (u8Encoder == 2) u8EncoderState = ENC_FOREW;
			else if (u8Encoder == 1) u8EncoderState = ENC_BACKW;			   
			break;
		case ENC_FOREW:
			if      (u8Encoder == 3) 
			{
				u8EncoderState = ENC_EQUAL;
				u8Action = ENC_FOREW;
			}
			else if (u8Encoder == 0) u8EncoderState = ENC_IDLE;			   
			break;
		case ENC_BACKW:
			if      (u8Encoder == 3)
			{
				u8EncoderState = ENC_EQUAL;
				u8Action = ENC_BACKW;
			}
			else if (u8Encoder == 0) u8EncoderState = ENC_IDLE;			   
			break;
		case ENC_EQUAL:
			if      (u8Encoder == 0) u8EncoderState = ENC_IDLE;
			break;
	}

	return u8Action ;
}
