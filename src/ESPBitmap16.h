/*
ESPBitmap Library, 16 bit optimized version
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

#ifndef _ESPBITMAP16_H_
#define _ESPBITMAP16_H_

#include <inttypes.h>
#include "ESPBitmapBase.h"

class ESPBitmap16 : public ESPBitmapBase
{
  private:
    uint16_t * palette = 0;
    uint8_t * colorData = 0;
    
  public:
    ESPBitmap16();
    ~ESPBitmap16();

    //decodes a bitmap from a buffer array.
    //Expects entire file to be present in the byte array
    BITMAP_RESULT_t DecodeFileBuffer(uint8_t *wholeFileBytes, int32_t length);

    uint16_t getPixel(int x, int y);
    uint16_t ERROR_COLOR;

#ifdef ESP8266
  BITMAP_RESULT_t getFromStream(Stream* stream, int len, int timeoutMs);
  //BITMAP_RESULT_t StreamDecode(Stream* stream, int len, int timeoutMs, void (*decodeCallBack)(BITMAP_RESULT_t, uint16_t, int, int));
#endif //ESP8266
};

#endif /*_ESPBITMAP16_H_*/
