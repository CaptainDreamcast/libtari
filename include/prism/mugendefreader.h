#pragma once

#include <string>

#include "datastructures.h"
#include "geometry.h"
#include "file.h"
#include "stlutil.h"

#define MUGEN_DEF_STRING_LENGTH 500

typedef enum {
	MUGEN_DEF_SCRIPT_GROUP_STRING_ELEMENT,
	MUGEN_DEF_SCRIPT_GROUP_NUMBER_ELEMENT,
	MUGEN_DEF_SCRIPT_GROUP_FLOAT_ELEMENT,
	MUGEN_DEF_SCRIPT_GROUP_VECTOR_ELEMENT,
} MugenDefScriptGroupElementType;

typedef struct {
	int mSize;
	char** mElement;

} MugenStringVector;

typedef struct {
	MugenStringVector mVector;
} MugenDefScriptVectorElement;

typedef struct {
	char* mString;

} MugenDefScriptStringElement;

typedef struct {
	int mValue;
} MugenDefScriptNumberElement;

typedef struct {
	double mValue;
} MugenDefScriptFloatElement;

typedef struct {
	std::string mName;
	MugenDefScriptGroupElementType mType;
	void* mData;

} MugenDefScriptGroupElement;

typedef struct MugenDefScriptGroup_t{
	std::string mName;
	std::map<std::string, MugenDefScriptGroupElement> mElements;
	List mOrderedElementList;
	struct MugenDefScriptGroup_t* mNext;
} MugenDefScriptGroup;

typedef struct {
	MugenDefScriptGroup* mFirstGroup;
	std::map<std::string, MugenDefScriptGroup> mGroups;
} MugenDefScript;

void loadMugenDefScript(MugenDefScript* oScript, std::string& tPath);
void loadMugenDefScript(MugenDefScript* oScript, const char* tPath);
void loadMugenDefScriptFromBufferAndFreeBuffer(MugenDefScript* oScript, Buffer tBuffer);
void unloadMugenDefScript(MugenDefScript tScript);

int isMugenDefStringVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
char* getAllocatedMugenDefStringVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
int isMugenDefStringVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
char* getAllocatedMugenDefStringVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
int isMugenDefStringVariableAsElement(MugenDefScriptGroupElement* tElement);
char* getAllocatedMugenDefStringVariableForAssignmentAsElement(MugenDefScriptGroupElement* tElement);
char* getAllocatedMugenDefStringVariableAsElement(MugenDefScriptGroupElement* tElement);

int isMugenDefVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);

int isMugenDefFloatVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
double getMugenDefFloatVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
int isMugenDefFloatVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
double getMugenDefFloatVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
int isMugenDefFloatVariableAsElement(MugenDefScriptGroupElement* tElement);
double getMugenDefFloatVariableAsElement(MugenDefScriptGroupElement* tElement);


int isMugenDefNumberVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
int getMugenDefNumberVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
int isMugenDefNumberVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
int getMugenDefNumberVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
int isMugenDefNumberVariableAsElement(MugenDefScriptGroupElement* tElement);
int getMugenDefNumberVariableAsElement(MugenDefScriptGroupElement* tElement);

int isMugenDefVectorVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
Vector3D getMugenDefVectorVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
int isMugenDefVectorVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
Vector3D getMugenDefVectorVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
int isMugenDefVectorVariableAsElement(MugenDefScriptGroupElement* tElement);
Vector3D getMugenDefVectorVariableAsElement(MugenDefScriptGroupElement* tElement);

int isMugenDefVectorIVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
int isMugenDefVectorIVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
Vector3DI getMugenDefVectorIVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
Vector3DI getMugenDefVectorIVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);

int isMugenDefStringVectorVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
int isMugenDefStringVectorVariableAsElement(MugenDefScriptGroupElement* tElement);
MugenStringVector getMugenDefStringVectorVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
MugenStringVector getMugenDefStringVectorVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
MugenStringVector getMugenDefStringVectorVariableAsElement(MugenDefScriptGroupElement* tElement);
MugenStringVector copyMugenDefStringVectorVariableAsElement(MugenDefScriptGroupElement * tElement);

int isMugenDefGeoRectangleVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
int isMugenDefGeoRectangleVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
int isMugenDefGeoRectangleVariableAsElement(MugenDefScriptGroupElement * tElement);
GeoRectangle getMugenDefGeoRectangleVariable(MugenDefScript* tScript, const char* tGroupName, const char* tVariableName);
GeoRectangle getMugenDefGeoRectangleVariableAsGroup(MugenDefScriptGroup* tGroup, const char* tVariableName);
GeoRectangle getMugenDefGeoRectangleVariableAsElement(MugenDefScriptGroupElement * tElement);

void getMugenDefStringOrDefault(char* tDst, MugenDefScript* s, const char* tGroup, const char* tVariable, const char* tDefault);
char* getAllocatedMugenDefStringOrDefault(MugenDefScript* s, const char* tGroup, const char* tVariable, const char* tDefault);
char* getAllocatedMugenDefStringOrDefaultAsGroup(MugenDefScriptGroup* tGroup, const char* tVariable, const char* tDefault);

double getMugenDefFloatOrDefault(MugenDefScript* s, const char* tGroup, const char* tVariable, double tDefault);
double getMugenDefFloatOrDefaultAsGroup(MugenDefScriptGroup* tGroup, const char* tVariable, double tDefault);
int getMugenDefIntegerOrDefault(MugenDefScript* s, const char* tGroup, const char* tVariable, int tDefault);
int getMugenDefIntegerOrDefaultAsGroup(MugenDefScriptGroup* tGroup, const char* tVariable, int tDefault);
Vector3D getMugenDefVectorOrDefault(MugenDefScript* s, const char* tGroup, const char* tVariable, Vector3D tDefault);
Vector3D getMugenDefVectorOrDefaultAsGroup(MugenDefScriptGroup* tGroup, const char* tVariable, Vector3D tDefault);
Vector3DI getMugenDefVectorIOrDefault(MugenDefScript* s, const char* tGroup, const char* tVariable, Vector3DI tDefault);
Vector3DI getMugenDefVectorIOrDefaultAsGroup(MugenDefScriptGroup* tGroup, const char* tVariable, Vector3DI tDefault);

GeoRectangle getMugenDefGeoRectangleOrDefault(MugenDefScript* s, const char* tGroup, const char* tVariable, GeoRectangle tDefault);
GeoRectangle getMugenDefGeoRectangleOrDefaultAsGroup(MugenDefScriptGroup* tGroup, const char* tVariable, GeoRectangle tDefault);