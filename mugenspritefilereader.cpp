#include "prism/mugenspritefilereader.h"

#include <assert.h>
#include <string.h>
#include <algorithm>

#ifdef DREAMCAST
#include <png/png.h>
#else
#include <png.h>
#endif

#include "prism/file.h"
#include "prism/log.h"
#include "prism/system.h"
#include "prism/texture.h"
#include "prism/math.h"
#include "prism/drawing.h"
#include "prism/compression.h"

#include "prism/lz5.h"

using namespace std;

typedef struct {
	char mSignature[12];
	uint8_t mVersion[4];
} SFFSharedHeader;

typedef struct {
	char mSignature[12];
	char mVersion[4];
	
	uint32_t mReserved1;
	uint32_t mReserved2;

	uint8_t mCompatibleVersion[4];

	uint32_t mReserved3;
	uint32_t mReserved4;

	uint32_t mSpriteOffset;
	uint32_t mSpriteTotal;

	uint32_t mPaletteOffset;
	uint32_t mPaletteTotal;

	uint32_t mLDataOffset;
	uint32_t mLDataLength;

	uint32_t mTDataOffset;
	uint32_t mTDataLength;

	uint32_t mReserved5;
	uint32_t mReserved6;

	char mComments[436];
} SFFHeader2;

typedef struct {
	uint16_t mGroupNo;
	uint16_t mItemNo;
	uint16_t mNumCols;
	uint16_t mIndex;

	uint32_t mDataOffset;
	uint32_t mDataLength;
} SFFPalette2;

typedef struct {
	uint16_t mGroupNo;
	uint16_t mItemNo;
	uint16_t mWidth;
	uint16_t mHeight;
	
	int16_t mAxisX;
	int16_t mAxisY;

	uint16_t mIndex;

	uint8_t mFormat;
	uint8_t mColorDepth;

	uint32_t mDataOffset;
	uint32_t mDataLength;

	uint16_t mPaletteIndex;
	uint16_t mFlags;
} SFFSprite2;

typedef struct {
	char mSignature[12];
	char mVersion[4];
	int32_t mGroupAmount;
	int32_t mImageAmount;
	int32_t mFirstFileOffset;
	int32_t mSubheaderSize;
	int8_t mPaletteType;
	char mBlank[3];
	char mComments[476];
} SFFHeader;


typedef struct {
	int32_t mNextFilePosition;
	int32_t mSubfileLength;
	int16_t mImageAxisXCoordinate;
	int16_t mImageAxisYCoordinate;
	int16_t mGroup;
	int16_t mImage;
	int16_t mIndexOfPreciousSpriteCopy;
	int8_t mHasSamePaletteAsPreviousImage;
	char mComments[12];
} SFFSubFileHeader;

typedef struct {
	uint8_t mZSoftID;
	uint8_t mVersion;
	uint8_t mEncoding;
	uint8_t mBitsPerPixel;

	int16_t mMinX;
	int16_t mMinY;
	int16_t mMaxX;
	int16_t mMaxY;

	int16_t mHorizontalResolution;
	int16_t mVerticalResolution;

	uint8_t mHeaderPalette[48];
	uint8_t mReserved;
	uint8_t mPlaneAmount;
	uint16_t mBytesPerLine;
	uint16_t mPaletteInfo;
	char mFiller[58];
} PCXHeader;

typedef struct MugenSpriteFileReader_internal {
	int mType;
	void(*mInit)(struct MugenSpriteFileReader_internal* tReader, char* tPath);
	void(*mRead)(struct MugenSpriteFileReader_internal* tReader, void* tDst, uint32_t tSize);
	Buffer(*mReadBufferReadOnly)(struct MugenSpriteFileReader_internal* tReader, uint32_t tSize);
	void(*mSeek)(struct MugenSpriteFileReader_internal* tReader, uint32_t tPosition);
	uint32_t(*mGetCurrentOffset)(struct MugenSpriteFileReader_internal* tReader);
	void(*mDelete)(struct MugenSpriteFileReader_internal* tReader);
	void(*mSetOver)(struct MugenSpriteFileReader_internal* tReader);
	int(*mIsOver)(struct MugenSpriteFileReader_internal* tReader);
	void* mData;
} MugenSpriteFileReader;

static struct {
	int mIsOnlyLoadingPortraits;
	int mHasPaletteFile;
	int mIsUsingRealPalette;
	int mPaletteID;
	MugenSpriteFileReader mReader;

	TextureData(*mCustomLoadTextureFromARGB16Buffer)(Buffer, int, int);
	TextureData(*mCustomLoadTextureFromARGB32Buffer)(Buffer, int, int);
	TextureData(*mCustomLoadPalettedTextureFrom8BitBuffer)(Buffer, int, int, int);
	int mIsWritingDreamcastOptimized;
} gData;

static uint32_t get2DBufferIndex(uint32_t i, uint32_t j, uint32_t w) {
	return j * w + i;
}

static TextureData loadTextureFromPalettedImageData1bppTo16ARGB(Buffer tPCXImageBuffer, Buffer tPaletteBuffer, int w, int h) {
	uint8_t* output = (uint8_t*)allocMemory(w*h * 2);
	uint8_t* img = (uint8_t*)tPCXImageBuffer.mData;
	uint8_t* pal = (uint8_t*)tPaletteBuffer.mData;

	int i, j;
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			uint8_t pid = img[get2DBufferIndex(i, j, w)];
			uint8_t a = ((uint8_t)(pid == 0 ? 0 : 0xFF)) >> 4;
			uint8_t r = ((uint8_t)pal[pid * 3 + 0]) >> 4;
			uint8_t g = ((uint8_t)pal[pid * 3 + 1]) >> 4;
			uint8_t b = ((uint8_t)pal[pid * 3 + 2]) >> 4;

			output[get2DBufferIndex(i, j, w) * 2 + 0] = (g << 4) | b;
			output[get2DBufferIndex(i, j, w) * 2 + 1] = (a << 4) | r;
		}
	}

	Buffer b = makeBufferOwned(output, w*h * 2);

	TextureData ret;
	if (gData.mCustomLoadTextureFromARGB16Buffer) {
		ret = gData.mCustomLoadTextureFromARGB16Buffer(b, w, h);

	}
	else {
		ret = loadTextureFromARGB16Buffer(b, w, h);
	}

	freeBuffer(b);

	return ret;
}

static TextureData loadTextureFromPalettedImageData1bppTo32ARGB(Buffer tPCXImageBuffer, Buffer tPaletteBuffer, int w, int h) {
	uint8_t* output = (uint8_t*)allocMemory(w*h * 4);
	uint8_t* img = (uint8_t*)tPCXImageBuffer.mData;
	uint8_t* pal = (uint8_t*)tPaletteBuffer.mData;

	int i, j;
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			uint8_t pid = img[get2DBufferIndex(i, j, w)];
			output[get2DBufferIndex(i, j, w) * 4 + 0] = pal[pid * 3 + 2];
			output[get2DBufferIndex(i, j, w) * 4 + 1] = pal[pid * 3 + 1];
			output[get2DBufferIndex(i, j, w) * 4 + 2] = pal[pid * 3 + 0];
			output[get2DBufferIndex(i, j, w) * 4 + 3] = pid == 0 ? 0 : 0xFF;
		}
	}

	Buffer b = makeBufferOwned(output, w*h * 4);

	TextureData ret;
	if (gData.mCustomLoadTextureFromARGB32Buffer) {
		ret = gData.mCustomLoadTextureFromARGB32Buffer(b, w, h);
	}
	else {
		ret = loadTextureFromARGB32Buffer(b, w, h);
	}

	freeBuffer(b);

	return ret;
}

static TextureData loadTextureFromPalettedImageData1bpp(Buffer tPCXImageBuffer, Buffer tPaletteBuffer, int w, int h) {
	if (isOnDreamcast() || gData.mIsWritingDreamcastOptimized) {
		return loadTextureFromPalettedImageData1bppTo16ARGB(tPCXImageBuffer, tPaletteBuffer, w, h);
	}
	else {
		return loadTextureFromPalettedImageData1bppTo32ARGB(tPCXImageBuffer, tPaletteBuffer, w, h);
	}
}

