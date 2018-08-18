#include "prism/screeneffect.h"

#include "prism/file.h"
#include "prism/timer.h"
#include "prism/memoryhandler.h"
#include "prism/physicshandler.h"
#include "prism/system.h"
#include "prism/texture.h"
#include "prism/system.h"
#include "prism/log.h"
#include "prism/math.h"

struct FadeInStruct;

typedef int(*IsScreenEffectOverFunction)(struct FadeInStruct* );

typedef struct FadeInStruct {
	int* mAnimationIDs;
	int mAnimationAmount;

	int mPhysicsID;
	Vector3D* mSize;

	int mAlphaPhysicsID;
	double* mAlpha;

	Duration mDuration;

	ScreenEffectFinishedCB mCB;
	void* mCaller;

	IsScreenEffectOverFunction mIsOverFunction;
} FadeIn;

typedef struct {
	double mR;
	double mG;
	double mB;

} FadeColor;

static struct {
	TextureData mWhiteTexture;
	int mIsActive;
	double mZ;

	int mFullLineSize;

	int mScreenFillID;

	FadeColor mFadeColor;
} gData;

void initScreenEffects() {
	gData.mWhiteTexture = createWhiteTexture();
	gData.mFullLineSize = 10;
	gData.mZ = 80;
	gData.mScreenFillID = -1;
	gData.mFadeColor.mR = gData.mFadeColor.mG = gData.mFadeColor.mB = 0;

	gData.mIsActive = 1;
}

void shutdownScreenEffects() {
	if (!gData.mIsActive) return;

	unloadTexture(gData.mWhiteTexture);
	gData.mIsActive = 0;
}

static void unloadedBehaviour(Duration tDuration, ScreenEffectFinishedCB tOptionalCB, void* tCaller) {
	if (tOptionalCB != NULL) {
			addTimerCB(tDuration, tOptionalCB, tCaller);
	}
}

static int isVerticalLineFadeInOver(FadeIn* tFadeIn) {
	return tFadeIn->mSize->y <= 0;
}	

static void removeFadeIn(FadeIn* e) {
	removeFromPhysicsHandler(e->mPhysicsID);
	removeFromPhysicsHandler(e->mAlphaPhysicsID);
	
	int i;
	for (i = 0; i < e->mAnimationAmount; i++) {
		removeHandledAnimation(e->mAnimationIDs[i]);
	}

	freeMemory(e->mAnimationIDs);
	freeMemory(e);
}

static void updateSingleFadeInAnimation(FadeIn* e, int i) {
	setAnimationSize(e->mAnimationIDs[i], *e->mSize, makePosition(0, 0, 0));
	setAnimationTransparency(e->mAnimationIDs[i], *e->mAlpha);
}

static void updateFadeIn(void* tCaller) {
	FadeIn* e = tCaller;

	if (e->mIsOverFunction(e)) {
		if (e->mCB) e->mCB(e->mCaller);
		removeFadeIn(e);
		return;
	}

	int i;
	for (i = 0; i < e->mAnimationAmount; i++) {
		updateSingleFadeInAnimation(e, i);
	}

	addTimerCB(0, updateFadeIn, e);
}

static void addFadeIn_internal(Duration tDuration, ScreenEffectFinishedCB tOptionalCB, void* tCaller, Vector3D tStartPatchSize, Vector3D tFullPatchSize, Vector3D tSizeDelta, double tStartAlpha, double tAlphaDelta, IsScreenEffectOverFunction tIsOverFunc) {
	if (!gData.mIsActive) {
		unloadedBehaviour(tDuration, tOptionalCB, tCaller);
		return;
	}

	ScreenSize screen = getScreenSize();

	FadeIn* e = allocMemory(sizeof(FadeIn));
	e->mDuration = tDuration;
	e->mCB = tOptionalCB;
	e->mCaller = tCaller;
	e->mDuration = tDuration;

	e->mPhysicsID = addToPhysicsHandler(tStartPatchSize);
	addAccelerationToHandledPhysics(e->mPhysicsID, tSizeDelta);
	e->mSize = &getPhysicsFromHandler(e->mPhysicsID)->mPosition;

	e->mAlphaPhysicsID = addToPhysicsHandler(makePosition(tStartAlpha, 0, 0));
	addAccelerationToHandledPhysics(e->mAlphaPhysicsID, makePosition(tAlphaDelta, 0, 0));
	e->mAlpha = &getPhysicsFromHandler(e->mAlphaPhysicsID)->mPosition.x;

	e->mIsOverFunction = tIsOverFunc;

	int amountX = (int)((screen.x + (tFullPatchSize.x - 1)) / tFullPatchSize.x);
	int amountY = (int)((screen.y + (tFullPatchSize.y - 1)) / tFullPatchSize.y);
	e->mAnimationAmount = amountX*amountY;

	e->mAnimationIDs = allocMemory(e->mAnimationAmount*sizeof(int));
	Position p = makePosition(0, 0, gData.mZ);
	int i;
	for (i = 0; i < e->mAnimationAmount; i++) {
		e->mAnimationIDs[i] = playAnimationLoop(p, &gData.mWhiteTexture, createOneFrameAnimation(), makeRectangleFromTexture(gData.mWhiteTexture));
		updateSingleFadeInAnimation(e, i);
		setAnimationColor(e->mAnimationIDs[i], gData.mFadeColor.mR, gData.mFadeColor.mG, gData.mFadeColor.mB);

		p = vecAdd(p, makePosition(tFullPatchSize.x, 0, 0));
		if (p.x >= screen.x) {
			p = vecAdd(p, tFullPatchSize);
			p.x = 0;
		}
	}

	addTimerCB(0, updateFadeIn, e);
}

