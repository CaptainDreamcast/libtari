#include "tari/memoryhandler.h"

#include "tari/math.h"

int getAvailableTextureMemory() {
	return INF;
}

void initMemoryHandlerHW() {}
void increaseAvailableTextureMemoryHW(size_t tSize) {
	(void)tSize;
}
void decreaseAvailableTextureMemoryHW(size_t tSize) {
	(void)tSize;
}