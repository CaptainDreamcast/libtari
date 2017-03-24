#ifndef TARI_DRAWING
#define TARI_DRAWING

#include "physics.h"
#include "texture.h"

typedef struct {
  int x;
  int y;
} TexturePosition;

typedef struct {
  TexturePosition topLeft;
  TexturePosition bottomRight;
} Rectangle;

typedef enum {
  COLOR_BLACK,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_YELLOW,
  COLOR_WHITE
} Color;

typedef int TextSize;

fup void initDrawing();
fup void drawSprite(TextureData tTexture, Position tPos, Rectangle tTexturePosition);
fup void drawText(char tText[], Position tPosition, TextSize tSize, Color tColor);
fup void waitForScreen();
fup void startDrawing();
fup void stopDrawing();

fup void scaleDrawing(double tFactor, Position tScalePosition);
fup void scaleDrawing3D(Vector3D tFactor, Position tScalePosition);
fup void setDrawingBaseColor(Color tColor);
fup void setDrawingTransparency(double tAlpha);
fup void setDrawingRotationZ(double tAngle, Position tPosition);
fup void setDrawingParametersToIdentity();

fup Rectangle makeRectangle(int x, int y, int w, int h);
fup Rectangle makeRectangleFromTexture(TextureData tTexture);
fup Position getTextureMiddlePosition(TextureData tTexture);
fup void printRectangle(Rectangle r);



#endif
