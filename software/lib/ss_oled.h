/*----------------------------------------------------------*
   SSD1306 library
                                        Last Edit ' 09/29

   Convert the following code to C++
   https://github.com/bitbank2/ss_oled
 *----------------------------------------------------------*/

#pragma once

#include  <stdlib.h>
#include  <string.h>
#include  <stdarg.h>
#include  <stdint.h>
#include  <ud5.h>

#define __MEMORY_SAVEING_SYSTEM__

/*!
 @brief SSD1306 OLED class.
 @note
   The classifieds are based on <a href="https://github.com/bitbank2/ss_oled">this repository</a>.
   CI2C required.
 @attention
   Some parts of the system were half forced to work with SSD1306 and have not been tested on any devices except for that one.
 */
class COLED {

 public:

  typedef struct ssoleds {
    uint8_t oled_addr; // requested address or 0xff for automatic detection
    uint8_t oled_wrap, oled_flip, oled_type;
    uint8_t *ucScreen;
    uint8_t iCursorX, iCursorY;
    uint8_t oled_x, oled_y;
    int iScreenOffset;
    int rc;
    CI2C *bbi2c;
  } SSOLED;

  //! 4 possible font sizes: 8x8, 16x32, 6x8, 16x16 (stretched from 8x8)
  enum {
    FONT_6x8 = 0,
    FONT_8x8,
    FONT_12x16,
    FONT_16x16,
    FONT_16x32
  };
  //! 4 possible rotation angles for oledScaledString()
  enum {
    ROT_0 = 0,
    ROT_90,
    ROT_180,
    ROT_270
  };
  //! OLED type for init function
  enum {
    OLED_128x128 = 1,
    OLED_128x32,
    OLED_128x64,
    OLED_132x64,
    OLED_64x32,
    OLED_96x16,
    OLED_72x40
  };
  //! Rotation and flip angles to draw tiles
  enum {
    ANGLE_0 = 0,
    ANGLE_90,
    ANGLE_180,
    ANGLE_270,
    ANGLE_FLIPX,
    ANGLE_FLIPY
  };
  //! Return value from oledInit()
  enum {
    OLED_NOT_FOUND = -1, // no display found
    OLED_SSD1306_3C,  // SSD1306 found at 0x3C
    OLED_SSD1306_3D,  // SSD1306 found at 0x3D
    OLED_SH1106_3C,   // SH1106 found at 0x3C
    OLED_SH1106_3D,   // SH1106 found at 0x3D
    OLED_SH1107_3C,   // SH1107
    OLED_SH1107_3D
  };
  enum {
    FONT_NORMAL     =FONT_8x8,
    FONT_SMALL      =FONT_6x8,
    FONT_LARGE      =FONT_16x32,
    FONT_STRETCHED  =FONT_16x16
  };

 private:

  static const uint8_t ucFont[];
#ifndef __MEMORY_SAVEING_SYSTEM__
  static const uint8_t ucBigFont[];
#endif
  // 5x7 font (in 6x8 cell)
  static const uint8_t ucSmallFont[];

  // Initialization sequences
  static const uint8_t oled128_initbuf[];
  static const uint8_t oled64_initbuf[];
  static const uint8_t oled32_initbuf[];
  static const uint8_t oled72_initbuf[];

 private:

  int I2CWrite (CI2C **pI2C, uint8_t iAddr, uint8_t *pData, int iLen);
  uint8_t I2CTest (CI2C **pI2C, uint8_t addr);
  int I2CReadRegister (CI2C **pI2C, uint8_t iAddr, uint8_t u8Register, uint8_t *pData, int iLen);
  int I2CRead (CI2C **pI2C, uint8_t iAddr, uint8_t *pData, int iLen);
  void _I2CWrite (SSOLED *pOLED, uint8_t *pData, int iLen);

  // Send a single byte command to the OLED controller
  void oledWriteCommand (SSOLED *pOLED, uint8_t c);
  void oledWriteCommand2 (SSOLED *pOLED, uint8_t c, uint8_t d);

#ifdef FUTURE
  void oledCachedFlush (void);
  void oledCachedWrite (uint8_t *pData, uint8_t bLen);
#endif // FUTURE

 public:

  COLED ();
  COLED (SSOLED *pOLED, CI2C *dev, int iType, int iAddr, int bFlip, int bInvert, uint8_t *pBuffer = NULL);

