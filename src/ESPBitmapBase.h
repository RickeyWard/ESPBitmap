/*
ESPBitmap Library
Copyright 2018 Rickey Ward

MIT License
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions: The
above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS
IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _ESPBITMAPBASE_H_
#define _ESPBITMAPBASE_H_

#include <inttypes.h>

#pragma pack(push, 1) //might need to be 2

struct PIXEL_t{
  uint8_t a;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct BITMAP_FILE_HEADER_t {
  uint16_t headerKey;   //will be char header[2] {'B', 'M'} if valid
  int32_t  filesize;    //whole file size in bytes;
  uint16_t reserved1;   //unused
  uint16_t reserved2;   //unused
  uint32_t dataOffset;  //byte offset that actual image data starts.
};

struct BITMAP_INFO_HEADER_t {
  int32_t headerSize; //typically 40, we will only be reading the first 40 (minimum requirement)
  int32_t width;
  int32_t height;
  int16_t planes; // will be 1
  int16_t bitsPerPixel; // 1, 2, 4, 8, 16, 24
  int32_t compression;
  int32_t dataSize;
  int32_t hResolution_pixPerMeter; //used for printing
  int32_t vResolution_pixPerMeter; //used for printing
  int32_t colorsUsed; //number of colors in the palette, 0 for max of current bit depth. (typically 0)
  int32_t importantColors; //number of colors that are considered important in the palette (must be start of palette) if zero used, all colors are important. Typically 0
};

#pragma pack(pop)

typedef enum
{
  BI_UNCOMPRESSED = 0, //RGB format
  BI_RLE_8 = 1, //usable only with 8-bit images
  BI_RLE_4 = 2, //usable only with 4-bit images
  BI_BITFIELDS = 3 //Used and required only with 16 and 32 bit images
} BITMAP_COMPRESSION_t;

typedef enum
{
  BITMAP_SUCCESS = 0,
  BITMAP_ERROR_UNSUPPORTED_COMPRESSION,
  BITMAP_ERROR_TOO_SHORT,
  BITMAP_ERROR_INVALID_FHEADER,
  BITMAP_ERROR_INVALID_IHEADER,
  BITMAP_ERROR_UNSUPPORTED_BITDEPTH,
  BITMAP_ERROR_OUT_OF_MEMORY,
  BITMAP_ERROR_FETCH_FAILED
} BITMAP_RESULT_t;

class ESPBitmapBase
{

  public:
    //prints to Serial the result according to the code passed.
    void printResult(BITMAP_RESULT_t errCode);

    //decodes a bitmap from a buffer array. Expects entire file to be present in the byte array
    virtual BITMAP_RESULT_t DecodeFileBuffer(uint8_t *wholeFileBytes, int32_t length);

    int32_t getWidth();
    int32_t getHeight();

    int32_t width = 0;
    int32_t height = 0;
    int32_t dataOffset = 0;
    size_t data_length = 0;
    int16_t bitsPerPixel = 0;
    bool flipped = false;

    //RGB888-24 to RGB565-16 (565 is standard for adafruit's amazing graphics library)
    //this implimentation is taken from there. 
    static uint16_t Color(uint8_t r, uint8_t g, uint8_t b);
    
#ifdef ESP8266
  BITMAP_RESULT_t fetchImageFromUrl(String imageUrl);
  BITMAP_RESULT_t fetchImageFromUrl(String imageUrl, int timeoutMs);
  virtual BITMAP_RESULT_t getFromStream(Stream* stream, int len, int timeoutMs);
#endif //ESP8266
};

#endif /*_ESPBITMAPBASE_H_*/
