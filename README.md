# ESPBitmap Library
Copyright 2018 Rickey Ward

Bitmap reading/buffering was annoying. I couldn't find a library that worked how I wanted so I wrote this one for my own project. I wanted to share it with the community that has shared so much with me.

The emphasis of this library is ease of use, and reasonable tradeoffs between what bitmaps are supported and the footprint of the library.

## Basic Usage
```cpp
//include standard
#include <ESPBitmap.h>
//or 16bit version
#include <ESPBitmap16.h>

//create an instance of an image
ESPBitmap bitmap;
//or
ESPBitmap16 bitmap;

//note that as long as the bitmap object is in scope. It will hold onto whatever data was loaded.
//It is not recommended to use the same object over and over,
//but instead keep in the smallest scope needed to free the memory afterward.
//that being said, you can keep it around to redraw it later. memory usage is resonable, but still.
//Bytes are bytes, and bitmaps are typically uncompressed.
//though palettized bitmaps such as 16 color 4bpp are pretty resonable on an esp8266.

//if you have a byte array that contains all the file bytes of an image (unsigned char or uint8_t) you can proccess the file data using 
BITMAP_RESULT_t res = bitmap.DecodeFileBuffer(/*uint8_t*/ wholeFileSrray, /*int32_t*/ length);

//or if you're on the esp8266 you can pass a stream into
BITMAP_RESULT_t res = bitmap.getFromStream(/*Stream**/ stream, /*int*/ len, /*int*/ timeoutMs);
//but the easiest way for the esp8266 is to pass a url into 
BITMAP_RESULT_t res = bitmap.fetchImageFromUrl(/*String*/ imageUrl);

//once you've ran one of these and gotten back a result, you can check the resut for a success. Which will be one of the constants:
//   BITMAP_SUCCESS = 0
//   BITMAP_ERROR_UNSUPPORTED_COMPRESSION,
//   BITMAP_ERROR_TOO_SHORT,
//   BITMAP_ERROR_INVALID_FHEADER,
//   BITMAP_ERROR_INVALID_IHEADER,
//   BITMAP_ERROR_UNSUPPORTED_BITDEPTH,
//   BITMAP_ERROR_OUT_OF_MEMORY,
//   BITMAP_ERROR_FETCH_FAILED

//the result can be printed to serial for debugging using:
bitmap.printResult(/*BITMAP_RESULT_t*/ res);

//if the result is success you can
    //getPixel(x,y) to get the color data.
    //getWidth() to get the image width.
    //getHeight() to get the image height.

if(res == BITMAP_SUCCESS)
{
    for(int y = 0; y < bitmap.getHeight(); y++)
        for(int x = 0; x < bitmap.getWidth(); x++)
        {
            PIXEL_t pix = bitmap.getPixel(x, y);
            //Do something with Pixel
            Serial.printf("[x:%d y:%d {%d,%d,%d}]", x, y, pix.r, pix.g, pix.b);
        }
}

```

## TODOs
* Add true ESP32 support (haven't looked into what it takes, just know that it doesn't fully work. The base full buffer proccessing will work, but no stream support)
* extend pure Arduino support (currently works for full image buffers only i.e. no stream support for non ESP8266)
* Support proccessing bitmap stream as it's available without needing to store entire image before showing pixels (will be done will callbacks)
* Create more examples that show all the different ways to use the lib

## Notes
* Why isn't native 16bpp decoding supported? Simple, few software packages allow you to save in this format directly. It's part of the spec, but it's rarely used and out of the 6 image editors I had installed I wasn't able to make one. So It would bloat the libary.
* the 16bit version of the ESPBitmap class, `ESPBitmap16` is best for conserving ram while still supporting many colors.
    * 1, 4, 16 bpp: converts palette colors from bgra to rbg565. Addressing data is unaltered.
    * 24 bpp: converts 3 byte (4 byte alligned) raw data into rgb565 unpadded.
* can I use this libary with an SD card or SPIFFS? YES! check out the `getFromStream` function. anything that inherits from an ESPCore stream that exposes the `Stream` functions to you can just be passed in.

### MIT License
>Permission is hereby granted, free of charge, to any person
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