typedef struct {
	Buffer mBuffer;
	Vector3DI mOffset;
	Vector3DI mSize;
} SubImageBuffer;

typedef struct {
	List* mDst;
	Buffer mPaletteBuffer;
} ApplyToSubImageListCaller;

static void loadTextureFromSinglePalettedImageListEntry1bpp(void* tCaller, void* tData) {
	ApplyToSubImageListCaller* caller = (ApplyToSubImageListCaller*)tCaller;
	SubImageBuffer* buffer = (SubImageBuffer*)tData;

	MugenSpriteFileSubSprite* newSprite = (MugenSpriteFileSubSprite*)allocMemory(sizeof(MugenSpriteFileSubSprite));
	newSprite->mOffset = buffer->mOffset;
	newSprite->mTexture = loadTextureFromPalettedImageData1bpp(buffer->mBuffer, caller->mPaletteBuffer, buffer->mSize.x, buffer->mSize.y);

	list_push_back_owned(caller->mDst, newSprite);
}

static List loadTextureFromPalettedImageList1bpp(List tBufferList, Buffer tPaletteBuffer) {
	List ret = new_list();

	ApplyToSubImageListCaller caller;
	caller.mDst = &ret;
	caller.mPaletteBuffer = tPaletteBuffer;

	list_map(&tBufferList, loadTextureFromSinglePalettedImageListEntry1bpp, &caller);

	return ret;
}


static void loadTextureFromSingleRawImageListEntry1bpp(void* tCaller, void* tData) {
	ApplyToSubImageListCaller* caller = (ApplyToSubImageListCaller*)tCaller;
	SubImageBuffer* buffer = (SubImageBuffer*)tData;

	MugenSpriteFileSubSprite* newSprite = (MugenSpriteFileSubSprite*)allocMemory(sizeof(MugenSpriteFileSubSprite));
	newSprite->mOffset = buffer->mOffset;
	if (gData.mCustomLoadPalettedTextureFrom8BitBuffer) {
		newSprite->mTexture = gData.mCustomLoadPalettedTextureFrom8BitBuffer(buffer->mBuffer, gData.mPaletteID, buffer->mSize.x, buffer->mSize.y);
	}
	else {
		newSprite->mTexture = loadPalettedTextureFrom8BitBuffer(buffer->mBuffer, gData.mPaletteID, buffer->mSize.x, buffer->mSize.y);
	}

	list_push_back_owned(caller->mDst, newSprite);
}

static List loadTextureFromRawImageList1bpp(List tBufferList) {
	List ret = new_list();

	ApplyToSubImageListCaller caller;
	caller.mDst = &ret;

	list_map(&tBufferList, loadTextureFromSingleRawImageListEntry1bpp, &caller);

	return ret;
}

static void loadTextureFromSingleImageARGB32ListEntry(void* tCaller, void* tData) {
	ApplyToSubImageListCaller* caller = (ApplyToSubImageListCaller*)tCaller;
	SubImageBuffer* buffer = (SubImageBuffer*)tData;

	MugenSpriteFileSubSprite* newSprite = (MugenSpriteFileSubSprite*)allocMemory(sizeof(MugenSpriteFileSubSprite));
	newSprite->mOffset = buffer->mOffset;

	if (gData.mCustomLoadTextureFromARGB32Buffer) {
		newSprite->mTexture = gData.mCustomLoadTextureFromARGB32Buffer(buffer->mBuffer, buffer->mSize.x, buffer->mSize.y);
	}
	else {
		newSprite->mTexture = loadTextureFromARGB32Buffer(buffer->mBuffer, buffer->mSize.x, buffer->mSize.y);
	}

	list_push_back_owned(caller->mDst, newSprite);
}

static List loadTextureFromImageARGB32List(List tBufferList) {
	List ret = new_list();

	ApplyToSubImageListCaller caller;
	caller.mDst = &ret;

	list_map(&tBufferList, loadTextureFromSingleImageARGB32ListEntry, &caller);

	return ret;
}


static Buffer decodeRLE5BufferAndReturnOwnedBuffer(Buffer b, int tFinalSize) {
	uint8_t* output = (uint8_t*)allocMemory(tFinalSize+10);
	uint8_t* input = (uint8_t*)b.mData;

	int ip;
	int op = 0;
	for (ip = 0; ip < (int)b.mLength; ip++) {
		uint8_t cur = input[ip];
	
		if ((cur & 0xC0) == 0xC0) { // decode
			int steps = cur & 0x3F;
			ip++;
			uint8_t val = input[ip];
			int k;
			
			for (k = 0; k < steps; k++) {
				output[op++] = val;
			}
			if (op >= tFinalSize + 1) break;
		}
		else {
			output[op++] = cur;
			if (op >= tFinalSize + 1) break;
		}

		
	}

	return makeBufferOwned(output, tFinalSize);
}

static Buffer decodeRLE8BufferAndReturnOwnedBuffer(Buffer b, int tFinalSize) {
	uint8_t* output = (uint8_t*)allocMemory(tFinalSize + 10);
	uint8_t* input = (uint8_t*)b.mData;

	uint32_t dstpos = 0;
	uint32_t srcpos = 0;

	while (srcpos < (uint32_t)b.mLength)
	{
		if (((input[srcpos] & 0xC0) == 0x40))
		{
			int run;
			for (run = 0; run < (input[srcpos] & 0x3F); run++)
			{
				output[dstpos] = input[srcpos + 1];
				dstpos++;
			}
			srcpos += 2;
		}
		else
		{
			output[dstpos] = input[srcpos];
			dstpos++;
			srcpos++;
		}
	}

	return makeBufferOwned(output, tFinalSize);
}

static MugenSpriteFileSprite* makeMugenSpriteFileSprite(List tTextures, TextureSize tOriginalTextureSize, Vector3D tAxisOffset) {
	MugenSpriteFileSprite* e = (MugenSpriteFileSprite*)allocMemory(sizeof(MugenSpriteFileSprite));
	e->mTextures = tTextures;
	e->mOriginalTextureSize = tOriginalTextureSize;
	e->mIsLinked = 0;
	e->mIsLinkedTo = 0;
	e->mAxisOffset = tAxisOffset;
	return e;
}

static MugenSpriteFileSprite* makeLinkedMugenSpriteFileSprite(int tIsLinkedTo, Vector3D tAxisOffset) {
	MugenSpriteFileSprite* e = (MugenSpriteFileSprite*)allocMemory(sizeof(MugenSpriteFileSprite));
	e->mIsLinked = 1;
	e->mIsLinkedTo = tIsLinkedTo;
	e->mAxisOffset = tAxisOffset;
	return e;
}

static Buffer* loadPCXPaletteToAllocatedBuffer(BufferPointer p, int tEncodedSize) {
	Buffer* insertPal = (Buffer*)allocMemory(sizeof(Buffer));
	
	int size = 256 * 3;
	char* data = (char*)allocMemory(size);
	memcpy(data, p + tEncodedSize, size);
	
	*insertPal = makeBufferOwned(data, size);
	return insertPal;
}

static SubImageBuffer* getSingleAllocatedBufferFromSource(Buffer b, int x, int y, int dx, int dy, int tWidth, int tHeight, int tBytesPerPixel) {
	int dstSize = dx*dy;
	char* dst = (char*)allocMemory(dstSize*tBytesPerPixel);
	char* src = (char*)b.mData;

	int j;
	for (j = 0; j < dy; j++) {
		int startIndexDst = get2DBufferIndex(0, j, dx)*tBytesPerPixel;
		int startIndexSrc = get2DBufferIndex(x, y + j, tWidth)*tBytesPerPixel;

		int rowSrcSize, rowBufferSize;
		if (y + j >= tHeight) rowSrcSize = 0;
		else rowSrcSize = min(dx, tWidth - x);
		rowBufferSize = (dx - rowSrcSize);
		rowSrcSize *= tBytesPerPixel;
		rowBufferSize *= tBytesPerPixel;

		memcpy(dst + startIndexDst, src + startIndexSrc, rowSrcSize);
		memset(dst + startIndexDst + rowSrcSize, 0, rowBufferSize);
	}

	SubImageBuffer* ret = (SubImageBuffer*)allocMemory(sizeof(SubImageBuffer));
	ret->mBuffer = makeBufferOwned(dst, dstSize*tBytesPerPixel);
	ret->mOffset = makeVector3DI(x, y, 0);
	ret->mSize = makeVector3DI(dx, dy, 0);
	return ret;
}

