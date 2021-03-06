#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

void* createMemoryPool();
void destroyMemoryPool(void* tPool);

void* allocPoolMemory(void* tPool, size_t size);
void* callocPoolMemory(void* tPool, size_t nelem, size_t elsize);
void* reallocPoolMemory(void* tPool, void *ptr, size_t size);
void freePoolMemory(void* tPool, void *ptr);
int isMemoryInPool(void* tPool, void* ptr);