  // Initializes the OLED controller into "page mode" on I2C
  // If SDAPin and SCLPin are not -1, then bit bang I2C on those pins
  // Otherwise use the Wire library.
  // If you don't need to use a separate reset pin, set it to -1
  int begin (SSOLED *pOLED, CI2C *dev, int iType, int iAddr, int bFlip, int bInvert, uint8_t *pBuffer = NULL);
  int get_rc (SSOLED *pOLED);

  // Power up/down the display
  // useful for low power situations
  void Power (SSOLED *pOLED, uint8_t bOn);
  // Sets the brightness (0=off, 255=brightest)
  void SetContrast (SSOLED *pOLED, uint8_t ucContrast);
  // Scroll the internal buffer by 1 scanline (up/down)
  // width is in pixels, lines is group of 8 rows
  // Returns 0 for success, -1 for invalid parameter
  int ScrollBuffer (SSOLED *pOLED, int iStartCol, int iEndCol, int iStartRow, int iEndRow, int bUp);

 private:

  // Send commands to position the "cursor" (aka memory write address)
  // to the given row and column
  void oledSetPosition (SSOLED *pOLED, int x, int y, int bRender);
  // Write a block of pixel data to the OLED
  // Length can be anything from 1 to 1024 (whole display)
  void oledWriteDataBlock (SSOLED *pOLED, const uint8_t *ucBuf, int iLen, int bRender);
  // Write a block of flash memory to the display
  void oledWriteFlashBlock (SSOLED *pOLED, uint8_t *s, int iLen);
  // Write a repeating byte to the display
  void oledRepeatByte (SSOLED *pOLED, uint8_t b, int iLen);

 public:

  //! Play a frame of animation data
  // The animation data is assumed to be encoded for a full frame of the display
  // Given the pointer to the start of the compressed data,
  // it returns the pointer to the start of the next frame
  // Frame rate control is up to the calling program to manage
  // When it finishes the last frame, it will start again from the beginning
  uint8_t *PlayAnimFrame (SSOLED *pOLED, uint8_t *pAnimation, uint8_t *pCurrent, int iLen);

  //! Draw a sprite of any size in any position
  // If it goes beyond the left/right or top/bottom edges
  // it's trimmed to show the valid parts
  // This function requires a back buffer to be defined
  // The priority color (0 or 1) determines which color is painted
  // when a 1 is encountered in the source image.
  // e.g. when 0, the input bitmap acts like a mask to clear
  // the destination where bits are set.
  void DrawSprite (SSOLED *pOLED, uint8_t *pSprite, int cx, int cy, int iPitch, int x, int y, uint8_t iPriority);

  //! Draw a 16x16 tile in any of 4 rotated positions
  // Assumes input image is laid out like "normal" graphics with
  // the MSB on the left and 2 bytes per line
  // On AVR, the source image is assumed to be in FLASH memory
  // The function can draw the tile on byte boundaries, so the x value
  // can be from 0 to 112 and y can be from 0 to 6
  void DrawTile (SSOLED *pOLED, const uint8_t *pTile, int x, int y, int iRotation, int bInvert, int bRender);

  //! Set (or clear) an individual pixel
  // The local copy of the frame buffer is used to avoid
  // reading data from the display controller
  // (which isn't possible in most configurations)
  // This function needs the USE_BACKBUFFER macro to be defined
  // otherwise, new pixels will erase old pixels within the same byte
  int SetPixel (SSOLED *pOLED, int x, int y, uint8_t ucColor, int bRender);

  //! Invert font data
  void InvertBytes (uint8_t *pData, uint8_t bLen);

  //! Load a 128x64 1-bpp Windows bitmap
  // Pass the pointer to the beginning of the BMP file
  // First pass version assumes a full screen bitmap
  int LoadBMP (SSOLED *pOLED, const uint8_t *pBMP, int bInvert, int bRender);

  //! Set the current cursor position
  // The column represents the pixel column (0-127)
  // The row represents the text row (0-7)
  void SetCursor (SSOLED *pOLED, int x, int y);

  //! Turn text wrap on or off for the oldWriteString() function
  void SetTextWrap (SSOLED *pOLED, int bWrap);

