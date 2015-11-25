/* *	File:		DZEvents.c * *	Contents:	Handles the events. * *	Copyright � 1996 Apple Computer, Inc. */#include <assert.h>#include <AppleEvents.h>#include <Events.h>#include <Menus.h>#include <ToolUtils.h>#include <Types.h>#include "DZDisplay.h"#include "DZEvents.h"#include "DZGame.h"#include "DZMain.h"#include "DZMenu.h"static void Events_MouseDown(	const EventRecord*		inEvent);static void Events_KeyDown(	const EventRecord*		inEvent);static void Events_AutoKey(	const EventRecord*		inEvent);static void Events_Update(	const EventRecord*		inEvent);static void Events_Activate(	const EventRecord*		inEvent);static void Events_OS(	const EventRecord*		inEvent);static void Events_HighLevel(	const EventRecord*		inEvent);/* ============================================================================= *		Events_Init (external) * *	Initializes the events stuff. * ========================================================================== */void Events_Init(	void){	FlushEvents(everyEvent, 0);}/* ============================================================================= *		Events_Exit (external) * *	Prepares for exit. * ========================================================================== */void Events_Exit(	void){}/* ============================================================================= *		Events_Process (external) * *	Processes any pending events. * ========================================================================== */void Events_Process(	void){	EventRecord				ev;		while (GetNextEvent(everyEvent, &ev))	{		switch (ev.what)		{			case mouseDown:				Events_MouseDown(&ev);			break;						case keyDown:				Events_KeyDown(&ev);			break;						case autoKey:				Events_AutoKey(&ev);			break;						case updateEvt:				Events_Update(&ev);			break;						case activateEvt:				Events_Activate(&ev);			break;						case osEvt:				Events_OS(&ev);			break;						case kHighLevelEvent:				Events_HighLevel(&ev);			break;					}	}		SystemTask();}/* ============================================================================= *		Events_MouseDown (internal) * *	Processes a mouse-down event. * ========================================================================== */void Events_MouseDown(	const EventRecord*		inEvent){	short 					part;	WindowPtr				wind;	long					menu;	Rect					limits;	long					size;		assert(inEvent != NULL);		part = FindWindow(inEvent->where, &wind);		switch (part)	{		case inMenuBar:			menu = MenuSelect(inEvent->where);			Menu_Select(HiWord(menu), LoWord(menu));		break;					case inSysWindow:			SystemClick(inEvent, wind);		break;					case inDrag:			DragWindow(wind, inEvent->where, &qd.screenBits.bounds);		break;					case inGoAway:			Main_LastRoundup();		break;						case inContent:			// nothing yet...		break;					case inGrow:			limits.top    = 64;			limits.left   = 64;			limits.bottom = 4096;			limits.right  = 4096;						size = GrowWindow(wind, inEvent->where, &limits);						if (size != 0)			{				SetPort(wind);				EraseRect(&wind->portRect);				SizeWindow(wind, LoWord(size), HiWord(size), true);				InvalRect(&wind->portRect);								if (wind == Display_GetWindow())				{					Display_Resize();				}			} 		break;	}}/* ============================================================================= *		Events_KeyDown (internal) * *	Processes a key-down event. * ========================================================================== */void Events_KeyDown(	const EventRecord*		inEvent){	unsigned long			ch;	unsigned long			cap;	long					menu;		assert(inEvent != NULL);		ch = inEvent->message & charCodeMask;	cap = (inEvent->message & keyCodeMask) >> 8;		if ((inEvent->modifiers & cmdKey) != 0)	{		menu = MenuKey(ch);		Menu_Select(HiWord(menu), LoWord(menu));	}	else if (ch == 0x1b)	{		if (Game_GetState() == kGameState_Paused)		{			Game_SetState(kGameState_Playing);		}	}}/* ============================================================================= *		Events_AutoKey (internal) * *	Processes an auto-key event. * ========================================================================== */void Events_AutoKey(	const EventRecord*		inEvent){	// Do nothing for now}/* ============================================================================= *		Events_Update (internal) * *	Processes an update event. * ========================================================================== */void Events_Update(	const EventRecord*		inEvent){	WindowPtr				wind;		assert(inEvent != NULL);		wind = (WindowPtr) inEvent->message;		BeginUpdate(wind);		if (wind == Display_GetWindow())	{		Display_DrawGrow();		Display_DrawContents();	}		EndUpdate(wind);}/* ============================================================================= *		Events_Activate (internal) * *	Processes an activate event. * ========================================================================== */void Events_Activate(	const EventRecord*		inEvent){	WindowPtr				wind;		assert(inEvent != NULL);		wind = (WindowPtr) inEvent->message;		if (wind == Display_GetWindow())	{		Display_Activate((inEvent->modifiers & activeFlag) != 0);	}}/* ============================================================================= *		Events_OS (internal) * *	Processes an OS event. * ========================================================================== */void Events_OS(	const EventRecord*		inEvent){	assert(inEvent != NULL);		if ((inEvent->message >> 24) & 0xFF == suspendResumeMessage)	{		Display_Activate((inEvent->message & resumeFlag) != 0);	}}/* ============================================================================= *		Events_HighLevel (internal) * *	Processes a high-level event. * ========================================================================== */void Events_HighLevel(	const EventRecord*		inEvent){	assert(inEvent != NULL);		AEProcessAppleEvent(inEvent);}