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

#include <Arduino.h>
#include "ESPBitmap.h"
#include <pins_arduino.h>

#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <stream.h>
#endif

//TODO, for 16 color mode, we need to create a colorData16 array, and proccess all the pixels into that (only for 24 bit)

#define DEBUG
#define DEBUG_FINE
//#define FAST_AND_LOOSE  //skip some error checking and risk undesirable behavior for speed.

#ifdef DEBUG
 #define DEBUG_PRINT(x) Serial.print(x)
 #define DEBUG_PRINTLN(x) Serial.println(x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x) 
#endif

#ifdef DEBUG_FINE
 #define DEBUG_FINE_PRINT(x) Serial.print(x)
 #define DEBUG_FINE_PRINTLN(x) Serial.println(x)
#else
 #define DEBUG_FINE_PRINT(x)
 #define DEBUG_FINE_PRINTLN(x) 
#endif

ESPBitmap::ESPBitmap(){
  DEBUG_PRINTLN(F("Construct ESPBitmap Object"));
  ERROR_COLOR.a = 0;
  ERROR_COLOR.r = 255;
  ERROR_COLOR.g = 0;
  ERROR_COLOR.b = 255;
  width = 0;
  height = 0;
}

ESPBitmap::~ESPBitmap(){
  DEBUG_PRINTLN(F("Deconstruct ESPBitmap Object"));
  if(palette != 0)
    delete[]  palette;
  if(colorData != 0)
    delete[]  colorData;
}

BITMAP_RESULT_t ESPBitmap::DecodeFileBuffer(uint8_t *wholeFileBytes, int32_t length)
{
    // make sure the buffer comming in is big enough to actually contain a bitmap header.
    if(length < sizeof(BITMAP_FILE_HEADER_t) + sizeof(BITMAP_INFO_HEADER_t))
      return BITMAP_ERROR_TOO_SHORT;

    //get header
    BITMAP_FILE_HEADER_t bitmapHeader;
    memcpy(&bitmapHeader, wholeFileBytes, sizeof(BITMAP_FILE_HEADER_t));

    DEBUG_PRINT(F("HeaderKey: "));
    DEBUG_PRINTLN(bitmapHeader.headerKey);
    DEBUG_PRINT(F("FileSize: "));
    DEBUG_PRINTLN(bitmapHeader.filesize);
    DEBUG_PRINT(F("data offset: "));
    DEBUG_PRINTLN(bitmapHeader.dataOffset);

    //bitmap header MUST start with 'BM' in ASCII 
    if(bitmapHeader.headerKey != 0x4D42)
      return BITMAP_ERROR_INVALID_FHEADER;

    dataOffset = bitmapHeader.dataOffset;
    
    //get bitmap info
    BITMAP_INFO_HEADER_t bitmapInfo;
    memcpy(&bitmapInfo, wholeFileBytes + sizeof(BITMAP_FILE_HEADER_t), sizeof(BITMAP_INFO_HEADER_t));

    DEBUG_PRINT("headersize: ");
    DEBUG_PRINTLN(bitmapInfo.headerSize);
    DEBUG_PRINT("width: ");
    DEBUG_PRINTLN(bitmapInfo.width);
    DEBUG_PRINT("height: ");
    DEBUG_PRINTLN(bitmapInfo.height);
    DEBUG_PRINT("Planes: ");
    DEBUG_PRINTLN(bitmapInfo.planes);
    DEBUG_PRINT("BitsPerPixel: ");
    DEBUG_PRINTLN(bitmapInfo.bitsPerPixel);
    DEBUG_PRINT("DataSize: ");
    DEBUG_PRINTLN(bitmapInfo.dataSize);
    DEBUG_PRINT("Compression: ");
    DEBUG_PRINTLN(bitmapInfo.compression);
    DEBUG_PRINT("colorsUsed: ");
    DEBUG_PRINTLN(bitmapInfo.colorsUsed);
    DEBUG_PRINT("importantColors: ");
    DEBUG_PRINTLN(bitmapInfo.importantColors);
    
    //bitmap header must at least be 40 for a windows compatibile bitmap image. OS/2 bitmaps are 12
    //planes must always be 1 (this is a furture proofing property that was never realized.)
    if(bitmapInfo.headerSize < 40 || bitmapInfo.planes != 1)
      return BITMAP_ERROR_INVALID_IHEADER;

    // currently no compression is supproted.
    if(bitmapInfo.compression != BI_UNCOMPRESSED)
      return BITMAP_ERROR_UNSUPPORTED_COMPRESSION;

    width = bitmapInfo.width;
    height = bitmapInfo.height;

    // if height is negeative, then the image is stored flipped vertically
    // normal bitmap is bottom to top left to right, flipped is top to bottom left to right
    if(height < 0) {
      flipped = true;
      height *= -1;
    }

    bitsPerPixel = bitmapInfo.bitsPerPixel;

    //based on the bit depth, we may or may not need to load the palette.
    size_t colorsToLoad = 0;
    switch (bitsPerPixel) {
      case 1: colorsToLoad = (bitmapInfo.colorsUsed == 0 || bitmapInfo.colorsUsed > 2) ? 2 : bitmapInfo.colorsUsed; break;
      case 4: colorsToLoad = (bitmapInfo.colorsUsed == 0 || bitmapInfo.colorsUsed > 16) ? 16 : bitmapInfo.colorsUsed; break;
      case 8: colorsToLoad = (bitmapInfo.colorsUsed == 0 || bitmapInfo.colorsUsed > 256) ? 256 : bitmapInfo.colorsUsed; break;
      case 24: colorsToLoad = 0; break;
      default: return BITMAP_ERROR_UNSUPPORTED_BITDEPTH; break;
    }

    //if we need a palette, load it.
    if(colorsToLoad > 0){
      palette = new PIXEL_t[colorsToLoad];
      if(palette == 0)
        return BITMAP_ERROR_OUT_OF_MEMORY;
      
      for(int i = 0; i < colorsToLoad; i++){
        int index = (sizeof(BITMAP_FILE_HEADER_t) /* should be 14 */ + bitmapInfo.headerSize) + (4 * i); //pallate of supported types starts at 54 but header could have other stuff
          PIXEL_t nPix;
          nPix.b = wholeFileBytes[index    ];
          nPix.g = wholeFileBytes[index + 1];
          nPix.r = wholeFileBytes[index + 2];
          nPix.a = wholeFileBytes[index + 3];
          palette[i] = nPix;
      }
    }

    //load all the color data, keeping it in whatever format it was in.
    //we don't want to parse it into pure colors, because we want to save all the ram we can.
    data_length = bitmapInfo.dataSize;
    if(data_length == 0)
      data_length = bitmapHeader.filesize - dataOffset;
    colorData = new uint8_t[data_length];
    if(colorData == 0)
      BITMAP_ERROR_OUT_OF_MEMORY;

    memcpy(colorData, wholeFileBytes + dataOffset, data_length);

    //now we have all the data loaded in colorData and the palette loaded if needed.
    return BITMAP_SUCCESS;
}

