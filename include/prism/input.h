#pragma once

#define MAXIMUM_CONTROLLER_AMOUNT 2

#include "geometry.h"
#include "animation.h"

void updateInput();
void resetInputForAllControllers();

int hasPressedA();
int hasPressedB();
int hasPressedX();
int hasPressedY();
int hasPressedLeft();
int hasPressedRight();
int hasPressedUp();
int hasPressedDown();
int hasPressedL();
int hasPressedR();
int hasPressedStart();
int hasPressedAbort();

int hasPressedAFlank();
int hasPressedBFlank();
int hasPressedXFlank();
int hasPressedYFlank();
int hasPressedLeftFlank();
int hasPressedRightFlank();
int hasPressedUpFlank();
int hasPressedDownFlank();
int hasPressedLFlank();
int hasPressedRFlank();
int hasPressedStartFlank();
int hasPressedAbortFlank();

double getLeftStickNormalizedX();
double getLeftStickNormalizedY();
double getLNormalized();
double getRNormalized();

int hasShotGun();
int hasShotGunFlank();
Vector3D getShotPosition();

int hasPressedASingle(int i);
int hasPressedBSingle(int i);
int hasPressedXSingle(int i);
int hasPressedYSingle(int i);
int hasPressedLeftSingle(int i);
int hasPressedRightSingle(int i);
int hasPressedUpSingle(int i);
int hasPressedDownSingle(int i);
int hasPressedLSingle(int i);
int hasPressedRSingle(int i);
int hasPressedStartSingle(int i);
int hasPressedAbortSingle(int i);

int hasPressedAFlankSingle(int i);
int hasPressedBFlankSingle(int i);
int hasPressedXFlankSingle(int i);
int hasPressedYFlankSingle(int i);
int hasPressedLeftFlankSingle(int i);
int hasPressedRightFlankSingle(int i);
int hasPressedUpFlankSingle(int i);
int hasPressedDownFlankSingle(int i);
int hasPressedLFlankSingle(int i);
int hasPressedRFlankSingle(int i);
int hasPressedStartFlankSingle(int i);
int hasPressedAbortFlankSingle(int i);

double getSingleLeftStickNormalizedX(int i);
double getSingleLeftStickNormalizedY(int i);
double getSingleLNormalized(int i);
double getSingleRNormalized(int i);

int hasShotGunSingle(int i);
int hasShotGunFlankSingle(int i);
Vector3D getShotPositionSingle(int i);

void setMainController(int i);
int getMainController();

void forceMouseCursorToWindow();
void releaseMouseCursorFromWindow();

int isUsingControllerSingle(int i);
int isUsingController();

double getFishingRodIntensity();
double getFishingRodIntensitySingle(int i);

void addControllerRumbleBasic(Duration tDuration);
void addControllerRumble(Duration tDuration, int tFrequency, double tAmplitude);
void turnControllerRumbleOn(int tFrequency, double tAmplitude);
void turnControllerRumbleOff();

void addControllerRumbleBasicSingle(int i, Duration tDuration);
void addControllerRumbleSingle(int i, Duration tDuration, int tFrequency, double tAmplitude);
void turnControllerRumbleOnSingle(int i, int tFrequency, double tAmplitude);
void turnControllerRumbleOffSingle(int i);