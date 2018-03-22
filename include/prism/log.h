#pragma once

#include <stdio.h>

#define logBase() {printf("[%s::%s, line %d] ", __FILE__, __FUNCTION__, __LINE__);}	
#define logg(x)	{logBase(); printf(x); printf("\n");}
#define logInteger(x) {logBase(); printf("Value of %s: %d\n", #x, (int)x);}
#define logString(x) {logBase(); printf("Value of %s: %s\n", #x, x);}
#define logWString(x) {logBase(); printf("Value of %s: %S\n", #x, x);}
#define logDouble(x) {logBase(); printf("Value of %s: %f\n", #x, (double)x);}
#define logHex(x) {logBase(); printf("Value of %s: %X\n", #x, (unsigned int)x);}
#define logPointer(x) {logBase(); printf("Value of %s: %p\n", #x, (char*)x);}
#define logFormat(x, ...) {logBase(); logFormatFunc(x,  __VA_ARGS__); }
void logFormatFunc(char* tFormatString, ...);

#define logError(x) logg(x)
#define logErrorInteger(x) logInteger(x)
#define logErrorString(x) logString(x)
#define logErrorDouble(x) logDouble(x)
#define logErrorHex(x) logHex(x)
#define logErrorPointer(x) logPointer(x)
#define logErrorFormat(x, ...) logFormat(x,  __VA_ARGS__)

#define logWarning(x) logg(x)
#define logWarningInteger(x) logInteger(x)
#define logWarningString(x) logString(x)
#define logWarningFormat(x, ...) logFormat(x,  __VA_ARGS__)

#ifdef DEBUG
#define debugLog(x) logg(x)
#define debugInteger(x) logInteger(x)
#define debugDouble(x) logDouble(x)
#define debugString(x) logString(x)
#define debugPointer(x) logPointer(x)

#else
#define debugLog(x) {}
#define debugInteger(x) {}
#define debugDouble(x) {}
#define debugString(x) {}
#define debugPointer(x) {}
#endif	

#ifdef DEBUG
void logTextureMemoryState();
void logMemoryState();
#else
#define logTextureMemoryState() {}
#define logMemoryState() {}
#endif