#ifdef ESP8266

BITMAP_RESULT_t ESPBitmap::getFromStream(Stream* stream,int len, int timeoutMs){

  long startMs = millis();
  size_t readOffset = 0;
  size_t totalBytesToRead = len;

  BITMAP_FILE_HEADER_t bitmapHeader;
  BITMAP_INFO_HEADER_t bitmapInfo;
  size_t colorsToLoad = 0;
  size_t colorsLoaded = 0;
  size_t data_length = 0;
  const size_t InfoHeaderEndOffset = sizeof(BITMAP_INFO_HEADER_t) + sizeof(BITMAP_FILE_HEADER_t);

  while (millis() - startMs < timeoutMs && (len > 0 || len == -1)) {
    size_t size = stream->available();
    DEBUG_FINE_PRINT(F("AVAILABLE STREAM BYTES: "));
    DEBUG_FINE_PRINTLN(size);
    DEBUG_FINE_PRINT(F("ReadOffset IS: "));
    DEBUG_FINE_PRINTLN(readOffset);
    
    if (size) {
        int c = 0;
        //parse the file header;
        if(readOffset < sizeof(BITMAP_FILE_HEADER_t))
        {
          DEBUG_FINE_PRINTLN(F("IN HEADER PARSE"));
          c = stream->readBytes((unsigned char *)&bitmapHeader + readOffset, size > sizeof(BITMAP_FILE_HEADER_t) - readOffset ? sizeof(BITMAP_FILE_HEADER_t) - readOffset : size);
          readOffset += c;

          //check if we got the whole header
          if(readOffset >= sizeof(BITMAP_FILE_HEADER_t))
          {
            DEBUG_PRINT(F("HeaderKey: "));
            DEBUG_PRINTLN(bitmapHeader.headerKey);
            DEBUG_PRINT(F("FileSize: "));
            DEBUG_PRINTLN(bitmapHeader.filesize);
            DEBUG_PRINT(F("data offset: "));
            DEBUG_PRINTLN(bitmapHeader.dataOffset);

            //bitmap header MUST start with 'BM' in ASCII 
            if(bitmapHeader.headerKey != 0x4D42)
              return BITMAP_ERROR_INVALID_FHEADER;

            dataOffset = bitmapHeader.dataOffset;
          }
        }
        //parse the bitmap info header
        else if(readOffset < InfoHeaderEndOffset)
        {
          DEBUG_FINE_PRINTLN(F("IN INFO PARSE"));
          size_t infoHeaderReadSoFar = (readOffset - sizeof(BITMAP_FILE_HEADER_t));
          size_t infoHeaderRemaining = (sizeof(BITMAP_INFO_HEADER_t) - infoHeaderReadSoFar);
          
          c = stream->readBytes((unsigned char *)&bitmapInfo + infoHeaderReadSoFar, size > infoHeaderRemaining ? infoHeaderRemaining : size);
          readOffset += c;

          //check if we got the whole INFO section
          if(readOffset >= InfoHeaderEndOffset)
          {
            DEBUG_PRINT("headersize: ");
            DEBUG_PRINTLN(bitmapInfo.headerSize);
            DEBUG_PRINT("width: ");
            DEBUG_PRINTLN(bitmapInfo.width);
            DEBUG_PRINT("height: ");
            DEBUG_PRINTLN(bitmapInfo.height);
            DEBUG_PRINT("Planes: ");
            DEBUG_PRINTLN(bitmapInfo.planes);
            DEBUG_PRINT("BitsPerPixel: ");
            DEBUG_PRINTLN(bitmapInfo.bitsPerPixel);
            DEBUG_PRINT("DataSize: ");
            DEBUG_PRINTLN(bitmapInfo.dataSize);
            DEBUG_PRINT("Compression: ");
            DEBUG_PRINTLN(bitmapInfo.compression);
            DEBUG_PRINT("colorsUsed: ");
            DEBUG_PRINTLN(bitmapInfo.colorsUsed);
            DEBUG_PRINT("importantColors: ");
            DEBUG_PRINTLN(bitmapInfo.importantColors);

            //bitmap header must at least be 40 for a windows compatibile bitmap image. OS/2 bitmaps are 12
            //planes must always be 1 (this is a furture proofing property that was never realized.)
            if(bitmapInfo.headerSize < 40 || bitmapInfo.planes != 1)
              return BITMAP_ERROR_INVALID_IHEADER;

            // currently no compression is supproted.
            if(bitmapInfo.compression != BI_UNCOMPRESSED)
              return BITMAP_ERROR_UNSUPPORTED_COMPRESSION;

            width = bitmapInfo.width;
            height = bitmapInfo.height;

            // if height is negeative, then the image is stored flipped vertically
            // normal bitmap is bottom to top left to right, flipped is top to bottom left to right
            if(height < 0) {
              flipped = true;
              height *= -1;
            }

            bitsPerPixel = bitmapInfo.bitsPerPixel;

            //based on the bit depth, we may or may not need to load the palette.
            switch (bitsPerPixel) {
              case 1: colorsToLoad = (bitmapInfo.colorsUsed == 0 || bitmapInfo.colorsUsed > 2) ? 2 : bitmapInfo.colorsUsed; break;
              case 4: colorsToLoad = (bitmapInfo.colorsUsed == 0 || bitmapInfo.colorsUsed > 16) ? 16 : bitmapInfo.colorsUsed; break;
              case 8: colorsToLoad = (bitmapInfo.colorsUsed == 0 || bitmapInfo.colorsUsed > 256) ? 256 : bitmapInfo.colorsUsed; break;
              case 24: colorsToLoad = 0; break;
              default: return BITMAP_ERROR_UNSUPPORTED_BITDEPTH; break;
            }

            if(colorsToLoad > 0){
              palette = new PIXEL_t[colorsToLoad];
              if(palette == 0)
                return BITMAP_ERROR_OUT_OF_MEMORY;
            }

            data_length = bitmapInfo.dataSize;
            if(data_length == 0)
              data_length = bitmapHeader.filesize - dataOffset;
            
            colorData = new uint8_t[data_length];
            if(colorData == 0)
              return BITMAP_ERROR_OUT_OF_MEMORY;
          }
        }
        //make sure if we have any useless > 40 bytes info header, that we throw it away.
        else if(readOffset < bitmapInfo.headerSize + sizeof(BITMAP_FILE_HEADER_t)){
          DEBUG_FINE_PRINTLN(F("IN UNKNOWN HEADER"));
          while(readOffset < (bitmapInfo.headerSize + sizeof(BITMAP_FILE_HEADER_t)) && stream->available()){
          int trash = stream->read();
          readOffset++;
          DEBUG_FINE_PRINTLN(F("UNKNOWN HEADER BYTE"));
          }
        }
        //load palette if necessary
        else if(colorsToLoad > 0 && readOffset < dataOffset && colorsLoaded < colorsToLoad){
          DEBUG_FINE_PRINT(F("ColorsToLoad: "));
          DEBUG_FINE_PRINTLN(colorsToLoad);
          DEBUG_FINE_PRINTLN(F("IN PARSE PALETTE adding colors: "));
          c = 0;
          //this is tricky because have to always have a whole color available to store it.
          size_t availSize = size;
          while(availSize >= 4 && colorsLoaded < colorsToLoad){
            PIXEL_t nPix;
            nPix.b = (uint8_t)stream->read();
            nPix.g = (uint8_t)stream->read();
            nPix.r = (uint8_t)stream->read();
            nPix.a = (uint8_t)stream->read();
            palette[colorsLoaded++] = nPix;
            availSize -= 4;
            readOffset += 4;
            c+=4;
            DEBUG_FINE_PRINT(F("{"));
            DEBUG_FINE_PRINT(nPix.r);
            DEBUG_FINE_PRINT(F(","));
            DEBUG_FINE_PRINT(nPix.g);
            DEBUG_FINE_PRINT(F(","));
            DEBUG_FINE_PRINT(nPix.b);
            DEBUG_FINE_PRINT(F("} "));
          }
            DEBUG_FINE_PRINTLN(" End Palette.");
            DEBUG_FINE_PRINT(F("Colors Loaded: "));
            DEBUG_FINE_PRINTLN(colorsLoaded);
        }
        //Finally, we actually load the colorData
        else if(readOffset >= dataOffset){
          DEBUG_FINE_PRINTLN(F("IN READ DATA"));
          size_t DataReadSoFar = (readOffset - dataOffset);
          size_t DataRemaining = (data_length - DataReadSoFar);
          c = stream->readBytes(colorData + DataReadSoFar, size > DataRemaining ? DataRemaining : size);
          readOffset += c;
        }

        if (len > 0) {
          len -= c;
        }

        if(len == 0)
          return BITMAP_SUCCESS;

        //if len ==0, this should already be true, but for saftey
        if(readOffset >= totalBytesToRead)
          return BITMAP_SUCCESS;
    }//END SIZE
    DEBUG_FINE_PRINT("DELAY FOR BUFFER. Offset Currently:");
    DEBUG_FINE_PRINTLN(readOffset);
    delay(1);
  }//END WHILE CONNECTED

  //timed out
  return BITMAP_ERROR_FETCH_FAILED;
}
#endif