  //! Draw a string of normal (8x8), small (6x8) or large (16x32) characters
  // At the given col+row with the given scroll offset. The scroll offset allows you to
  // horizontally scroll text which does not fit on the width of the display. The offset
  // represents the pixels to skip when drawing the text. An offset of 0 starts at the beginning
  // of the text.
  // The system remembers where the last text was written (the cursor position)
  // To continue writing from the last position, set the x,y values to -1
  // The text can optionally wrap around to the next line by calling oledSetTextWrap(true);
  // otherwise text which would go off the right edge will not be drawn and the cursor will
  // be left "off screen" until set to a new position explicitly
  //
  //  Returns 0 for success, -1 for invalid parameter
  int WriteString (SSOLED *pOLED, int iScroll, int x, int y, const char *szMsg, int iSize, int bInvert, int bRender);

  //! Render a window of pixels from a provided buffer or the library's internal buffer
  // to the display. The row values refer to byte rows, not pixel rows due to the memory
  // layout of OLEDs. Pass a src pointer of NULL to use the internal backing buffer
  // returns 0 for success, -1 for invalid parameter
  int DrawGFX (SSOLED *pOLED, uint8_t *pBuffer, int iSrcCol, int iSrcRow, int iDestCol, int iDestRow, int iWidth, int iHeight, int iSrcPitch);

  //! Dump a screen's worth of data directly to the display
  // Try to speed it up by comparing the new bytes with the existing buffer
  void DumpBuffer (SSOLED *pOLED, uint8_t *pBuffer);

  //! Fill the frame buffer with a byte pattern
  // e.g. all off (0x00) or all on (0xff)
  void Fill (SSOLED *pOLED, uint8_t ucData, int bRender);

  //! Provide or revoke a back buffer for your OLED graphics
  // This allows you to manage the RAM used by ss_oled on tiny
  // embedded platforms like the ATmega series
  // Pass NULL to revoke the buffer. Make sure you provide a buffer
  // large enough for your display (e.g. 128x64 needs 1K - 1024 bytes)
  void SetBackBuffer (SSOLED *pOLED, uint8_t *pBuffer);

  //! Draw a line between 2 points
  void DrawLine (SSOLED *pOLED, int x1, int y1, int x2, int y2, int bRender);

  //! Draw a string with a fractional scale in both dimensions
  // the scale is a 16-bit integer with and 8-bit fraction and 8-bit mantissa
  // To draw at 1x scale, set the scale factor to 256. To draw at 2x, use 512
  // The output must be drawn into a memory buffer, not directly to the display
  int ScaledString (SSOLED *pOLED, int x, int y, char *szMsg, int iSize, int bInvert, int iXScale, int iYScale, int iRotation);

 private:
  //! For drawing ellipses, a circle is drawn and the x and y pixels are scaled by a 16-bit integer fraction
  // This function draws a single pixel and scales its position based on the x/y fraction of the ellipse
  void DrawScaledPixel (SSOLED *pOLED, int iCX, int iCY, int x, int y, int32_t iXFrac, int32_t iYFrac, uint8_t ucColor);
  //! For drawing filled ellipses
  void DrawScaledLine (SSOLED *pOLED, int iCX, int iCY, int x, int y, int32_t iXFrac, int32_t iYFrac, uint8_t ucColor);
  //! Draw the 8 pixels around the Bresenham circle
  // (scaled to make an ellipse)
  void BresenhamCircle (SSOLED *pOLED, int iCX, int iCY, int x, int y, int32_t iXFrac, int32_t iYFrac, uint8_t ucColor, uint8_t bFill);

 public:
  //! Draw an outline or filled ellipse
  void Ellipse (SSOLED *pOLED, int iCenterX, int iCenterY, int32_t iRadiusX, int32_t iRadiusY, uint8_t ucColor, uint8_t bFilled);
  //! Draw an outline or filled rectangle
  void Rectangle (SSOLED *pOLED, int x1, int y1, int x2, int y2, uint8_t ucColor, uint8_t bFilled);

 private:
  //! Reverse string alignment
  void reverse (char *s, int len);
  //! Integer to String Conversion
  int i2s (int x, char *s, int digit);
  //! raising a power
  int mypower (int nb, int power);
  //! Conversion from double to string (simplified version)
  char *d2s (char *s, double n, int decinum);

public:
  void putc (SSOLED *pOLED, char c);
  void puts (SSOLED *pOLED, char *s);
  int printf (SSOLED *pOLED, const char *fmt, ...);
};
