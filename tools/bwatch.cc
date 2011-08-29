/*
    Copyright (C) 2011, Alberto Mattea

    Some parts are inspired from bjavaloader

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License in the COPYING file at the
    root directory of this project for more details.
*/


#include <barry/barry.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <string.h>
#include <SDL/SDL.h>


using namespace std;
using namespace Barry;

int main() {

  // Init SDL
  bool sdl_started=false;
  SDL_Surface *screen=NULL;
  SDL_Event event;
  int keypress = 0;
  if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;

  // Barry variables
  cout.sync_with_stdio(true); // leave this on, since libusb uses stdio for debug messages
  uint32_t pin = 0;
  bool data_dump = false;
  string password;
  vector<string> params;
  string busname;
  string devname;
  string iconvCharset;
  Usb::EndpointPair epOverride;

  // Initialize the barry library.  Must be called before
  // anything else.
  Barry::Init(data_dump);

  JLScreenInfo info;
  Data image;
  const uint16_t* rawdata;
  unsigned char* bitmapdata;
  unsigned char* origvalue;
  if ((bitmapdata=(unsigned char*)malloc(1000000))==NULL) {
    cerr << "Cannot allocate buffer" << endl;
    return 1;
  }
  origvalue=bitmapdata;

  // Main loop
  while(!keypress) {
    // Put this inside it's own block to avoid blocking the handheld
    {
      // Probe the USB bus for Blackberry devices and display.
      // If user has specified a PIN, search for it in the
      // available device list here as well
      Barry::Probe probe;
      int activeDevice = probe.FindActive(pin);
      if( activeDevice == -1 ) {
        cerr << "No device selected, or PIN not found" << endl;
        return 1;
      }

      // Create our controller object
      Barry::Controller con(probe.Get(activeDevice));
      Barry::Mode::JavaLoader javaloader(con);
      javaloader.Open(password.c_str());
      javaloader.StartStream();
      // Take a screenshot
      //   - info object contains the screenshot properties (width, height...)
      //   - image will be filled with the raw pixel screenshot data
      javaloader.GetScreenshot(info, image);
      javaloader.StopStream();
    }
    // The first time set the video mode according to the screenshot data
    if (!sdl_started) {
      if (!(screen = SDL_SetVideoMode(info.width, info.height, 0, SDL_HWSURFACE))) {
        SDL_Quit();
        return 1;
      }
      sdl_started=true;
      SDL_WM_SetCaption("Blackberry", 0);
    }
    // May want to tune this between 100 and 1000
    SDL_Delay(500);
    // Convert to 24-bit RGB
    rawdata=(const uint16_t*)image.GetData();
    for (size_t j=0; j<info.height; j++) {
      for (size_t i=0; i<info.width; i++) {
        // Read one pixel in the picture
        short value=rawdata[i+j*info.width];
        // Pixel format used by the handheld is : 16 bits
        // MSB < .... .... .... .... > LSB
        //                    ^^^^^^ : Blue (between 0x00 and 0x1F)
        //             ^^^^^^^ : Green (between 0x00 and 0x3F)
        //       ^^^^^^ : Red (between 0x00 and 0x1F)
        bitmapdata[2] = (((value >> 11) & 0x1F) * 0xFF) / 0x1F;      // red
        bitmapdata[1] = (((value >> 5) & 0x3F) * 0xFF) / 0x3F;       // green
        bitmapdata[0] = ((value & 0x1F) * 0xFF) / 0x1F;              // blue
        bitmapdata += 3;
      }
    }
    bitmapdata=origvalue;
    // Do the blit
    SDL_Surface *tmp;
    tmp=SDL_CreateRGBSurfaceFrom(bitmapdata, info.width, info.height, 24, info.width*3, 0, 0, 0, 0);
    SDL_BlitSurface(tmp, 0, screen, 0);
    SDL_Flip(screen);
    while(SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          keypress = 1;
          break;
        case SDL_KEYDOWN:
          keypress = 1;
          break;
      }
    }
  }
    
  // Stop SDL
  SDL_Quit();
  return 0;
}


