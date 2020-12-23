/*

 Example sketch for TFT_eSPI library.

 No fonts are needed.
 
 Draws a 3d rotating cube on the TFT screen.
 
 Original code was found at http://forum.freetronics.com/viewtopic.php?f=37&t=5495
 
 */

#include <Wire.h>

//
// Define these in User_Setup.h in the TFT_eSPI
//
#define ST7796_DRIVER 1
#define TFT_WIDTH  480
#define TFT_HEIGHT 320

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320

#define USE_HSPI_PORT 1
#define PIN_SDA 18
#define PIN_SCL 19
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   21
#define TFT_RST  22
#define TFT_BL   23

//#define TOUCH_CS PIN_D2     // Chip select pin (T_CS) of touch screen
#define FT62XX_ADDR 0x38

#define BLACK 0x0000
#define WHITE 0xFFFF

#include <SPI.h>

#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

int16_t h;
int16_t w;

int inc = -2;

float xx, xy, xz;
float yx, yy, yz;
float zx, zy, zz;

float fact;

int Xan, Yan;

int Xoff;
int Yoff;
int Zoff;

struct Point3d
{
  int x;
  int y;
  int z;
};

struct Point2d
{
  int x;
  int y;
};

int LinestoRender; // lines to render.
int OldLinestoRender; // lines to render just in case it changes. this makes sure the old lines all get erased.

struct Line3d
{
  Point3d p0;
  Point3d p1;
};

struct Line2d
{
  Point2d p0;
  Point2d p1;
};

Line3d Lines[20];
Line2d Render[20];
Line2d ORender[20];

#define FT62XX_REG_MODE 0x00        //!< Device mode, either WORKING or FACTORY
#define FT62XX_REG_CALIBRATE 0x02   //!< Calibrate mode
#define FT62XX_REG_WORKMODE 0x00    //!< Work mode
#define FT62XX_REG_FACTORYMODE 0x40 //!< Factory mode
#define FT62XX_REG_THRESHHOLD 0x80  //!< Threshold for touch detection
#define FT62XX_REG_POINTRATE 0x88   //!< Point rate
#define FT62XX_REG_FIRMVERS 0xA6    //!< Firmware version
#define FT62XX_REG_CHIPID 0xA3      //!< Chip selecting
#define FT62XX_REG_VENDID 0xA8      //!< FocalTech's panel ID

#define FT62XX_VENDID 0x11  //!< FocalTech's panel ID
#define FT6206_CHIPID 0x06  //!< Chip selecting
#define FT6236_CHIPID 0x36  //!< Chip selecting
#define FT6236U_CHIPID 0x64 //!< Chip selecting

bool touchScreenBegin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  #ifdef TOUCHSCREEN_DEBUG
    Serial.print("Vend ID: 0x");
    Serial.println(readByteFromTouch(FT62XX_REG_VENDID), HEX);
    Serial.print("Chip ID: 0x");
    Serial.println(readByteFromTouch(FT62XX_REG_CHIPID), HEX);
    Serial.print("Firm V: ");
    Serial.println(readByteFromTouch(FT62XX_REG_FIRMVERS));
    Serial.print("Point Rate Hz: ");
    Serial.println(readByteFromTouch(FT62XX_REG_POINTRATE));
    Serial.print("Thresh: ");
    Serial.println(readByteFromTouch(FT62XX_REG_THRESHHOLD));
  
    // dump all registers
    for (int16_t i = 0; i < 0x10; i++) {
      Serial.print("I2C $");
      Serial.print(i, HEX);
      Serial.print(" = 0x");
      Serial.println(readByteFromTouch(i), HEX);
    }
  #endif

  // change threshhold to be higher/lower
  //writeByteToTouch(FT62XX_REG_THRESHHOLD, FT62XX_DEFAULT_THRESHOLD);

  if (readByteFromTouch(FT62XX_REG_VENDID) != FT62XX_VENDID) {
    return false;
  }
  uint8_t id = readByteFromTouch(FT62XX_REG_CHIPID);
  if ((id != FT6206_CHIPID) && (id != FT6236_CHIPID) &&
      (id != FT6236U_CHIPID)) {
    return false;
  }
  return true;
}