static int getMaximumSizeFit(int tVal) {
	int mini = 32;
	int maxi = 512;

	if (tVal <= mini) return mini;

	int i = mini;
	while(i < 5000) {
		if (i <= tVal && (i * 2 > tVal || i == maxi)) {
			return i;
		}

		i *= 2;
	}

	logError("Unable to find fit");
	logErrorInteger(tVal);
	recoverFromError();
	return 0;
}

List breakImageBufferUpIntoMultipleBuffers(Buffer b, int tWidth, int tHeight, int tBytesPerPixel) {
	List ret = new_list();

	int y = 0;
	while (y < tHeight) { // INFO: Need power of 2 split for paletted images on Dreamcast, they need to be twiddled and twiddling only works with power of 2
		int heightLeft = tHeight - y;
		int dy = getMaximumSizeFit(heightLeft);
		int x = 0;
		while (x < tWidth) {
			int widthLeft = tWidth - x;
			int dx = getMaximumSizeFit(widthLeft);
			SubImageBuffer* newBuffer = getSingleAllocatedBufferFromSource(b, x, y, dx, dy, tWidth, tHeight, tBytesPerPixel);
			list_push_back_owned(&ret, newBuffer);
			x += dx;
		}
		y += dy;
	}

	return ret;
}

static int removeSingleSubImageBufferEntryCB(void* tCaller, void* tData) {
	(void)tCaller;
	SubImageBuffer* buffer = (SubImageBuffer*)tData;
	freeBuffer(buffer->mBuffer);

	return 1;
}

static void freeSubImageBufferList(List tList) {
	list_remove_predicate(&tList, removeSingleSubImageBufferEntryCB, NULL);
}

typedef struct {
	BufferPointer p;
	Buffer b;
} PNGReadCaller;

static void readPNGDataFromInputStream(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead) {
	png_voidp io_ptr = png_get_io_ptr(png_ptr);
	if (io_ptr == NULL) {
		logError("Did not get caller");
		recoverFromError();
	}


	PNGReadCaller* caller = (PNGReadCaller*)io_ptr;
	if (((uint32_t)caller->p) + byteCountToRead > ((uint32_t)caller->b.mData) + caller->b.mLength) {
		logError("Trying to read outside buffer");
		recoverFromError();
	}
	readFromBufferPointer(outBytes, &caller->p, byteCountToRead);
}

static Buffer parseRGBAPNG(png_structp* png_ptr, png_infop* info_ptr, int tHasAlpha, int width, int height)
{
	uint8_t* dst = (uint8_t*)allocMemory(width*height * 4);

	const png_uint_32 bytesPerRow = png_get_rowbytes(*png_ptr, *info_ptr);
	char* rowData = (char*)allocMemory(bytesPerRow);

	// read single row at a time
	uint32_t rowIdx;
	for (rowIdx = 0; rowIdx < (uint32_t)height; ++rowIdx)
	{
		png_read_row(*png_ptr, (png_bytep)rowData, NULL);

		uint32_t rowOffset = rowIdx * width;

		uint32_t byteIndex = 0;
		uint32_t colIdx;
		for (colIdx = 0; colIdx < (uint32_t)width; ++colIdx)
		{
			uint32_t targetPixelIndex = rowOffset + colIdx;
			dst[targetPixelIndex * 4 + 2] = rowData[byteIndex++];
			dst[targetPixelIndex * 4 + 1] = rowData[byteIndex++];
			dst[targetPixelIndex * 4 + 0] = rowData[byteIndex++];
			dst[targetPixelIndex * 4 + 3] = tHasAlpha ? rowData[byteIndex++] : 255;
		}
		assert(byteIndex == bytesPerRow);
	}

	freeMemory(rowData);

	return makeBufferOwned(dst, width*height*4);
}

static png_colorp gPalette[256 * 3];

static Buffer parsePalettedPNG(png_structp* png_ptr, png_infop* info_ptr, int width, int height)
{
	uint8_t* dst = (uint8_t*)allocMemory(width*height * 4);

	png_uint_32 bytesPerRow = png_get_rowbytes(*png_ptr, *info_ptr);
	uint8_t* rowData = (uint8_t*)allocMemory(bytesPerRow);

	int palAmount;
	png_get_PLTE(*png_ptr, *info_ptr, gPalette, &palAmount);
	assert(palAmount <= 256 * 3);

	uint32_t rowIdx;
	for (rowIdx = 0; rowIdx < (uint32_t)height; ++rowIdx)
	{
		png_read_row(*png_ptr, (png_bytep)rowData, NULL);

		uint32_t rowOffset = rowIdx * width;
		uint32_t byteIndex = 0;
		uint32_t colIdx;
		for (colIdx = 0; colIdx < (uint32_t)width; ++colIdx)
		{
			uint32_t targetPixelIndex = rowOffset + colIdx;
			int index = rowData[byteIndex++];
			assert(index < palAmount);
			if (gPalette[index]) { // TODO: find out what's going on
				dst[targetPixelIndex * 4 + 2] = gPalette[index]->red;
				dst[targetPixelIndex * 4 + 1] = gPalette[index]->green;
				dst[targetPixelIndex * 4 + 0] = gPalette[index]->blue;
				dst[targetPixelIndex * 4 + 3] = 255;
			}
			else {
				dst[targetPixelIndex * 4 + 3] = 0;
			}
		}
		assert(byteIndex == bytesPerRow);
	}

	freeMemory(rowData);

	return makeBufferOwned(dst, width*height * 4);
}

static Buffer loadARGB32BufferFromRawPNGBuffer(Buffer tRawPNGBuffer, int tWidth, int tHeight) {
	BufferPointer p = getBufferPointer(tRawPNGBuffer);
	
	uint8_t* pngSignature = (uint8_t*)p;
	p += 8;
	if (!png_check_sig(pngSignature, 8)) {
		logError("Invalid png signature");
		recoverFromError();
	}

	png_structp png_ptr = NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		logError("Cannot create read struct.");
		recoverFromError();
	}

	png_infop info_ptr = NULL;
	info_ptr = png_create_info_struct(png_ptr);

	if (info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		logError("Cannot create info struct.");
		recoverFromError();
	}

	PNGReadCaller caller;
	caller.p = p;
	caller.b = tRawPNGBuffer;

	png_set_read_fn(png_ptr, &caller, readPNGDataFromInputStream);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	png_uint_32 width = 0;
	png_uint_32 height = 0;
	int bitDepth = 0;
	int colorType = -1;
	png_uint_32 retval = png_get_IHDR(png_ptr, info_ptr,
		&width,
		&height,
		&bitDepth,
		&colorType,
		NULL, NULL, NULL);
	assert((int)width == tWidth);
	assert((int)height == tHeight);

	if (retval != 1) {
		logError("Unable to read image data");
		recoverFromError();
	}

	Buffer ret;
	if (colorType == PNG_COLOR_TYPE_RGB) {
		ret = parseRGBAPNG(&png_ptr, &info_ptr, 0, width, height);
	}
	else if (colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
		ret = parseRGBAPNG(&png_ptr, &info_ptr, 1, width, height);
	}
	else if (colorType == PNG_COLOR_TYPE_PALETTE) {
		ret = parsePalettedPNG(&png_ptr, &info_ptr, width, height);
	}
	else {
		logError("Unrecognized color type");
		logErrorInteger(colorType);
		recoverFromError();
		ret = makeBuffer(NULL, 0);
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	return ret;
}

static MugenSpriteFileSprite* makeMugenSpriteFileSpriteFromRawPNGBuffer(Buffer tRawPNGBuffer, int tWidth, int tHeight, int tBytesPerLine, Vector3D tAxisOffset) {

	Buffer argb32Buffer = loadARGB32BufferFromRawPNGBuffer(tRawPNGBuffer, tBytesPerLine, tHeight);
	freeBuffer(tRawPNGBuffer);

	List subImageList = breakImageBufferUpIntoMultipleBuffers(argb32Buffer, tBytesPerLine, tHeight, 4);
	freeBuffer(argb32Buffer);

	List textures = loadTextureFromImageARGB32List(subImageList);
	freeSubImageBufferList(subImageList);

	return makeMugenSpriteFileSprite(textures, makeTextureSize(tWidth, tHeight), tAxisOffset);
}

