#include "prism/texture.h"

#include "prism/file.h"
#include "prism/log.h"
#include "prism/system.h"

#define FONT_CHARACTER_AMOUNT 91

static int isFontDataLoaded;
static TextureData gFont;
static FontCharacterData gFontCharacterData[FONT_CHARACTER_AMOUNT];

void unloadFont() {
	if (!isFontDataLoaded)
		return;

	unloadTexture(gFont);
	memset(gFontCharacterData, 0, sizeof gFontCharacterData);

	isFontDataLoaded = 0;
}

void loadFontHeader(char tFileDir[]) {
	FileHandler file;

	file = fileOpen(tFileDir, O_RDONLY);

	if (file == FILEHND_INVALID) {
		logError("Cannot open font header.");
		logErrorString(tFileDir);
		recoverFromError();
	}

	fileSeek(file, 0, 0);
	int i;
	for (i = 0; i < FONT_CHARACTER_AMOUNT; i++) {
		fileRead(file, &gFontCharacterData[i], sizeof gFontCharacterData[i]);
	}

	fileClose(file);
}

void loadFontTexture(char tFileDir[]) {
	gFont = loadTexturePKG(tFileDir);
}

void setFont(char tFileDirHeader[], char tFileDirTexture[]) {
	if (isFontDataLoaded) {
		unloadFont();
	}

	if (!isFile(tFileDirHeader)) {
		return;
	}

	loadFontHeader(tFileDirHeader);
	loadFontTexture(tFileDirTexture);

	isFontDataLoaded = 1;
}

void loadConsecutiveTextures(TextureData * tDst, char * tBaseFileDir, int tAmount)
{
	int i;
	for (i = 0; i < tAmount; i++) {
		char path[1024];
		getPathWithNumberAffixedFromAssetPath(path, tBaseFileDir, i);
		tDst[i] = loadTexture(path);
	}
}

TextureData getFontTexture() {
	return gFont;
}

FontCharacterData getFontCharacterData(char tChar) {
	int i;
	if (tChar < ' ' || tChar > 'z')
		i = 0;
	else
		i = tChar - ' ';

	return gFontCharacterData[i];
}

TextureSize makeTextureSize(int x, int y) {
	TextureSize ret;
	ret.x = x;
	ret.y = y;
	return ret;
}

TextureData createWhiteTexture() {
	int length = 16 * 16 * 4;
	uint8_t* data = allocMemory(length);
	memset(data, 0xFF, length);

	TextureData ret = loadTextureFromARGB32Buffer(makeBuffer(data, length), 16, 16);

	freeMemory(data);
	return ret;
}

Buffer turnARGB32BufferIntoARGB16Buffer(Buffer tSrc) {
	int dstSize = tSrc.mLength / 2;
	char* dst = allocMemory(dstSize);
	char* src = tSrc.mData;

	int n = dstSize / 2;
	int i;
	for(i = 0; i < n; i++) {
		int srcPos = 4*i;
		int dstPos = 2*i;

		uint8_t a = ((uint8_t)src[srcPos + 3]) >> 4;
		uint8_t r = ((uint8_t)src[srcPos + 2]) >> 4;
		uint8_t g = ((uint8_t)src[srcPos + 1]) >> 4;
		uint8_t b = ((uint8_t)src[srcPos + 0]) >> 4;
		
		dst[dstPos + 0] = (g << 4) | b;
		dst[dstPos + 1] = (a << 4) | r;
	}


	return makeBufferOwned(dst, dstSize);
}
