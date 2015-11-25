/* *	File:		DZGame.c * *	Contents:	Handles the game play. * *	Copyright � 1996 Apple Computer, Inc. */#include <assert.h>#include <string.h>#include <Events.h>#include <Fonts.h>#include <QDOffscreen.h>#include <Timer.h>#include <Types.h>#include <QD3D.h>#include <QD3DGeometry.h>#include <QD3DMath.h>#include <QD3DSet.h>#include "SoundSprocket.h"#include "DZDisplay.h"#include "DZDrone.h"#include "DZGame.h"#include "DZInput.h"#include "DZSound.h"#include "DZSpace.h"#ifndef ONE_DRONE	#define ONE_DRONE		0#endif#ifndef	HUD_SHOWS_VELOCITY_VECTOR#define	HUD_SHOWS_VELOCITY_VECTOR	1#endif#define DEFAULT_INTERVAL	0.05		// Initial frame rate -- just a guess#define MAX_INTERVAL		0.25		// Limit on seconds per frame#define INV_SQRT_TWO		0.707#define ROLL_RATE					3.0			// Radians per second for full joystick throw#define PITCH_RATE					1.0			// Radians per second for full joystick throw#define YAW_RATE					3.0			// Radians per second for full joystick throw#define ENGINE_RATING				1.0			// Percentage engine is more/less powerful#define DAMPING_PERCENT				0.90 		// as a percentage (1.00 would be no damping)#define TICKS_PER_AUTOFIRE_SHOT		60 			// numbers of ticks until next auto-repeat shotenum {	#if ONE_DRONE		kGameAutoDroneCount = 1			// Number of autopilot drones	#else		kGameAutoDroneCount = 4			// Number of autopilot drones	#endif};float						gGameInterval			= DEFAULT_INTERVAL;float						gGameFramesPerSecond	= 1.0/DEFAULT_INTERVAL;Boolean						gSoundOn				= true;static TGameState			gGameState				= kGameState_Stopped;static unsigned long		gGameTime				= 0;		// Last frame timestatic Boolean				gGameHUDVisible			= true;static TDroneObject			gGameSelfDrone			= NULL;static GWorldPtr			gGameFPSGWorld			= NULL;static TQ3GeometryObject	gGameFPSMarker			= NULL;static TQ3MarkerData		gGameFPSMarkerData;static Boolean				gGameFPSVisible			= false;static GWorldPtr			gGameThrottleGWorld			= NULL;static TQ3GeometryObject	gGameThrottleMarker			= NULL;static TQ3MarkerData		gGameThrottleMarkerData;static Boolean				gGameThrottleVisible	= false;static GWorldPtr			gGameVelocityGWorld			= NULL;static TQ3GeometryObject	gGameVelocityMarker			= NULL;static TQ3MarkerData		gGameVelocityMarkerData;static Boolean				gGameVelocityVisible	= true;static Boolean				gInertialDampersOn		= false;static Boolean				gFiringOn				= false;static UInt32				gLastShotTicks			= 0;static TQ3GeometryObject	gGameCrossHairs			= NULL;static unsigned long		gGameCrossHairsData[32]	= {	0x00000000, 0x00000000, 0x00000000, 0x00000000,	0x00000000, 0x00010000, 0x00000000, 0x00010000,	0x00000000, 0x00054000, 0x00101000, 0x00228800,	0x00082000, 0x00400400, 0x00101000, 0x05400540,	0x00101000, 0x00400400, 0x00082000, 0x00228800,	0x00101000, 0x00054000, 0x00000000, 0x00010000,	0x00000000, 0x00010000, 0x00000000, 0x00000000,	0x00000000, 0x00000000, 0x00000000, 0x00000000};/* ============================================================================= *		Game_Init (external) * *	Initializes the game stuff. * ========================================================================== */void Game_Init(	void){	Rect				bounds;	PixMapHandle		pixMapHandle;	CGrafPtr			savePort;	GDHandle			saveGDevice;	TQ3MarkerData		markerData;	TQ3ColorRGB			color;		// Set up the 3D sound listener	Sound_GetListener();		// Create the FPS marker	bounds.top    = 0;	bounds.left   = 0;	bounds.bottom = 12;	bounds.right  = 31;		gGameFPSGWorld = NULL;	NewGWorld(&gGameFPSGWorld, 1, &bounds, NULL, NULL, 0);	assert(gGameFPSGWorld != NULL);		LockPixels(gGameFPSGWorld->portPixMap);		GetGWorld(&savePort, &saveGDevice);	SetGWorld(gGameFPSGWorld, NULL);		TextFont(kFontIDGeneva);	TextSize(10);		EraseRect(&gGameFPSGWorld->portRect);		SetGWorld(savePort, saveGDevice);		pixMapHandle = GetGWorldPixMap(gGameFPSGWorld);		gGameFPSMarkerData.location.x			= 0.0;	gGameFPSMarkerData.location.y			= 0.0;	gGameFPSMarkerData.location.z			= 0.0;	gGameFPSMarkerData.xOffset				= -(bounds.left+bounds.right+1 >> 1);	gGameFPSMarkerData.yOffset				= -(bounds.top+bounds.bottom+1 >> 1);	gGameFPSMarkerData.bitmap.image			= (unsigned char*) GetPixBaseAddr(pixMapHandle);	gGameFPSMarkerData.bitmap.width			= bounds.right-bounds.left;	gGameFPSMarkerData.bitmap.height		= bounds.bottom-bounds.top;	gGameFPSMarkerData.bitmap.rowBytes		= (*pixMapHandle)->rowBytes & 0x00003FFF;	gGameFPSMarkerData.bitmap.bitOrder		= kQ3EndianBig;	gGameFPSMarkerData.markerAttributeSet	= Q3AttributeSet_New();	assert(gGameFPSMarkerData.markerAttributeSet != NULL);		color.r = 1.0;	color.g = 1.0;	color.b = 0.4;		Q3AttributeSet_Add(gGameFPSMarkerData.markerAttributeSet, kQ3AttributeTypeDiffuseColor, &color);		gGameFPSMarker = Q3Marker_New(&gGameFPSMarkerData);	// Create the Velocity marker	bounds.top    = 0;	bounds.left   = 0;	bounds.bottom = 12;	bounds.right  = 31;		gGameVelocityGWorld = NULL;	NewGWorld(&gGameVelocityGWorld, 1, &bounds, NULL, NULL, 0);	assert(gGameVelocityGWorld != NULL);		LockPixels(gGameVelocityGWorld->portPixMap);		GetGWorld(&savePort, &saveGDevice);	SetGWorld(gGameVelocityGWorld, NULL);		TextFont(kFontIDGeneva);	TextSize(10);		EraseRect(&gGameVelocityGWorld->portRect);		SetGWorld(savePort, saveGDevice);		pixMapHandle = GetGWorldPixMap(gGameVelocityGWorld);		gGameVelocityMarkerData.location.x			= 0.0;	gGameVelocityMarkerData.location.y			= 0.0;	gGameVelocityMarkerData.location.z			= 0.0;	gGameVelocityMarkerData.xOffset				= -(bounds.left+bounds.right+1 >> 1);	gGameVelocityMarkerData.yOffset				= -(bounds.top+bounds.bottom+1 >> 1);	gGameVelocityMarkerData.bitmap.image		= (unsigned char*) GetPixBaseAddr(pixMapHandle);	gGameVelocityMarkerData.bitmap.width		= bounds.right-bounds.left;	gGameVelocityMarkerData.bitmap.height		= bounds.bottom-bounds.top;	gGameVelocityMarkerData.bitmap.rowBytes		= (*pixMapHandle)->rowBytes & 0x00003FFF;	gGameVelocityMarkerData.bitmap.bitOrder		= kQ3EndianBig;	gGameVelocityMarkerData.markerAttributeSet	= Q3AttributeSet_New();	assert(gGameVelocityMarkerData.markerAttributeSet != NULL);		color.r = 1.0;	color.g = 1.0;	color.b = 0.4;		Q3AttributeSet_Add(gGameVelocityMarkerData.markerAttributeSet, kQ3AttributeTypeDiffuseColor, &color);		gGameVelocityMarker = Q3Marker_New(&gGameVelocityMarkerData);		// Create the crosshairs	markerData.location.x			= 0.0;	markerData.location.y			= 0.0;	markerData.location.z			= 0.0;	markerData.xOffset				= -15;	markerData.yOffset				= -15;	markerData.bitmap.image			= (unsigned char*) gGameCrossHairsData;	markerData.bitmap.width			= 31;  //� SHOULD BE 32, BUT TO GET AROUND A APPLE QD3D ACCEL CARD DRIVER BUG...	markerData.bitmap.height		= 32;	markerData.bitmap.rowBytes		= 4;	markerData.bitmap.bitOrder		= kQ3EndianBig;	markerData.markerAttributeSet	= Q3AttributeSet_New();	assert(markerData.markerAttributeSet != NULL);		color.r = 1.0;	color.g = 1.0;	color.b = 0.4;		Q3AttributeSet_Add(markerData.markerAttributeSet, kQ3AttributeTypeDiffuseColor, &color);		gGameCrossHairs = Q3Marker_New(&markerData);		Q3Object_Dispose(markerData.markerAttributeSet);	markerData.markerAttributeSet = NULL;}/* ============================================================================= *		Game_Exit (external) * *	Prepares for exit. * ========================================================================== */void Game_Exit(	void){	if (gGameFPSMarker != NULL)	{		Q3Object_Dispose(gGameFPSMarker);		gGameFPSMarker = NULL;	}		if (gGameVelocityMarker != NULL)	{		Q3Object_Dispose(gGameVelocityMarker);		gGameVelocityMarker = NULL;	}		if (gGameCrossHairs != NULL)	{		Q3Object_Dispose(gGameCrossHairs);		gGameCrossHairs = NULL;	}}/* ============================================================================= *		Game_GetState (external) * *	Returns the game state. * ========================================================================== */TGameState Game_GetState(	void){	return gGameState;}/* ============================================================================= *		Game_SetState (external) * *	Changes the game state. * ========================================================================== */void Game_SetState(	TGameState			inGameState){	TGameState			prevGameState;	TDroneObject		drone;	int					droneNum;		if (gGameState != inGameState)	{		prevGameState = gGameState;		gGameState = inGameState;				switch (gGameState)		{			case kGameState_Playing:				// Begin Playing				if (prevGameState == kGameState_Stopped)				{					gGameSelfDrone = SelfDrone_New();										// Create some automatic drones					for (droneNum = 0; droneNum < kGameAutoDroneCount; droneNum++)					{						AutoDrone_New(gGameSelfDrone);					}				}								Display_DrawContents();				Input_Activate(true);			break;						case kGameState_Paused:				// From Playing to Paused				assert(prevGameState != kGameState_Stopped);								Game_Silence();				Display_DrawContents();				Input_Activate(false);			break;						case kGameState_Stopped:				// From Playing or Paused to Stopped				while ((drone = Drone_Next(NULL)) != NULL)				{					Drone_Dispose(drone);				}								gGameSelfDrone = NULL;								Display_DrawContents();				Input_Activate(false);			break;		}	}}/* ============================================================================= *		Game_Process (external) * *	Handles idle time by moving the game ahead one time step.  Only called if *	the game is in "play" state. * ========================================================================== */void Game_Process(	void){	UnsignedWide		wide;	unsigned long		now;	Str15				str;	CGrafPtr			savePort;	GDHandle			saveGDevice;	TDroneObject		drone;	TDroneObject		next;	TQ3Point3D			position;	TQ3Vector3D			direction;	TQ3Vector3D			up;	TQ3Matrix4x4		matrix;	Boolean				ok;	UInt32				count;		// Find the frame rate	Microseconds(&wide);	now = wide.lo;		if (gGameTime != 0)	{		// Find the interval for the last frame		gGameInterval = 0.000001*(now-gGameTime);				// Limit frame rate to a resonable number		if (gGameInterval > MAX_INTERVAL)		{			gGameInterval = MAX_INTERVAL;		}				// Find corresponding frames per second		gGameFramesPerSecond = 1.0/gGameInterval;	}		gGameTime = now;		// Update the FPS marker	if (gGameFPSVisible)	{		sprintf((char*) str, "x%.1f", gGameFramesPerSecond);		str[0] = strlen((char*) str) - 1;				GetGWorld(&savePort, &saveGDevice);		SetGWorld(gGameFPSGWorld, NULL);				EraseRect(&gGameFPSGWorld->portRect);				MoveTo(gGameFPSGWorld->portRect.left+gGameFPSGWorld->portRect.right-StringWidth(str) >> 1, 10);		DrawString(str);				SetGWorld(savePort, saveGDevice);				Q3Marker_SetBitmap(gGameFPSMarker, &gGameFPSMarkerData.bitmap);	}	// Update the velocity marker	if (gGameVelocityVisible)	{		TQ3Vector3D		velocity;				Drone_GetVelocity(gGameSelfDrone, &velocity);				sprintf((char*) str, "x%.1f", gGameFramesPerSecond);		str[0] = strlen((char*) str) - 1;				GetGWorld(&savePort, &saveGDevice);		SetGWorld(gGameVelocityGWorld, NULL);				EraseRect(&gGameVelocityGWorld->portRect);				MoveTo(gGameVelocityGWorld->portRect.left+gGameVelocityGWorld->portRect.right-StringWidth(str) >> 1, 10);		DrawString(str);				SetGWorld(savePort, saveGDevice);				Q3Marker_SetBitmap(gGameVelocityMarker, &gGameVelocityMarkerData.bitmap);	}		// Get and process state-based inputs	SelfDrone_Roll(gGameSelfDrone, Input_GetRoll()*gGameInterval*ROLL_RATE);	SelfDrone_Pitch(gGameSelfDrone, Input_GetPitch()*gGameInterval*PITCH_RATE);	SelfDrone_Yaw(gGameSelfDrone, Input_GetYaw()*gGameInterval*YAW_RATE);		if (gInertialDampersOn) SelfDrone_DampVelocity(gGameSelfDrone, DAMPING_PERCENT);	SelfDrone_Thrust(gGameSelfDrone, Input_GetThrottle()*gGameInterval*ENGINE_RATING);	if (gFiringOn) 	{		UInt32	ticks = TickCount();				if (gLastShotTicks + TICKS_PER_AUTOFIRE_SHOT < ticks)		{			Drone_Fire(gGameSelfDrone);			gLastShotTicks = ticks;		}	}	// Get and process input events	ok = true;	do	{		switch (Input_GetEvent())		{			case kInputEvent_None:				// No more input events to process				ok = false;			break;						case kInputEvent_Fire_On:				// Fire button pressed down				Drone_Fire(gGameSelfDrone);				gLastShotTicks = TickCount();				gFiringOn = true;			break;						case kInputEvent_Fire_Off:				// Fire button released				gFiringOn = false;			break;						case kInputEvent_InertialDampers_On:				// Inertial Dampers button down				SelfDrone_DampVelocity(gGameSelfDrone, DAMPING_PERCENT);				gInertialDampersOn = true;			break;						case kInputEvent_InertialDampers_Off:				// Inertial Dampers button released				gInertialDampersOn = false;			break;						case kInputEvent_InstantStop:				// Set Velocity to zero				SelfDrone_InstantStop(gGameSelfDrone);			break;						case kInputEvent_ShowHUD:				// Toggle HMD display				gGameHUDVisible = !gGameHUDVisible;			break;						case kInputEvent_ShowFPS:				// Toggle FPS display				gGameFPSVisible = !gGameFPSVisible;			break;						case kInputEvent_ShowThrottle:				// Toggle throttle display				gGameThrottleVisible = !gGameThrottleVisible;			break;			case kInputEvent_ShowVelocity:				// Toggle velocity display				gGameVelocityVisible = !gGameVelocityVisible;			break;			case kInputEvent_Pause:				// Toggle paused/play state				switch (gGameState)				{					case kGameState_Playing:						Game_SetState(kGameState_Paused);					break;										case kGameState_Paused:						Game_SetState(kGameState_Playing);					break;										case kGameState_Stopped:						// do nothing					break;				}			break;					}	}	while (ok);		// Move all the drones, mark any to be deleted	for (drone = Drone_Next(NULL); drone != NULL; drone = Drone_Next(drone))	{		Drone_Move(drone);	}		// Delete the marked drones	drone = Drone_Next(NULL);	while (drone != NULL)	{		next = Drone_Next(drone);				if (Drone_GetMark(drone))		{			Drone_Dispose(drone);		}				drone = next;	}		// Move the viewer to follow the self drone	Drone_GetPosition(gGameSelfDrone, &position);	Drone_GetDirection(gGameSelfDrone, &direction);	Drone_GetUp(gGameSelfDrone, &up);		Display_SetViewerPosition(&position, &direction, &up);		if (gSoundOn)	{		// Move the listener to follow the self drone		Drone_GetMatrix(gGameSelfDrone, &matrix);			SSpListener_SetTransform(Sound_GetListener(), &matrix);				// Change the localized sounds		for (drone = Drone_Next(NULL); drone != NULL; drone = Drone_Next(drone))		{			Drone_UpdateSound(drone);		}	}		// Time to retire?		// NOTE: This is not the best way to determine if the game is done.  We	// should probably ask each drone if it thinks the game should continue.	// Only AutoDrones would respond yes.  Then we'd "or" the results.		count = 0;	for (drone = Drone_Next(NULL); drone != NULL; drone = Drone_Next(drone))	{		count += 1;	}		if (count == 1)	{		// Only the autodrone is left -- time to quit		Game_SetState(kGameState_Stopped);	}}/* ============================================================================= *		Game_Submit (external) * *	Submits all the 3D geometry of the game. * ========================================================================== */void Game_Submit(	TQ3ViewObject		inView){	TDroneObject		drone;	TQ3Point3D			position;	TQ3Vector3D			direction;	TQ3Vector3D			velocity;	TQ3Vector3D			up;	TQ3Vector3D			v;	TQ3Point3D			markerPosition;		assert(inView != NULL);		// Submit the drones	for (drone = Drone_Next(NULL); drone != NULL; drone = Drone_Next(drone))	{		Drone_Submit(drone, gGameHUDVisible, inView);	}		// Get information about the camera position	Drone_GetPosition(gGameSelfDrone, &position);	Drone_GetDirection(gGameSelfDrone, &direction);	Drone_GetUp(gGameSelfDrone, &up);	Drone_GetVelocity(gGameSelfDrone, &velocity);		// Submit the spacejunk	Space_Submit(inView, &position, &direction);		// Submit the FPS marker	if (gGameFPSVisible)	{		Q3Point3D_Vector3D_Add(&position, &direction, &markerPosition);		Q3Vector3D_Scale(&up, -0.6, &v);		Q3Point3D_Vector3D_Add(&markerPosition, &v, &markerPosition);		Q3Marker_SetPosition(gGameFPSMarker, &markerPosition);		Q3Object_Submit(gGameFPSMarker, inView);	}	// Submit the velocity marker	if (gGameVelocityVisible)	{		Q3Point3D_Vector3D_Add(&position, &direction, &markerPosition);		Q3Vector3D_Scale(&up, -0.8, &v);		Q3Point3D_Vector3D_Add(&markerPosition, &v, &markerPosition);		Q3Marker_SetPosition(gGameVelocityMarker, &markerPosition);		Q3Object_Submit(gGameVelocityMarker, inView);	}		// Submit the crosshairs	if (gGameHUDVisible)	{#if HUD_SHOWS_VELOCITY_VECTOR		Q3Vector3D_Normalize(&velocity, &velocity);		Q3Point3D_Vector3D_Add(&position, &velocity, &markerPosition);#else				Q3Point3D_Vector3D_Add(&position, &direction, &markerPosition);#endif		Q3Marker_SetPosition(gGameCrossHairs, &markerPosition);		Q3Object_Submit(gGameCrossHairs, inView);	}}/* ============================================================================= *		Game_Silence (external) * *	Stops all sounds. * ========================================================================== */void Game_Silence(	void){	TDroneObject		drone;		for (drone = Drone_Next(NULL); drone != NULL; drone = Drone_Next(drone))	{		Drone_Silence(drone);	}}