static MugenSpriteFileSprite* makeMugenSpriteFileSpriteFromRawAndPaletteBufferGeneral(Buffer tRawImageBuffer, Buffer tPaletteBuffer, int tIsUsingPaletteBuffer, int tWidth, int tHeight, int tBytesPerLine, Vector3D tAxisOffset) {
	List subImagelist = breakImageBufferUpIntoMultipleBuffers(tRawImageBuffer, tBytesPerLine, tHeight, 1);
	freeBuffer(tRawImageBuffer);

	List textures;
	if (tIsUsingPaletteBuffer) {
		textures = loadTextureFromPalettedImageList1bpp(subImagelist, tPaletteBuffer);
	}
	else {
		textures = loadTextureFromRawImageList1bpp(subImagelist);
	}
	freeSubImageBufferList(subImagelist);

	return makeMugenSpriteFileSprite(textures, makeTextureSize(tWidth, tHeight), tAxisOffset);
}

static MugenSpriteFileSprite* makeMugenSpriteFileSpriteFromRawAndPaletteBuffer(Buffer tRawImageBuffer, Buffer tPaletteBuffer, int tWidth, int tHeight, int tBytesPerLine, Vector3D tAxisOffset) {
	return makeMugenSpriteFileSpriteFromRawAndPaletteBufferGeneral(tRawImageBuffer, tPaletteBuffer, 1, tWidth, tHeight, tBytesPerLine, tAxisOffset);
}

static MugenSpriteFileSprite* makeMugenSpriteFileSpriteFromRawBuffer(Buffer tRawImageBuffer, int tWidth, int tHeight, int tBytesPerLine, Vector3D tAxisOffset) {
	return makeMugenSpriteFileSpriteFromRawAndPaletteBufferGeneral(tRawImageBuffer, makeBuffer(NULL, 0), 0, tWidth, tHeight, tBytesPerLine, tAxisOffset);
}

static void insertPaletteIntoMugenSpriteFile(MugenSpriteFile* tSprites, Buffer* b) {
	assert(b->mLength == 256 * 3);

	if (gData.mIsUsingRealPalette && !vector_size(&tSprites->mPalettes)) {
		setPaletteFromBGR256WithFirstValueTransparentBuffer(gData.mPaletteID, *b);
	}

	vector_push_back_owned(&tSprites->mPalettes, b);
}

 
static MugenSpriteFileSprite* loadTextureFromPCXBuffer(MugenSpriteFile* tDst, int mIsUsingOwnPalette, Buffer b, Vector3D tAxisOffset) { // TODO: refactor
	PCXHeader header;
	

	BufferPointer p = getBufferPointer(b);

	readFromBufferPointer(&header, &p, sizeof(PCXHeader));

	int bytesPerPixel = header.mBitsPerPixel / 8;
	int w = header.mMaxX - header.mMinX + 1;
	int h = header.mMaxY - header.mMinY + 1;
	int32_t pcxImageSize = bytesPerPixel*header.mBytesPerLine*h;
	
	assert(header.mBitsPerPixel == 8);
	assert(header.mEncoding == 1);
	if (header.mPlaneAmount != 1) {
		logWarningFormat("Unsupported pcx plane amount: %d", header.mPlaneAmount);
	}

	int encodedSize;
	if (mIsUsingOwnPalette) {
		encodedSize = b.mLength - sizeof(PCXHeader) - 256 * 3;
	}
	else {
		encodedSize = b.mLength - sizeof(PCXHeader);
	}

	Buffer encodedImageBuffer = makeBuffer(p, encodedSize);
	Buffer rawImageBuffer = decodeRLE5BufferAndReturnOwnedBuffer(encodedImageBuffer, pcxImageSize);

	if (mIsUsingOwnPalette) {
		Buffer* insertPalette = loadPCXPaletteToAllocatedBuffer(p, encodedSize);
		insertPaletteIntoMugenSpriteFile(tDst, insertPalette);
	}
	assert(vector_size(&tDst->mPalettes));

	if (gData.mIsUsingRealPalette && vector_size(&tDst->mPalettes) == 1) {
		return makeMugenSpriteFileSpriteFromRawBuffer(rawImageBuffer, w, h, header.mBytesPerLine, tAxisOffset);
	}
	else {
		Buffer* paletteBuffer = (Buffer*)vector_get_back(&tDst->mPalettes);
		return makeMugenSpriteFileSpriteFromRawAndPaletteBuffer(rawImageBuffer, *paletteBuffer, w, h, header.mBytesPerLine, tAxisOffset);
	}
	
}

// TODO: refactor+simplify
static MugenSpriteFile makeEmptySpriteFile();
static void unloadSinglePalette(void* tCaller, void* tData);

MugenSpriteFileSprite* loadSingleTextureFromPCXBuffer(Buffer tBuffer) {
	MugenSpriteFile sprites = makeEmptySpriteFile();

	MugenSpriteFileSprite* sprite = loadTextureFromPCXBuffer(&sprites, 1, tBuffer, makePosition(0, 0, 0));
	delete_int_map(&sprites.mGroups);
	delete_vector(&sprites.mAllSprites);
	vector_map(&sprites.mPalettes, unloadSinglePalette, NULL);
	delete_vector(&sprites.mPalettes);

	return sprite;
}



static void insertTextureIntoSpriteFile(MugenSpriteFile* tDst, MugenSpriteFileSprite* tTexture, int tGroup, int tSprite) {

	if (!int_map_contains(&tDst->mGroups, tGroup)) {
		MugenSpriteFileGroup* group = (MugenSpriteFileGroup*)allocMemory(sizeof(MugenSpriteFileGroup));
		group->mSprites = new_int_map();
		int_map_push_owned(&tDst->mGroups, tGroup, group);
	}

	MugenSpriteFileGroup* g = (MugenSpriteFileGroup*)int_map_get(&tDst->mGroups, tGroup);
	
	int_map_push_owned(&g->mSprites, tSprite, tTexture);
	vector_push_back(&tDst->mAllSprites, tTexture);
}

static int gPreviousGroup;

static void loadSingleSFFFileAndInsertIntoSpriteFile(SFFSubFileHeader subHeader, MugenSpriteFile* tDst) {

	MugenSpriteFileSprite* texture;

	if (subHeader.mSubfileLength) {
		Buffer pcxBuffer = gData.mReader.mReadBufferReadOnly(&gData.mReader, subHeader.mSubfileLength);
		int isFirstImageWithActiveActFile = gData.mHasPaletteFile && subHeader.mGroup == 0 && subHeader.mImage == 0; 
		int isUsingOwnPalette = !subHeader.mHasSamePaletteAsPreviousImage && !isFirstImageWithActiveActFile;
		gPreviousGroup = subHeader.mGroup;
		texture = loadTextureFromPCXBuffer(tDst, isUsingOwnPalette, pcxBuffer, makePosition(subHeader.mImageAxisXCoordinate, subHeader.mImageAxisYCoordinate, 0));
		freeBuffer(pcxBuffer);
	}
	else {
		texture = makeLinkedMugenSpriteFileSprite(subHeader.mIndexOfPreciousSpriteCopy, makePosition(subHeader.mImageAxisXCoordinate, subHeader.mImageAxisYCoordinate, 0));
	}

	insertTextureIntoSpriteFile(tDst, texture, subHeader.mGroup, subHeader.mImage);
}

static void removeAllPalettesExceptFirst(MugenSpriteFile* tDst) {
	assert(vector_size(&tDst->mPalettes) >= 1);

	while (vector_size(&tDst->mPalettes) > 1) {
		Buffer* b = (Buffer*)vector_get_back(&tDst->mPalettes);
		freeBuffer(*b);
		vector_pop_back(&tDst->mPalettes);
	}
}