uint8_t readByteFromTouch(uint8_t reg) {
  Wire.beginTransmission(FT62XX_ADDR);
  Wire.write((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom((byte)FT62XX_ADDR, (byte)1);
  uint8_t x = Wire.read();
  #ifdef TOUCHSCREEN_DEBUG
    Serial.print("$");
    Serial.print(reg, HEX);
    Serial.print(": 0x");
    Serial.println(x, HEX);
  #endif
  return x;
}

void writeByteToTouch(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(FT62XX_ADDR);
  Wire.write((byte)reg);
  Wire.write((byte)val);
  Wire.endTransmission();
}

uint32_t readTouchData(void) {

  uint8_t i2cdat[16];
  Wire.beginTransmission(FT62XX_ADDR);
  Wire.write((byte)0);
  Wire.endTransmission();

  Wire.requestFrom((byte)FT62XX_ADDR, (byte)16);
  for (uint8_t i = 0; i < 16; i++) {
    i2cdat[i] = Wire.read();
  }

  #ifdef TOUCHSCREEN_DEBUG
    for (int16_t i = 0; i < 16; i++) {
      Serial.print("I2C $");
      Serial.print(i, HEX);
      Serial.print(" = 0x");
      Serial.println(i2cdat[i], HEX);
    }
  #endif

  uint8_t touches = i2cdat[0x02];

  if (touches != 1) {
    return 0;
  }
  
  #ifdef TOUCHSCREEN_DEBUG
    Serial.print("# Touches: ");
    Serial.println(touches);
  
    for (uint8_t i = 0; i < 16; i++) {
      Serial.print("0x");
      Serial.print(i2cdat[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    if (i2cdat[0x01] != 0x00) {
      Serial.print("Gesture #");
      Serial.println(i2cdat[0x01]);
    }
  #endif

  uint16_t touchY = i2cdat[0x03] & 0x0F;
  touchY <<= 8;
  touchY |= i2cdat[0x04];
  uint16_t touchX = i2cdat[0x05] & 0x0F;
  touchX <<= 8;
  touchX |= i2cdat[0x06];
  uint16_t touchID = i2cdat[0x05] >> 4;

  #ifdef TOUCHSCREEN_DEBUG
    Serial.println();
    for (uint8_t i = 0; i < touches; i++) {
      Serial.print("ID #");
      Serial.print(touchID);
      Serial.print("\t(");
      Serial.print(touchX);
      Serial.print(", ");
      Serial.print(touchY);
      Serial.print(") ");
    }
    Serial.println();
  #endif

  touchY = DISPLAY_HEIGHT - touchY;

  return 0x80000000 | ((touchX & 0xFFF) << 16) | ((touchY & 0xFFF) << 4) | (touchID & 0xF);
}

/***********************************************************************************************************************************/

int32_t lastX = -1, lastY = -1;

void setup() {
  Serial.begin(115200);
  tft.init();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, 128);

  touchScreenBegin();

  h = tft.height();
  w = tft.width();

  tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);

  cube();

  fact = 180 / 3.14159259; // conversion from degrees to radians.

  Xoff = 240; // Position the center of the 3d conversion space into the center of the TFT screen.
  Yoff = 160;
  Zoff = 550; // Z offset in 3D space (smaller = closer and bigger rendering)
}

/***********************************************************************************************************************************/
void loop() {

  uint32_t touchPos = readTouchData();

  if (touchPos & 0x80000000) {
    uint16_t xPos = (touchPos >> 16) & 0xFFF;
    uint16_t yPos = ((touchPos >> 4) & 0xFFF);
    uint16_t touchID = touchPos & 0xF;
    Serial.printf("X: %d, Y: %d, ID: %d\n", xPos, yPos, touchID);
    lastX = xPos;
    lastY = yPos;
  }

  if( lastX >= 0 && lastY >= 0) {
    tft.drawCircle(lastX, lastY, 10, TFT_SILVER);
  }

  // Rotate around x and y axes in 1 degree increments
  Xan++;
  Yan++;

  Yan = Yan % 360;
  Xan = Xan % 360; // prevents overflow.

  SetVars(); //sets up the global vars to do the 3D conversion.

  // Zoom in and out on Z axis within limits
  // the cube intersects with the screen for values < 160
  Zoff += inc; 
  if (Zoff > 500) inc = -1;     // Switch to zoom in
  else if (Zoff < 160) inc = 1; // Switch to zoom out

  for (int i = 0; i < LinestoRender ; i++)
  {
    ORender[i] = Render[i]; // stores the old line segment so we can delete it later.
    ProcessLine(&Render[i], Lines[i]); // converts the 3d line segments to 2d.
  }
  RenderImage(); // go draw it!

  delay(20); // Delay to reduce loop rate (reduces flicker caused by aliasing with TFT screen refresh rate)
}

/***********************************************************************************************************************************/
void RenderImage( void)
{
  // renders all the lines after erasing the old ones.
  // in here is the only code actually interfacing with the OLED. so if you use a different lib, this is where to change it.

  for (int i = 0; i < OldLinestoRender; i++ )
  {
    tft.drawLine(ORender[i].p0.x, ORender[i].p0.y, ORender[i].p1.x, ORender[i].p1.y, BLACK); // erase the old lines.
  }


  for (int i = 0; i < LinestoRender; i++ )
  {
    uint16_t color = TFT_BLUE;
    if (i < 4) color = TFT_RED;
    if (i > 7) color = TFT_GREEN;
    tft.drawLine(Render[i].p0.x, Render[i].p0.y, Render[i].p1.x, Render[i].p1.y, color);
  }
  OldLinestoRender = LinestoRender;
}

/***********************************************************************************************************************************/
// Sets the global vars for the 3d transform. Any points sent through "process" will be transformed using these figures.
// only needs to be called if Xan or Yan are changed.
void SetVars(void)
{
  float Xan2, Yan2, Zan2;
  float s1, s2, s3, c1, c2, c3;

  Xan2 = Xan / fact; // convert degrees to radians.
  Yan2 = Yan / fact;

  // Zan is assumed to be zero

  s1 = sin(Yan2);
  s2 = sin(Xan2);

  c1 = cos(Yan2);
  c2 = cos(Xan2);

  xx = c1;
  xy = 0;
  xz = -s1;

  yx = (s1 * s2);
  yy = c2;
  yz = (c1 * s2);

  zx = (s1 * c2);
  zy = -s2;
  zz = (c1 * c2);
}


/***********************************************************************************************************************************/
// processes x1,y1,z1 and returns rx1,ry1 transformed by the variables set in SetVars()
// fairly heavy on floating point here.
// uses a bunch of global vars. Could be rewritten with a struct but not worth the effort.
void ProcessLine(struct Line2d *ret, struct Line3d vec)
{
  float zvt1;
  int xv1, yv1, zv1;

  float zvt2;
  int xv2, yv2, zv2;

  int rx1, ry1;
  int rx2, ry2;

  int x1;
  int y1;
  int z1;

  int x2;
  int y2;
  int z2;

  int Ok;

  x1 = vec.p0.x;
  y1 = vec.p0.y;
  z1 = vec.p0.z;

  x2 = vec.p1.x;
  y2 = vec.p1.y;
  z2 = vec.p1.z;

  Ok = 0; // defaults to not OK

  xv1 = (x1 * xx) + (y1 * xy) + (z1 * xz);
  yv1 = (x1 * yx) + (y1 * yy) + (z1 * yz);
  zv1 = (x1 * zx) + (y1 * zy) + (z1 * zz);

  zvt1 = zv1 - Zoff;

  if ( zvt1 < -5) {
    rx1 = 256 * (xv1 / zvt1) + Xoff;
    ry1 = 256 * (yv1 / zvt1) + Yoff;
    Ok = 1; // ok we are alright for point 1.
  }

  xv2 = (x2 * xx) + (y2 * xy) + (z2 * xz);
  yv2 = (x2 * yx) + (y2 * yy) + (z2 * yz);
  zv2 = (x2 * zx) + (y2 * zy) + (z2 * zz);

  zvt2 = zv2 - Zoff;

  if ( zvt2 < -5) {
    rx2 = 256 * (xv2 / zvt2) + Xoff;
    ry2 = 256 * (yv2 / zvt2) + Yoff;
  } else
  {
    Ok = 0;
  }

  if (Ok == 1) {

    ret->p0.x = rx1;
    ret->p0.y = ry1;

    ret->p1.x = rx2;
    ret->p1.y = ry2;
  }
  // The ifs here are checks for out of bounds. needs a bit more code here to "safe" lines that will be way out of whack, so they dont get drawn and cause screen garbage.

}

/***********************************************************************************************************************************/
// line segments to draw a cube. basically p0 to p1. p1 to p2. p2 to p3 so on.
void cube(void)
{
  // Front Face.

  Lines[0].p0.x = -50;
  Lines[0].p0.y = -50;
  Lines[0].p0.z = 50;
  Lines[0].p1.x = 50;
  Lines[0].p1.y = -50;
  Lines[0].p1.z = 50;

  Lines[1].p0.x = 50;
  Lines[1].p0.y = -50;
  Lines[1].p0.z = 50;
  Lines[1].p1.x = 50;
  Lines[1].p1.y = 50;
  Lines[1].p1.z = 50;

  Lines[2].p0.x = 50;
  Lines[2].p0.y = 50;
  Lines[2].p0.z = 50;
  Lines[2].p1.x = -50;
  Lines[2].p1.y = 50;
  Lines[2].p1.z = 50;

  Lines[3].p0.x = -50;
  Lines[3].p0.y = 50;
  Lines[3].p0.z = 50;
  Lines[3].p1.x = -50;
  Lines[3].p1.y = -50;
  Lines[3].p1.z = 50;


  //back face.

  Lines[4].p0.x = -50;
  Lines[4].p0.y = -50;
  Lines[4].p0.z = -50;
  Lines[4].p1.x = 50;
  Lines[4].p1.y = -50;
  Lines[4].p1.z = -50;

  Lines[5].p0.x = 50;
  Lines[5].p0.y = -50;
  Lines[5].p0.z = -50;
  Lines[5].p1.x = 50;
  Lines[5].p1.y = 50;
  Lines[5].p1.z = -50;

  Lines[6].p0.x = 50;
  Lines[6].p0.y = 50;
  Lines[6].p0.z = -50;
  Lines[6].p1.x = -50;
  Lines[6].p1.y = 50;
  Lines[6].p1.z = -50;

  Lines[7].p0.x = -50;
  Lines[7].p0.y = 50;
  Lines[7].p0.z = -50;
  Lines[7].p1.x = -50;
  Lines[7].p1.y = -50;
  Lines[7].p1.z = -50;


  // now the 4 edge lines.

  Lines[8].p0.x = -50;
  Lines[8].p0.y = -50;
  Lines[8].p0.z = 50;
  Lines[8].p1.x = -50;
  Lines[8].p1.y = -50;
  Lines[8].p1.z = -50;

  Lines[9].p0.x = 50;
  Lines[9].p0.y = -50;
  Lines[9].p0.z = 50;
  Lines[9].p1.x = 50;
  Lines[9].p1.y = -50;
  Lines[9].p1.z = -50;

  Lines[10].p0.x = -50;
  Lines[10].p0.y = 50;
  Lines[10].p0.z = 50;
  Lines[10].p1.x = -50;
  Lines[10].p1.y = 50;
  Lines[10].p1.z = -50;

  Lines[11].p0.x = 50;
  Lines[11].p0.y = 50;
  Lines[11].p0.z = 50;
  Lines[11].p1.x = 50;
  Lines[11].p1.y = 50;
  Lines[11].p1.z = -50;

  LinestoRender = 12;
  OldLinestoRender = LinestoRender;

}