static int isFadeInOver(FadeIn* e) {
	return (*e->mAlpha) <= 0;
}

void addFadeIn(Duration tDuration, ScreenEffectFinishedCB tOptionalCB, void* tCaller) {
	double da = -1 / (double)tDuration;
	Vector3D patchSize = makePosition(getScreenSize().x, getScreenSize().y, 1);
	addFadeIn_internal(tDuration, tOptionalCB, tCaller, patchSize, patchSize, makePosition(0, 0, 0), 1, da, isFadeInOver);
}

void addVerticalLineFadeIn(Duration tDuration, ScreenEffectFinishedCB tOptionalCB, void* tCaller) {
	double dy = -gData.mFullLineSize / (double)tDuration;
	addFadeIn_internal(tDuration, tOptionalCB, tCaller, makePosition(getScreenSize().x, gData.mFullLineSize+1, 1), makePosition(getScreenSize().x, gData.mFullLineSize, 1), makePosition(0, dy, 0), 1, 0, isVerticalLineFadeInOver);
}

typedef struct {
	void* mCaller;
	ScreenEffectFinishedCB mCB;
} FadeOutData;

static void fadeOutOverCB(void* tCaller) {
	FadeOutData* e = tCaller;
	setScreenBlack();
	if (e->mCB) {
		e->mCB(e->mCaller);
	}
	freeMemory(e);
}

static int isFadeOutOver(FadeIn* e) {
	return (*e->mAlpha) >= 1;
}

void addFadeOut(Duration tDuration, ScreenEffectFinishedCB tOptionalCB, void* tCaller) {
	double da = 1 / (double)tDuration;
	Vector3D patchSize = makePosition(getScreenSize().x, getScreenSize().y, 1);
	FadeOutData* e = allocMemory(sizeof(FadeOutData));
	e->mCB = tOptionalCB;
	e->mCaller = tCaller;

	addFadeIn_internal(tDuration, fadeOutOverCB, e, patchSize, patchSize, makePosition(0, 0, 0), 0, da, isFadeOutOver);
}

void setFadeColor(Color tColor) {
	getRGBFromColor(tColor, &gData.mFadeColor.mR, &gData.mFadeColor.mG, &gData.mFadeColor.mB);
}

void setFadeColorRGB(double r, double g, double b) {
	gData.mFadeColor.mR = r;
	gData.mFadeColor.mG = g;
	gData.mFadeColor.mB = b;
}

void drawColoredRectangle(GeoRectangle tRect, Color tColor) {
	if (!gData.mIsActive) return;

	double dx = (tRect.mBottomRight.x - tRect.mTopLeft.x);
	double dy = (tRect.mBottomRight.y - tRect.mTopLeft.y);
	dx /= gData.mWhiteTexture.mTextureSize.x;
	dy /= gData.mWhiteTexture.mTextureSize.y;

	scaleDrawing3D(makePosition(dx, dy, 1), tRect.mTopLeft);
	setDrawingBaseColor(tColor);
	drawSprite(gData.mWhiteTexture, tRect.mTopLeft, makeRectangleFromTexture(gData.mWhiteTexture));
	setDrawingParametersToIdentity();
}

void drawColoredHorizontalLine(Position tA, Position tB, Color tColor)
{
	if (tA.y != tB.y) return;

	double x = min(tA.x, tB.x);
	double w = fabs(tB.x - tA.x);
	drawColoredRectangle(makeGeoRectangle3D(x, tA.y, tA.z, w, 1), tColor);
}

void drawColoredPoint(Position tPoint, Color tColor) {
	drawColoredRectangle(makeGeoRectangle3D(tPoint.x, tPoint.y, tPoint.z, 1, 1), tColor);
}

void setScreenBlack() {
	if (!gData.mIsActive) return;

	setScreenColor(COLOR_BLACK);
	return;

	gData.mScreenFillID = playAnimationLoop(makePosition(0,0,gData.mZ), &gData.mWhiteTexture, createOneFrameAnimation(), makeRectangleFromTexture(gData.mWhiteTexture));
	setAnimationSize(gData.mScreenFillID, makePosition(640, 480, 1), makePosition(0, 0, 0));
	setAnimationColor(gData.mScreenFillID, gData.mFadeColor.mR, gData.mFadeColor.mG, gData.mFadeColor.mB);
}

void unsetScreenBlack() {
	if (!gData.mIsActive) return;

	unsetScreenColor();
	return;

	if (gData.mScreenFillID == -1) {
		logError("Screen not set to black, unable to reset");
		recoverFromError();
	}

	removeHandledAnimation(gData.mScreenFillID);
}

void setScreenWhite() {
	setScreenColor(COLOR_WHITE);
}

void unsetScreenWhite() {
	unsetScreenColor();
}

TextureData getEmptyWhiteTexture()
{
	return gData.mWhiteTexture;
}