static void loadSingleSFFFile(MugenSpriteFile* tDst) {
	SFFSubFileHeader subHeader;

	gData.mReader.mRead(&gData.mReader, &subHeader, sizeof(SFFSubFileHeader));

	debugInteger(sizeof(SFFSubFileHeader));
	debugPointer(subHeader.mNextFilePosition);
	debugInteger(subHeader.mSubfileLength);
	debugInteger(subHeader.mGroup);
	debugInteger(subHeader.mImage);
	debugString(subHeader.mComments);

	if (gData.mHasPaletteFile && subHeader.mGroup == 0 && subHeader.mImage == 0) {
		removeAllPalettesExceptFirst(tDst);
	}

	if (!gData.mIsOnlyLoadingPortraits || (subHeader.mGroup == 9000 && subHeader.mImage <= 1)) { // TODO: non-hardcoded
		loadSingleSFFFileAndInsertIntoSpriteFile(subHeader, tDst);
	}

	if (!subHeader.mNextFilePosition) {
		gData.mReader.mSetOver(&gData.mReader);
	}
	else {
		gData.mReader.mSeek(&gData.mReader, subHeader.mNextFilePosition);		
	}

}

static void loadSFFHeader(SFFHeader* tHeader) {
	gData.mReader.mRead(&gData.mReader, tHeader, sizeof(SFFHeader));
}

static MugenSpriteFile makeEmptySpriteFile() {
	MugenSpriteFile ret;
	ret.mGroups = new_int_map();
	ret.mAllSprites = new_vector();
	ret.mPalettes = new_vector();
	return ret;
}

static void loadMugenSpriteFilePaletteFile(MugenSpriteFile* ret, char* tPath) {
	assert(isFile(tPath));

	Buffer* b = (Buffer*)allocMemory(sizeof(Buffer));
	*b = fileToBuffer(tPath);
	Buffer src = fileToBuffer(tPath);

	int i;
	for (i = 0; i < 256; i++) {
		int j = 255 - i;
		((uint8_t*)b->mData)[j * 3 + 0] = ((uint8_t*)src.mData)[i * 3 + 0];
		((uint8_t*)b->mData)[j * 3 + 1] = ((uint8_t*)src.mData)[i * 3 + 1];
		((uint8_t*)b->mData)[j * 3 + 2] = ((uint8_t*)src.mData)[i * 3 + 2];
	}

	freeBuffer(src);
	insertPaletteIntoMugenSpriteFile(ret, b);
}

static MugenSpriteFile loadMugenSpriteFile1(int tHasPaletteFile, char* tOptionalPaletteFile) {
	MugenSpriteFile ret = makeEmptySpriteFile();
	
	if (tHasPaletteFile) {
		loadMugenSpriteFilePaletteFile(&ret, tOptionalPaletteFile);
	}
	
	gData.mReader.mSeek(&gData.mReader, 0);

	SFFHeader header;
	loadSFFHeader(&header);

	debugInteger(header.mGroupAmount);
	debugInteger(header.mImageAmount);
	debugPointer(header.mFirstFileOffset);

	gPreviousGroup = -1;

	gData.mReader.mSeek(&gData.mReader, header.mFirstFileOffset);
	while (!gData.mReader.mIsOver(&gData.mReader)) {
		loadSingleSFFFile(&ret);
		if(gData.mIsOnlyLoadingPortraits && vector_size(&ret.mAllSprites) == 2) break;
	}

	return ret;
}

static void loadSFFHeader2(SFFHeader2* tHeader) {
	gData.mReader.mRead(&gData.mReader, tHeader, sizeof(SFFHeader2));
}

static Buffer processRawPalette2(Buffer tRaw) {
	int n = tRaw.mLength / 4;

	assert(n <= 256);
	char* raw = (char*)tRaw.mData;
	char* out = (char*)allocMemory(256 * 3);
	memset(out, 0, 256 * 3);

	int i = 0;
	for (i = 0; i < n; i++) {
		out[3 * i + 0] = raw[4 * i +  0];
		out[3 * i + 1] = raw[4 * i + 1];
		out[3 * i + 2] = raw[4 * i + 2];
	}


	return makeBufferOwned(out, 256 * 3);
}

static void loadSinglePalette2(SFFHeader2* tHeader, MugenSpriteFile* tDst) {
	SFFPalette2 palette;
	gData.mReader.mRead(&gData.mReader, &palette, sizeof(SFFPalette2));

	debugPointer(palette.mDataOffset);
	debugInteger(palette.mDataLength);
	debugInteger(palette.mIndex);

	uint32_t originalPosition = gData.mReader.mGetCurrentOffset(&gData.mReader);
	gData.mReader.mSeek(&gData.mReader, tHeader->mLDataOffset + palette.mDataOffset);
	Buffer rawPalette = gData.mReader.mReadBufferReadOnly(&gData.mReader, palette.mDataLength);
	Buffer* processedPalette = (Buffer*)allocMemory(sizeof(Buffer));
	*processedPalette = processRawPalette2(rawPalette);
	freeBuffer(rawPalette);
	gData.mReader.mSeek(&gData.mReader, originalPosition);

	vector_push_back_owned(&tDst->mPalettes, processedPalette);

}

static void loadPalettes2(SFFHeader2* tHeader, MugenSpriteFile* tDst) {
	int i = 0;

	gData.mReader.mSeek(&gData.mReader, tHeader->mPaletteOffset);
	for (i = 0; i < (int)tHeader->mPaletteTotal; i++) {
		loadSinglePalette2(tHeader, tDst);
	}
}


static Buffer readRawRLE8Sprite2(uint32_t tSize) {
	uint32_t decompressedSize = 0;
	gData.mReader.mRead(&gData.mReader, &decompressedSize, sizeof(uint32_t));
	Buffer b = gData.mReader.mReadBufferReadOnly(&gData.mReader, tSize - 4);
	Buffer ret = decodeRLE8BufferAndReturnOwnedBuffer(b, decompressedSize);
	freeBuffer(b);

	return ret;
}

static Buffer decodeLZ5BufferAndReturnOwnedBuffer(Buffer b, uint32_t tFinalSize) {
	uint8_t* out = (uint8_t*)allocMemory(tFinalSize + 10);
	uint8_t* src = (uint8_t*)b.mData;

	decompressLZ5(out, src, b.mLength);

	return makeBufferOwned(out, tFinalSize);
}

static Buffer readRawLZ5Sprite2(uint32_t tSize) {
	uint32_t decompressedSize = 0;
	gData.mReader.mRead(&gData.mReader, &decompressedSize, sizeof(uint32_t));
	Buffer b = gData.mReader.mReadBufferReadOnly(&gData.mReader, tSize - 4);
	Buffer ret = decodeLZ5BufferAndReturnOwnedBuffer(b, decompressedSize);
	freeBuffer(b);

	return ret;
}

static Buffer readRawUncompressedSprite2(uint32_t tSize) {
	uint8_t* out = (uint8_t*)allocMemory(tSize + 10);

	Buffer b = gData.mReader.mReadBufferReadOnly(&gData.mReader, tSize);
	memcpy(out, b.mData, tSize);
	freeBuffer(b);

	return makeBufferOwned(out, tSize);
}


static uint32_t getSpriteDataOffset(SFFSprite2* tSprite, SFFHeader2* tHeader) {
	uint32_t realOffset;
	if (tSprite->mFlags) realOffset = tHeader->mTDataOffset;
	else realOffset = tHeader->mLDataOffset;

	return realOffset + tSprite->mDataOffset;
}

static Buffer readRawSprite2(SFFSprite2* tSprite, SFFHeader2* tHeader) {
	gData.mReader.mSeek(&gData.mReader, getSpriteDataOffset(tSprite, tHeader));

	if (tSprite->mFormat == 0) {
		return readRawUncompressedSprite2(tSprite->mDataLength);
	}
	else if (tSprite->mFormat == 2) {		
		return readRawRLE8Sprite2(tSprite->mDataLength);
	} else if (tSprite->mFormat == 4) {
		return readRawLZ5Sprite2(tSprite->mDataLength);
	}
	else {
		logError("Unable to parse sprite format.");
		logErrorInteger(tSprite->mFormat);
		recoverFromError();
		return makeBuffer(NULL, 0);
	}
	


}

static void insertLinkedTextureIntoSpriteFile2(MugenSpriteFile* tDst, SFFSprite2* tSprite) {
	MugenSpriteFileSprite* e = makeLinkedMugenSpriteFileSprite(tSprite->mIndex, makePosition(tSprite->mAxisX, tSprite->mAxisY, 0));
	insertTextureIntoSpriteFile(tDst, e, tSprite->mGroupNo, tSprite->mItemNo);
}