PIXEL_t ESPBitmap::getPixel(int x, int y){

    //if the color's array hasn't been initialized, then this is an empty image object
    if(colorData == 0) return ERROR_COLOR;

    #ifndef FAST_AND_LOOSE
    //bounds checking, this is a lot of unneccesary overhead,
    //but in intrest of foolproofing, it's nice to have.
    if(x < 0)
      x = 0;
    else if(x > width - 1)
      x = width -1;

    if(y < 0)
      y = 0;
    else if(y > height - 1)
      y = height -1;
    #endif

    //assume y goes top to bottom like in all other graphics, handle flipped.
    //not if (flipped) image is stored top to bottom, otherwise it is bottom to top.
    if(!flipped)
      y = (height-1) - y;


    //general reference (I tend to forget these rules)
    //Storing 2D in 1D array, using mappings:
    //i = x + width*y;
    //and
    //x = i % width;    // % is the "modulo operator", the remainder of i / width;
    //y = i / width;    // where "/" is an integer division
    
    //for some strange reason bitmap scanlines are padded if need be to a 4-byte boundary, unused padding bytes full of 0s
    //size_t ScanlineWidth = (width * 3 + 3) & ~3; //This magic only works with 24bpp; If anyone knows math well enough to apply this to Nbpp, pull requests welcome.
    size_t ScanlineWidth = 4 * ((int)( ((width * bitsPerPixel) + 31) / 32));
    
    switch (bitsPerPixel) {
      case 1:
        return palette[(colorData[ScanlineWidth * y + (x>>3)]) >> (7 - (x % 8)) & 0x01];
        break;
      case 4:
        return palette[((colorData[ScanlineWidth * y + (x>>1)]) >> ((x%2==0)? 4:0)) & 0x0F];
        break;
      case 8:
        return palette[(colorData[ScanlineWidth * y + x])];
        break;
      case 24:
        {
          int offset = x * 3 + ScanlineWidth * y;
          PIXEL_t pix;
          pix.b = colorData[offset];
          pix.g = colorData[offset + 1];
          pix.r = colorData[offset + 2];
          pix.a = 0;
          return pix;
        }
        break;
      default:
        return ERROR_COLOR; break;
    }
}