#pragma once

#include "datastructures.h"
#include "animation.h"

typedef enum {
	BLITZ_TIMELINE_ANIMATION_STEP_TYPE_INTERPOLATION_END,
	BLITZ_TIMELINE_ANIMATION_STEP_TYPE_INTERPOLATION,
	BLITZ_TIMELINE_ANIMATION_STEP_TYPE_SETTING,
}BlitzTimelineAnimationStepType;

typedef enum {
	BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_POSITION_X,
	BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_POSITION_Y,
	BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_MUGEN_ANIMATION,
}BlitzTimelineAnimationStepTargetType;

typedef struct {
	Tick mTime;
	BlitzTimelineAnimationStepType mType;
	BlitzTimelineAnimationStepTargetType mTargetType;

	Tick mDuration;
	char mStartValue[100];
	char mEndValue[100];
} BlitzTimelineAnimationStep;

typedef struct {
	int mID;
	Tick mDuration;

	Vector BlitzTimelineAnimationSteps; // contains BlitzTimelineAnimationStep
} BlitzTimelineAnimation;

typedef struct {
	IntMap mAnimations; // contains BlitzTimelineAnimation
} BlitzTimelineAnimations;

BlitzTimelineAnimations loadBlitzTimelineAnimations(char* tPath);
BlitzTimelineAnimation* getBlitzTimelineAnimation(BlitzTimelineAnimations* tAnimations, int tAnimationID);