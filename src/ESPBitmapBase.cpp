/*
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
#include <pins_arduino.h>
#include "ESPBitmapBase.h"

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

#ifdef ESP8266
BITMAP_RESULT_t ESPBitmapBase::fetchImageFromUrl(String imageUrl){
  fetchImageFromUrl(imageUrl, 5000);
}

BITMAP_RESULT_t ESPBitmapBase::fetchImageFromUrl(String imageUrl, int timeoutMs){
  HTTPClient http;
  http.begin(imageUrl);
  int httpCode = http.GET();
  DEBUG_PRINTLN("[HTTP] GET..." + imageUrl);
  if (httpCode > 0) {
    DEBUG_PRINT(F("[HTTP] RETURN CODE WAS: "));
    DEBUG_PRINTLN(httpCode);
    if (httpCode == HTTP_CODE_OK) {
      return getFromStream(http.getStreamPtr(), http.getSize(), timeoutMs);
    }//END OK
  }//EMD CODE>0
  return BITMAP_ERROR_FETCH_FAILED;
}
#endif

void ESPBitmapBase::printResult(BITMAP_RESULT_t errCode){
  Serial.print(F("ESPBitmap Result: "));
  switch(errCode){
    case BITMAP_SUCCESS: Serial.println(F("SUCCESS")); break;
    case BITMAP_ERROR_UNSUPPORTED_COMPRESSION: Serial.println(F("Unsupported Compression type")); break;
    case BITMAP_ERROR_TOO_SHORT: Serial.println(F("Too Short, not enough bytes supplied to be a bitmap")); break;
    case BITMAP_ERROR_INVALID_FHEADER: Serial.println(F("Invalid file header (first 14 bytes)")); break;
    case BITMAP_ERROR_INVALID_IHEADER: Serial.println(F("Invalid bitmap info header (bytes 15 to 54")); break;
    case BITMAP_ERROR_UNSUPPORTED_BITDEPTH: Serial.println(F("Unsupported bit depth, only 1, 4, 8, 24 supported.")); break;
    case BITMAP_ERROR_OUT_OF_MEMORY: Serial.println(F("Out of memory- failed allocation")); break;
    case BITMAP_ERROR_FETCH_FAILED: Serial.println(F("http fetch failed,")); break;
    default: Serial.println(F("UNKNOWN")); break;
  }
}

int32_t ESPBitmapBase::getWidth() {
  return width;
}

int32_t ESPBitmapBase::getHeight() {
  return height;
}

uint16_t ESPBitmapBase::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint16_t)(r & 0xF8) << 8) |
         ((uint16_t)(g & 0xFC) << 3) |
                    (b         >> 3);
}