static void loadSingleSprite2(SFFHeader2* tHeader, MugenSpriteFile* tDst, int tPreferredPalette, int tHasPalette) {
	SFFSprite2 sprite;
	gData.mReader.mRead(&gData.mReader, &sprite, sizeof(SFFSprite2));

	debugLog("Load sprite2");
	debugInteger(sprite.mGroupNo);
	debugInteger(sprite.mItemNo);
	debugInteger(sprite.mFormat);

	if (gData.mIsOnlyLoadingPortraits && (sprite.mGroupNo != 9000 || sprite.mItemNo > 1)) { // TODO: non-hardcoded
		return;
	}

	if (!sprite.mDataLength) {
		insertLinkedTextureIntoSpriteFile2(tDst, &sprite);
		return;
	}

	int isPaletted = sprite.mFormat == 0 || sprite.mFormat == 2 || sprite.mFormat == 4;
	int isRawPNG = sprite.mFormat == 10 || sprite.mFormat == 11 || sprite.mFormat == 12;

	MugenSpriteFileSprite* e;
	if (isPaletted) {
		uint32_t originalPosition = gData.mReader.mGetCurrentOffset(&gData.mReader);

		Buffer rawBuffer = readRawSprite2(&sprite, tHeader);

		int palette;
		int spritePaletteIndex = tHasPalette ? sprite.mPaletteIndex + 1 : sprite.mPaletteIndex;
		tPreferredPalette = -1; // TODO
		if (tPreferredPalette == -1) palette = spritePaletteIndex;
		else palette = tPreferredPalette;

		Buffer* paletteBuffer = (Buffer*)vector_get(&tDst->mPalettes, palette);

		e = makeMugenSpriteFileSpriteFromRawAndPaletteBuffer(rawBuffer, *paletteBuffer, sprite.mWidth, sprite.mHeight, sprite.mWidth, makePosition(sprite.mAxisX, sprite.mAxisY, 0));
		gData.mReader.mSeek(&gData.mReader, originalPosition);
	}
	else if (isRawPNG) {
		uint32_t originalPosition = gData.mReader.mGetCurrentOffset(&gData.mReader);
		gData.mReader.mSeek(&gData.mReader, getSpriteDataOffset(&sprite, tHeader));
		uint16_t textureWidth, textureHeight;
		gData.mReader.mRead(&gData.mReader, &textureWidth, 2);
		gData.mReader.mRead(&gData.mReader, &textureHeight, 2);
		Buffer pngBuffer = gData.mReader.mReadBufferReadOnly(&gData.mReader, sprite.mDataLength);
		e = makeMugenSpriteFileSpriteFromRawPNGBuffer(pngBuffer, sprite.mWidth, sprite.mHeight, sprite.mWidth, makePosition(sprite.mAxisX, sprite.mAxisY, 0));
		gData.mReader.mSeek(&gData.mReader, originalPosition);
	}
	else {
		logError("Unrecognized image format.");
		logErrorInteger(sprite.mFormat);
		recoverFromError();
		e = NULL;
	}

	insertTextureIntoSpriteFile(tDst, e, sprite.mGroupNo, sprite.mItemNo);
}

static void loadSprites2(SFFHeader2* tHeader, MugenSpriteFile* tDst, int tPreferredPalette, int tHasPalette) {
	int i = 0;
	gData.mReader.mSeek(&gData.mReader, tHeader->mSpriteOffset);
	for (i = 0; i < (int)tHeader->mSpriteTotal; i++) {
		loadSingleSprite2(tHeader, tDst, tPreferredPalette, tHasPalette);
		if(gData.mIsOnlyLoadingPortraits && vector_size(&tDst->mAllSprites) == 2) break;
	}
}

static MugenSpriteFile loadMugenSpriteFile2(int tPreferredPalette, int tHasPaletteFile, char* tOptionalPaletteFile) {
	MugenSpriteFile ret = makeEmptySpriteFile();

	if (tHasPaletteFile) {
		loadMugenSpriteFilePaletteFile(&ret, tOptionalPaletteFile);
	}

	gData.mReader.mSeek(&gData.mReader, 0);

	SFFHeader2 header;
	loadSFFHeader2(&header);

	debugPointer(header.mSpriteOffset);
	debugInteger(header.mSpriteTotal);
	debugPointer(header.mPaletteOffset);
	debugInteger(header.mPaletteTotal);

	loadPalettes2(&header, &ret);
	tPreferredPalette = min(tPreferredPalette, vector_size(&ret.mPalettes) - 1);
	loadSprites2(&header, &ret, tPreferredPalette, tHasPaletteFile);

	return ret;
}

static Buffer loadPaddedBufferPreloaded() {
	uint32_t size;
	gData.mReader.mRead(&gData.mReader, &size, 4);

	char* dst = (char*)allocMemory(size);
	gData.mReader.mRead(&gData.mReader, dst, size);

	char padBuffer[10];
	int pad = (4 - (size % 4)) % 4;
	gData.mReader.mRead(&gData.mReader, padBuffer, pad);

	return makeBufferOwned(dst, size);
}

static void loadSinglePalettePreloaded(MugenSpriteFile* tDst) {

	Buffer b = loadPaddedBufferPreloaded();
	Buffer* insert = (Buffer*)allocMemory(sizeof(Buffer));
	*insert = b;

	insertPaletteIntoMugenSpriteFile(tDst, insert);
}

static void loadPalettesPreloaded(MugenSpriteFile* tDst) {
	uint32_t amount;
	gData.mReader.mRead(&gData.mReader, &amount, 4);

	uint32_t i;
	for (i = 0; i < amount; i++) {
		loadSinglePalettePreloaded(tDst);
	}

}

typedef struct {
	Vector3DI mOffset;
	int32_t mHasPalette;
	TextureSize mTextureSize;
} SubspriteHeader;

static MugenSpriteFileSubSprite* loadSingleSpriteSubSpritePreloaded() {
	SubspriteHeader header;
	gData.mReader.mRead(&gData.mReader, &header, sizeof(SubspriteHeader));

	Buffer b = loadPaddedBufferPreloaded();
	decompressBufferZSTD(&b);

	TextureData data;
	if (header.mHasPalette) {
		data = loadPalettedTextureFrom8BitBuffer(b, gData.mPaletteID, header.mTextureSize.x, header.mTextureSize.y);
	}
	else {
		data = loadTextureFromTwiddledARGB16Buffer(b, header.mTextureSize.x, header.mTextureSize.y);
	}

	if (isOnDreamcast()) {
		freeBuffer(b); // TODO: fix
	}

	MugenSpriteFileSubSprite* newSprite = (MugenSpriteFileSubSprite*)allocMemory(sizeof(MugenSpriteFileSubSprite));
	newSprite->mOffset = header.mOffset;
	newSprite->mTexture = data;

	return newSprite;
}

typedef struct {
	uint32_t mBlockSpriteAmount;
	uint32_t mBlockSize;
} PreloadedBlock;

typedef struct {
	int32_t mIsLinked;
	int32_t mIsLinkedTo;

	float mAxisOffsetX;
	float mAxisOffsetY;
	float mAxisOffsetZ;

	int32_t mGroupNumber;
	int32_t mSpriteNumber;

	int32_t mOriginalTextureSizeX;
	int32_t mOriginalTextureSizeY;

	uint32_t mSubspriteAmount;
} PreloadedSprite;

