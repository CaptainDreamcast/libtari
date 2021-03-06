#include "prism/soundeffect.h"

#include "prism/memoryhandler.h"
#include "prism/file.h"
#include "prism/math.h"

SoundEffectCollection loadConsecutiveSoundEffectsToCollection(const char* tPath, int tAmount) {
	SoundEffectCollection ret;
	ret.mAmount = tAmount;
	ret.mSoundEffects = (int*)allocMemory(tAmount*sizeof(int));
	loadConsecutiveSoundEffects(ret.mSoundEffects, tPath, tAmount);
	return ret;
}

void loadConsecutiveSoundEffects(int * tDst, const char * tPath, int tAmount)
{
	int i;
	for (i = 0; i < tAmount; i++) {
		char path[1024];
		getPathWithNumberAffixedFromAssetPath(path, tPath, i);
		tDst[i] = loadSoundEffect(path);
	}
}

int playRandomSoundEffectFromCollection(const SoundEffectCollection& tCollection)
{
	int i;
	i = randfromInteger(0, tCollection.mAmount - 1);
	return playSoundEffect(tCollection.mSoundEffects[i]);
}
