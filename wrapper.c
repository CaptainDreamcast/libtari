#include "tari/wrapper.h"

#include "tari/pvr.h"
#include "tari/physics.h"
#include "tari/file.h"
#include "tari/drawing.h"
#include "tari/log.h"
#include "tari/memoryhandler.h"
#include "tari/sound.h"

#include "tari/timer.h"
#include "tari/animation.h"
#include "tari/input.h"
#include "tari/physicshandler.h"
#include "tari/stagehandler.h"
#include "tari/collisionhandler.h"
#include "tari/collisionanimation.h"
#include "tari/soundeffect.h"
#include "tari/system.h"
#include "tari/texturepool.h"
#include "tari/texthandler.h"
#include "tari/screeneffect.h"

void initTariWrapperWithDefaultFlags() {
	logg("Initiating wrapper.");
	initSystem();
	initiatePVR();
	initMemoryHandler();
	initPhysics();
	initFileSystem();
	initDrawing();
	initSound();
	initSoundEffects();
	setFont("$/rd/fonts/dolmexica.hdr", "$/rd/fonts/dolmexica.pkg");
	initScreenEffects();
	
}
void shutdownTariWrapper() {
	shutdownSound();
	shutdownMemoryHandler();
	shutdownSystem();
}

void pauseWrapper() {
	pausePhysics();
	pauseDurationHandling();
}

void resumeWrapper() {
	resumePhysics();
	resumeDurationHandling();
}

static struct {
	int mIsAborted;
} gData;

static void loadScreen(Screen* tScreen) {
	logg("Loading handled screen");
	logg("Pushing memory stacks");
	pushMemoryStack();
	pushTextureMemoryStack();
	logg("Setting up Timer");
	setupTimer();
	logg("Setting up Texture pool");
	setupTexturePool();
	logg("Setting up Animationhandling");
	setupAnimationHandler();
	logg("Setting up Texthandling");
	setupTextHandler();
	logg("Setting up Physicshandling");
	setupPhysicsHandler();
	logg("Setting up Stagehandling");
	setupStageHandler();
	logg("Setting up Collisionhandling");
	setupCollisionHandler();
	logg("Setting up Collisionanimationhandling");
	setupCollisionAnimationHandler();
	logg("Setting up Soundeffecthandling");
	setupSoundEffectHandler();
	logg("Setting up input flanks");
	resetInput();
	
	if (tScreen->mLoad) {
		logg("Loading user screen data");
		tScreen->mLoad();
	}
}

static void unloadScreen(Screen* tScreen) {
	logg("Unloading handled screen");
	
	if (tScreen->mUnload) {
		logg("Unloading user screen data");
		tScreen->mUnload();
	}

	logg("Shutting down Soundeffecthandling");
	shutdownSoundEffectHandler();
	logg("Shutting down Collisionanimationhandling");
	shutdownCollisionAnimationHandler();
	logg("Shutting down Collisionhandling");
	shutdownCollisionHandler();
	logg("Shutting down Stagehandling");
	shutdownStageHandler();
	logg("Shutting down Physicshandling");
	shutdownPhysicsHandler();
	logg("Shutting down Texthandling");
	shutdownTextHandler();
	logg("Shutting down Animationhandling");
	shutdownAnimationHandler();
	logg("Shutting down Texture pool");
	shutdownTexturePool();
	logg("Shutting down Timer");
	shutdownTimer();
	logg("Popping Memory Stacks");
	popTextureMemoryStack();
	popMemoryStack();

	logMemoryState();
	logTextureMemoryState();
}

static void updateScreen(Screen* tScreen) {
	updateSystem();
	updateInput();
	updatePhysicsHandler();
	updateAnimationHandler();
	updateTextHandler();
	updateStageHandler();
	updateCollisionAnimationHandler();
	updateCollisionHandler();
	updateTimer();

	if (tScreen->mUpdate) {
		tScreen->mUpdate();
	}
}

static void drawScreen(Screen* tScreen) {
	waitForScreen();
	startDrawing();
	drawHandledAnimations();
	drawHandledTexts();
	drawHandledCollisions();

	if (tScreen->mDraw) {
		tScreen->mDraw();
	}

	stopDrawing();
}

static Screen* showScreen(Screen* tScreen) {
	logg("Show screen");

	Screen* next = NULL;
	while(!gData.mIsAborted && next == NULL) {
		updateScreen(tScreen);
		drawScreen(tScreen);
		next = tScreen->mGetNextScreen();	
	}

	return next;
}

void abortScreenHandling() {
	gData.mIsAborted = 1;
}

void startScreenHandling(Screen* tScreen) {
	gData.mIsAborted = 0;

	while(!gData.mIsAborted) {
	
		if (!tScreen->mGetNextScreen) {
			logError("GetNextScreen not set.");
			abortSystem();
		}

		loadScreen(tScreen);
		Screen* next = showScreen(tScreen);
		unloadScreen(tScreen);
		tScreen = next;
	}
	
}
