#include "prism/blitzmugenanimation.h"

#include "prism/datastructures.h"
#include "prism/blitzentity.h"
#include "prism/blitzcamerahandler.h"
#include "prism/log.h"
#include "prism/system.h"
#include "prism/stlutil.h"

using namespace std;

typedef struct {
	int mEntityID;

	MugenSpriteFile* mSprites;
	MugenAnimations* mAnimations;
	int mIsStatic;

	int mAnimationID;
} BlitzAnimationEntry;

static struct {
	map<int, BlitzAnimationEntry> mEntities;
} gBlitzAnimationData;

static void loadBlitzMugenAnimationHandler(void* tData) {
	(void)tData;
	gBlitzAnimationData.mEntities.clear();
}

static void unloadBlitzMugenAnimationHandler(void* tData) {
	(void)tData;
	gBlitzAnimationData.mEntities.clear();
}


ActorBlueprint getBlitzMugenAnimationHandler() {
	return makeActorBlueprint(loadBlitzMugenAnimationHandler, unloadBlitzMugenAnimationHandler);
}

static void unregisterEntity(int tEntityID);

BlitzComponent getBlitzMugenAnimationComponent() {
	return makeBlitzComponent(unregisterEntity);
}

static BlitzAnimationEntry* getBlitzAnimationEntry(int tEntityID) {
	if (!stl_map_contains(gBlitzAnimationData.mEntities, tEntityID)) {
		logErrorFormat("Entity with ID %d does not have animation component.", tEntityID);
		recoverFromError();
	}

	return &gBlitzAnimationData.mEntities[tEntityID];
}

void addBlitzMugenAnimationComponentGeneral(int tEntityID, MugenSpriteFile * tSprites, MugenAnimations * tAnimations, MugenAnimation* tStartAnimation, int tIsStatic)
{
	BlitzAnimationEntry e;
	e.mEntityID = tEntityID;
	e.mSprites = tSprites;
	e.mAnimations = tAnimations;
	e.mAnimationID = addMugenAnimation(tStartAnimation, tSprites, makePosition(0, 0, 0));
	e.mIsStatic = tIsStatic;
	setMugenAnimationBasePosition(e.mAnimationID, getBlitzEntityPositionReference(tEntityID));
	setMugenAnimationScaleReference(e.mAnimationID, getBlitzEntityScaleReference(tEntityID));
	setMugenAnimationAngleReference(e.mAnimationID, getBlitzEntityRotationZReference(tEntityID));
	if (isBlitzCameraHandlerEnabled()) {
		setMugenAnimationCameraPositionReference(e.mAnimationID, getBlitzCameraHandlerPositionReference());
		setMugenAnimationCameraScaleReference(e.mAnimationID, getBlitzCameraHandlerScaleReference());
		setMugenAnimationCameraAngleReference(e.mAnimationID, getBlitzCameraHandlerRotationZReference());
		setMugenAnimationCameraEffectPositionReference(e.mAnimationID, getBlitzCameraHandlerEffectPositionReference());
	}
	
	registerBlitzComponent(tEntityID, getBlitzMugenAnimationComponent());
	gBlitzAnimationData.mEntities[tEntityID] = e;
}

void addBlitzMugenAnimationComponent(int tEntityID, MugenSpriteFile * tSprites, MugenAnimations * tAnimations, int tStartAnimation)
{
	addBlitzMugenAnimationComponentGeneral(tEntityID, tSprites, tAnimations, getMugenAnimation(tAnimations, tStartAnimation), 0);
}

void addBlitzMugenAnimationComponentStatic(int tEntityID, MugenSpriteFile * tSprites, int tSpriteGroup, int tSpriteItem)
{
	addBlitzMugenAnimationComponentGeneral(tEntityID, tSprites, NULL, createOneFrameMugenAnimationForSprite(tSpriteGroup, tSpriteItem), 1);
}

static void unregisterEntity(int tEntityID) {
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	removeMugenAnimation(e->mAnimationID);
	gBlitzAnimationData.mEntities.erase(tEntityID);
}

int getBlitzMugenAnimationID(int tEntityID)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	return e->mAnimationID;
}

void changeBlitzMugenAnimation(int tEntityID, int tAnimationNumber)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	changeMugenAnimation(e->mAnimationID, getMugenAnimation(e->mAnimations, tAnimationNumber));
}

void changeBlitzMugenAnimationWithStartStep(int tEntityID, int tAnimationNumber, int tStep) {
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	changeMugenAnimationWithStartStep(e->mAnimationID, getMugenAnimation(e->mAnimations, tAnimationNumber), tStep);
}

void changeBlitzMugenAnimationIfDifferent(int tEntityID, int tAnimationNumber) {
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	if (getMugenAnimationAnimationNumber(e->mAnimationID) == tAnimationNumber) return;

	changeMugenAnimation(e->mAnimationID, getMugenAnimation(e->mAnimations, tAnimationNumber));
}

int getBlitzMugenAnimationAnimationNumber(int tEntityID) {
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	return getMugenAnimationAnimationNumber(e->mAnimationID);
}

void setBlitzMugenAnimationTransparency(int tEntityID, double tTransparency) {
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	setMugenAnimationTransparency(e->mAnimationID, tTransparency);
}

void setBlitzMugenAnimationFaceDirection(int tEntityID, int tIsFacingRight) {
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	setMugenAnimationFaceDirection(e->mAnimationID, tIsFacingRight);
}

void setBlitzMugenAnimationVerticalFaceDirection(int tEntityID, int tIsFacingDown)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	setMugenAnimationVerticalFaceDirection(e->mAnimationID, tIsFacingDown);
}

void setBlitzMugenAnimationPositionX(int tEntityID, double tX)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	Position p = getMugenAnimationPosition(e->mAnimationID);
	p.x = tX;
	setMugenAnimationPosition(e->mAnimationID, p);
}

void setBlitzMugenAnimationPositionY(int tEntityID, double tY)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	Position p = getMugenAnimationPosition(e->mAnimationID);
	p.y = tY;
	setMugenAnimationPosition(e->mAnimationID, p);
}

void setBlitzMugenAnimationVisibility(int tEntityID, int tVisibility)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	setMugenAnimationVisibility(e->mAnimationID, tVisibility);
}

void setBlitzMugenAnimationColor(int tEntityID, double tR, double tG, double tB)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	setMugenAnimationColor(e->mAnimationID, tR, tG, tB);
}

void setBlitzMugenAnimationBaseDrawScale(int tEntityID, double tScale)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	setMugenAnimationBaseDrawScale(e->mAnimationID, tScale);
}

void setBlitzMugenAnimationAngle(int tEntityID, double tRotation)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	setMugenAnimationDrawAngle(e->mAnimationID, tRotation);
}

void setBlitzMugenAnimationCallback(int tEntityID, void(*tFunc)(void *), void * tCaller)
{
	BlitzAnimationEntry* e = getBlitzAnimationEntry(tEntityID);
	setMugenAnimationCallback(e->mAnimationID, tFunc, tCaller);
}