static void loadSingleSpritePreloaded(MugenSpriteFile* tDst) {
	PreloadedSprite spriteHeader;

	gData.mReader.mRead(&gData.mReader, &spriteHeader, sizeof(PreloadedSprite));

	Vector3D axisOffset = makePosition(spriteHeader.mAxisOffsetX, spriteHeader.mAxisOffsetY, spriteHeader.mAxisOffsetZ);

	if (spriteHeader.mIsLinked) {
		MugenSpriteFileSprite* sprite = makeLinkedMugenSpriteFileSprite(spriteHeader.mIsLinkedTo, axisOffset);
		insertTextureIntoSpriteFile(tDst, sprite, spriteHeader.mGroupNumber, spriteHeader.mSpriteNumber);
		return;
	}

	List textures = new_list();
	uint32_t i;
	for (i = 0; i < spriteHeader.mSubspriteAmount; i++) {
		MugenSpriteFileSubSprite* subSprite = loadSingleSpriteSubSpritePreloaded();
		list_push_back_owned(&textures, subSprite);
	}

	MugenSpriteFileSprite* sprite = makeMugenSpriteFileSprite(textures, makeTextureSize(spriteHeader.mOriginalTextureSizeX, spriteHeader.mOriginalTextureSizeY), axisOffset);
	insertTextureIntoSpriteFile(tDst, sprite, spriteHeader.mGroupNumber, spriteHeader.mSpriteNumber);
}

static void initBufferReaderReadOnlyBuffer(MugenSpriteFileReader* tReader, Buffer tBuffer);

static void loadSingleBlockPreloaded(MugenSpriteFile* tDst) {
	PreloadedBlock header;

	gData.mReader.mRead(&gData.mReader, &header, sizeof(PreloadedBlock));

	char* buf = (char*)allocMemory(header.mBlockSize + 2);
	gData.mReader.mRead(&gData.mReader, buf, header.mBlockSize);
	Buffer b = makeBufferOwned(buf, header.mBlockSize);

	MugenSpriteFileReader originalReader = gData.mReader;
	setMugenSpriteFileReaderToBuffer();
	initBufferReaderReadOnlyBuffer(&gData.mReader, b);

	for (uint32_t i = 0; i < header.mBlockSpriteAmount; i++) {
		loadSingleSpritePreloaded(tDst);
	}

	gData.mReader.mDelete(&gData.mReader);
	gData.mReader = originalReader;
}

static void loadSpritesPreloaded(MugenSpriteFile* tDst) {
	uint32_t amount;

	gData.mReader.mRead(&gData.mReader, &amount, 4);
	uint32_t i;
	for (i = 0; i < amount; i++) {
		loadSingleBlockPreloaded(tDst);
	}

}


static MugenSpriteFile loadMugenSpriteFilePreloaded(int tPreferredPalette, int tHasPaletteFile, char* tOptionalPaletteFile) {
	MugenSpriteFile ret = makeEmptySpriteFile();

	if (tHasPaletteFile) {
		loadMugenSpriteFilePaletteFile(&ret, tOptionalPaletteFile);
	}
	gData.mReader.mSeek(&gData.mReader, 0);
	SFFSharedHeader header;
	gData.mReader.mRead(&gData.mReader, &header, sizeof(SFFSharedHeader));

	loadPalettesPreloaded(&ret);
	loadSpritesPreloaded(&ret);

	return ret;
}

static void checkMugenSpriteFileReader() {
	if (gData.mReader.mType == 0) {
		setMugenSpriteFileReaderToBuffer();
		setMugenSpriteFileReaderToFileOperations(); // TODO: REMOVE
	}
}

static MugenSpriteFile loadMugenSpriteFileGeneral(char * tPath, int tPreferredPalette, int tHasPaletteFile, char* tOptionalPaletteFile)
{
	debugLog("Loading sprite file.");
	debugString(tPath);
	
	if (!isFile(tPath)) {
		logErrorFormat("Unable to open sprite file %s. Aborting.", tPath);
		recoverFromError();
	}
	char preloadPath[1024];
	sprintf(preloadPath, "%s.preloaded", tPath);
	if (isFile(preloadPath) && !gData.mIsWritingDreamcastOptimized) {
		return loadMugenSpriteFileGeneral(preloadPath, tPreferredPalette, tHasPaletteFile, tOptionalPaletteFile);
	}

	gData.mHasPaletteFile = tHasPaletteFile;
	checkMugenSpriteFileReader();

	MugenSpriteFile ret;
	gData.mReader.mInit(&gData.mReader, tPath);

	SFFSharedHeader header;
	gData.mReader.mRead(&gData.mReader, &header, sizeof(SFFSharedHeader));

	if (header.mVersion[1] == 1 && header.mVersion[3] == 1) {
		ret = loadMugenSpriteFile1(tHasPaletteFile, tOptionalPaletteFile);
	} else if (header.mVersion[1] == 1 && header.mVersion[3] == 2) {
		ret = loadMugenSpriteFile2(tPreferredPalette, tHasPaletteFile, tOptionalPaletteFile);
	}
	else if (header.mVersion[1] == 0 && header.mVersion[3] == 2) {
		ret = loadMugenSpriteFile2(tPreferredPalette, tHasPaletteFile, tOptionalPaletteFile);
	}
	else if (header.mVersion[1] == 0 && header.mVersion[3] == 100) {
		ret = loadMugenSpriteFilePreloaded(tPreferredPalette, tHasPaletteFile, tOptionalPaletteFile);
	}
	else {
		logError("Unrecognized SFF version.");
		logErrorInteger(header.mVersion[0]);
		logErrorInteger(header.mVersion[1])  ;
		logErrorInteger(header.mVersion[2]);
		logErrorInteger(header.mVersion[3]);
		recoverFromError();
	}

	gData.mReader.mDelete(&gData.mReader);
	
	return ret;
}

MugenSpriteFile loadMugenSpriteFile(char * tPath, int tPreferredPalette, int tHasPaletteFile, char* tOptionalPaletteFile) {
	gData.mIsOnlyLoadingPortraits = 0;
	return loadMugenSpriteFileGeneral(tPath, tPreferredPalette, tHasPaletteFile, tOptionalPaletteFile);
}

MugenSpriteFile loadMugenSpriteFilePortraits(char * tPath, int tPreferredPalette, int tHasPaletteFile, char* tOptionalPaletteFile) {
	gData.mIsOnlyLoadingPortraits = 1;
	return loadMugenSpriteFileGeneral(tPath, tPreferredPalette, tHasPaletteFile, tOptionalPaletteFile);
}

MugenSpriteFile loadMugenSpriteFileWithoutPalette(std::string & tPath)
{
	char path[1024];
	strcpy(path, tPath.data());
	return loadMugenSpriteFileWithoutPalette(path);
}


MugenSpriteFile loadMugenSpriteFileWithoutPalette(char * tPath)
{
	return loadMugenSpriteFile(tPath, -1, 0, NULL);
}

static void unloadSinglePalette(void* tCaller, void* tData) {
	(void)tCaller;
	Buffer* b = (Buffer*)tData;
	freeBuffer(*b);
}

static int unloadSingleSpriteFileGroupSpriteCB(void* tCaller, void* tData) {
	(void)tCaller;
	MugenSpriteFileSprite* e = (MugenSpriteFileSprite*)tData;
	unloadMugenSpriteFileSprite(e);
	return 1;
}

static int unloadSingleSpriteFileGroup(void* tCaller, void* tData) {
	(void)tCaller;
	MugenSpriteFileGroup* e = (MugenSpriteFileGroup*)tData;
	int_map_remove_predicate(&e->mSprites, unloadSingleSpriteFileGroupSpriteCB, NULL);
	delete_int_map(&e->mSprites);

	return 1;
}

void unloadMugenSpriteFile(MugenSpriteFile* tFile) {
	int_map_remove_predicate(&tFile->mGroups, unloadSingleSpriteFileGroup, NULL);
	delete_int_map(&tFile->mGroups);

	vector_map(&tFile->mPalettes, unloadSinglePalette, NULL);
	delete_vector(&tFile->mPalettes);
	delete_vector(&tFile->mAllSprites);
}

static int unloadSingleMugenSpriteFileSubSprite(void* tCaller, void* tData) {
	(void)tCaller;
	MugenSpriteFileSubSprite* subSprite = (MugenSpriteFileSubSprite*)tData;
	unloadTexture(subSprite->mTexture);

	return 1;
}

void unloadMugenSpriteFileSprite(MugenSpriteFileSprite* tSprite) {

	if (!tSprite->mIsLinked) {
		list_remove_predicate(&tSprite->mTextures, unloadSingleMugenSpriteFileSubSprite, NULL);
		delete_list(&tSprite->mTextures);
	}
}

MugenSpriteFileSprite* getMugenSpriteFileTextureReference(MugenSpriteFile* tFile, int tGroup, int tSprite)
{
	if (!tFile) return NULL;
	if (!int_map_contains(&tFile->mGroups, tGroup)) return NULL;

	MugenSpriteFileGroup* g = (MugenSpriteFileGroup*)int_map_get(&tFile->mGroups, tGroup);

	if (!int_map_contains(&g->mSprites, tSprite)) return NULL;

	MugenSpriteFileSprite* original = (MugenSpriteFileSprite*)int_map_get(&g->mSprites, tSprite);
	if (original->mIsLinked) {
		MugenSpriteFileSprite* e = original;
		while (e->mIsLinked) {
			e = (MugenSpriteFileSprite*)vector_get(&tFile->mAllSprites, e->mIsLinkedTo);
		}
		original->mOriginalTextureSize = e->mOriginalTextureSize;
		original->mTextures = e->mTextures;
	}
	return original;
}


typedef struct {
	Buffer b;
	BufferPointer p;

	int mIsOver;
} MugenSpriteFileBufferReaderData;


static void initBufferReader(MugenSpriteFileReader* tReader, char* tPath) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)allocMemory(sizeof(MugenSpriteFileBufferReaderData));
	data->b = fileToBuffer(tPath);
	data->p = getBufferPointer(data->b);
	data->mIsOver = 0;
	tReader->mData = data;
}

static void initBufferReaderReadOnlyBuffer(MugenSpriteFileReader* tReader, Buffer tBuffer) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)allocMemory(sizeof(MugenSpriteFileBufferReaderData));
	data->b = tBuffer;
	data->p = getBufferPointer(data->b);
	data->mIsOver = 0;
	tReader->mData = data;
}

static void deleteBufferReader(MugenSpriteFileReader* tReader) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)tReader->mData;
	freeBuffer(data->b);
	freeMemory(data);
}

static void readBufferReader(MugenSpriteFileReader* tReader, void* tDst, uint32_t tSize) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)tReader->mData;
	readFromBufferPointer(tDst, &data->p, tSize);
}

static void seekBufferReader(MugenSpriteFileReader* tReader, uint32_t tPosition) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)tReader->mData;
	data->p = ((char*)data->b.mData) + tPosition;

	if (data->p >= ((char*)data->b.mData) + data->b.mLength) {
		gData.mReader.mSetOver(&gData.mReader);
	}
}

static Buffer readBufferReaderBuffer(MugenSpriteFileReader* tReader, uint32_t tSize) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)tReader->mData;
	return makeBuffer(data->p, tSize);
}

static uint32_t getCurrentBufferReaderOffset(MugenSpriteFileReader* tReader) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)tReader->mData;
	return (uint32_t)(data->p - ((char*)data->b.mData));
}


static void setBufferReaderOver(MugenSpriteFileReader* tReader) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)tReader->mData;
	data->p = NULL;
	data->mIsOver = 1;
}

static int isBufferReaderOver(MugenSpriteFileReader* tReader) {
	MugenSpriteFileBufferReaderData* data = (MugenSpriteFileBufferReaderData*)tReader->mData;
	return data->mIsOver;
}

static MugenSpriteFileReader getMugenSpriteFileBufferReader() {
	MugenSpriteFileReader ret;
	ret.mType = 1;
	ret.mInit = initBufferReader;
	ret.mRead = readBufferReader;
	ret.mReadBufferReadOnly = readBufferReaderBuffer;
	ret.mSeek = seekBufferReader;
	ret.mGetCurrentOffset = getCurrentBufferReaderOffset;
	ret.mDelete = deleteBufferReader;
	ret.mSetOver = setBufferReaderOver;
	ret.mIsOver = isBufferReaderOver;
	return ret;
}

void setMugenSpriteFileReaderToBuffer()
{
	gData.mReader = getMugenSpriteFileBufferReader();
}

typedef struct {
	FileHandler mFile;
	uint32_t mFileSize;
	int mIsOver;
} MugenSpriteFileFileReaderData;


static void initFileReader(MugenSpriteFileReader* tReader, char* tPath) {
	MugenSpriteFileFileReaderData* data = (MugenSpriteFileFileReaderData*)allocMemory(sizeof(MugenSpriteFileFileReaderData));
	data->mFile = fileOpen(tPath, O_RDONLY);
	data->mFileSize = fileTotal(data->mFile);
	data->mIsOver = 0;
	tReader->mData = data;
}

static void deleteFileReader(MugenSpriteFileReader* tReader) {
	MugenSpriteFileFileReaderData* data = (MugenSpriteFileFileReaderData*)tReader->mData;
	fileClose(data->mFile);
	freeMemory(data);
}

static void readFileReader(MugenSpriteFileReader* tReader, void* tDst, uint32_t tSize) {
	MugenSpriteFileFileReaderData* data = (MugenSpriteFileFileReaderData*)tReader->mData;

	fileRead(data->mFile, tDst, tSize);
}

static void seekFileReader(MugenSpriteFileReader* tReader, uint32_t tPosition) {
	MugenSpriteFileFileReaderData* data = (MugenSpriteFileFileReaderData*)tReader->mData;

	fileSeek(data->mFile, tPosition, SEEK_SET); 
	uint32_t position = gData.mReader.mGetCurrentOffset(&gData.mReader);

	if (position >= data->mFileSize) {
		gData.mReader.mSetOver(&gData.mReader);
	}
}

static Buffer readFileReaderBuffer(MugenSpriteFileReader* tReader, uint32_t tSize) {
	(void)tReader;
	uint32_t originalPosition = gData.mReader.mGetCurrentOffset(&gData.mReader);
	char* dst = (char*)allocMemory(tSize + 10);
	gData.mReader.mRead(&gData.mReader, dst, tSize);

	gData.mReader.mSeek(&gData.mReader, originalPosition);

	return makeBufferOwned(dst, tSize);
}

static uint32_t getCurrentFileReaderOffset(MugenSpriteFileReader* tReader) {
	MugenSpriteFileFileReaderData* data = (MugenSpriteFileFileReaderData*)tReader->mData;

	return (uint32_t)fileTell(data->mFile);
}


static void setFileReaderOver(MugenSpriteFileReader* tReader) {
	MugenSpriteFileFileReaderData* data = (MugenSpriteFileFileReaderData*)tReader->mData;
	data->mIsOver = 1;
}

static int isFileReaderOver(MugenSpriteFileReader* tReader) {
	MugenSpriteFileFileReaderData* data = (MugenSpriteFileFileReaderData*)tReader->mData;
	return data->mIsOver;
}

static MugenSpriteFileReader getMugenSpriteFileFileReader() {
	MugenSpriteFileReader ret;
	ret.mType = 2;
	ret.mInit = initFileReader;
	ret.mRead = readFileReader;
	ret.mReadBufferReadOnly = readFileReaderBuffer;
	ret.mSeek = seekFileReader;
	ret.mGetCurrentOffset = getCurrentFileReaderOffset;
	ret.mDelete = deleteFileReader;
	ret.mSetOver = setFileReaderOver;
	ret.mIsOver = isFileReaderOver;
	return ret;
}

void setMugenSpriteFileReaderToFileOperations()
{
	gData.mReader = getMugenSpriteFileFileReader();
}

void setMugenSpriteFileReaderToUsePalette(int tPaletteID)
{
	gData.mIsUsingRealPalette = 1;
	gData.mPaletteID = tPaletteID;
}

void setMugenSpriteFileReaderToNotUsePalette()
{
	gData.mIsUsingRealPalette = 0;
}

void setMugenSpriteFileReaderCustomFunctionsAndForceARGB16(TextureData(*tCustomLoadTextureFromARGB16Buffer)(Buffer, int, int), TextureData(*tCustomLoadTextureFromARGB32Buffer)(Buffer, int, int), TextureData(*tCustomLoadPalettedTextureFrom8BitBuffer)(Buffer, int, int, int))
{
	gData.mCustomLoadTextureFromARGB16Buffer = tCustomLoadTextureFromARGB16Buffer;
	gData.mCustomLoadTextureFromARGB32Buffer = tCustomLoadTextureFromARGB32Buffer;
	gData.mCustomLoadPalettedTextureFrom8BitBuffer = tCustomLoadPalettedTextureFrom8BitBuffer;
	gData.mIsWritingDreamcastOptimized = 1;
}
