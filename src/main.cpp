/*
 *This program is a simple music player
 *
 *Copyright © 2015 Kwon-Young Choi
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 *a copy of this software and associated documentation files (the "Software"),
 *to deal in the Software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the Software, and to permit persons to whom the
 *Software is furnished to do so, subject to the following conditions:
 *
 *The above copyright notice and this permission notice shall be included
 *in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 *OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <SFML/Graphics.hpp>
#include <libtools.h>
#include "RtAudio.h"

typedef float  MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32
#define SCALE  1.0;

struct OutputData {
  //decoder to read the music file
  BassDecoder myDecoder;
  //stereo sound
  Signal right_out;
  Signal left_out;
  int playState;
  int forwardState;
  volatile bool push;
};

// Interleaved buffers
int output( void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
            double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data )
{
  OutputData *oData = (OutputData*) data;

  // In general, it's not a good idea to do file input in the audio
  // callback function but I'm doing it here because I don't know the
  // length of the file we are reading.
  //unsigned int count = fread( outputBuffer, oData->channels * sizeof( MY_TYPE ), nBufferFrames, oData->fd);
  memcpy((MY_TYPE*)(outputBuffer),
         oData->left_out.samples,
         nBufferFrames * sizeof( MY_TYPE));
  memcpy((MY_TYPE*)(outputBuffer)+nBufferFrames,
         oData->right_out.samples,
         nBufferFrames * sizeof( MY_TYPE));

  oData->push=true;
  std::cout << "sample played\n" << std::endl;
  return 0;
}

int main(int argc, char* argv[])
{
  unsigned int bufferFrames=0;
  //this line is important to be right on the start of the program
  //it configure the size of a signal
  Signal::globalConfigurationFromPow2(48000, 512);

  string_t music_file;
  std::vector<string_t> music_files;
  if (argc >1) {
    for (int i=1; i<argc; i++) {
      //retrieve the name of the file to be played
      music_file = argv[i];
      music_files.push_back(music_file);
    }
  }
  int current_music_file = -1;

  RtAudio dac;
  if ( dac.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 0 );
  }

  // Set our stream parameters for output only.
  bufferFrames = Signal::size;
  RtAudio::StreamParameters oParams;
  oParams.deviceId = 0;
  oParams.nChannels = 2;
  oParams.firstChannel = 0;

  //load bass dll
  if (load_bass_procs("bass.dll") == -1) {
    std::cerr << "Couldn't load the library bass.dll"
              << std::endl;
    return 0xDEAD;
  }
  //initialize bass
  if (!BASS_EnsureBassInit(0, Signal::frequency, 0)) {
    std::cerr << "Couldn't initialize bass" << std::endl;
    return 0xDEAD;
  }

  OutputData data;


  oParams.deviceId = dac.getDefaultOutputDevice();

  std::cout << "\nPlaying mp3 file " << music_file.toAnsiString()
            << " (buffer frames = " << bufferFrames
            << ")." << std::endl;

  NEWindow::registerClass();
  NEWindow window(sf::VideoMode(800, 400), "Millenium Player", sf::Font());
  //sf::RenderWindow window(sf::VideoMode(800, 400),
  //                  "Millenium Player");

  //declare player interface
  Interface player(sf::Vector2u(400, 400),
                   sf::Vector2f(400, 400));
  sf::IntRect idle_area(0, 0, 400, 400);
  sf::IntRect clicked_area(400, 0, 400, 400);
  Button play(sf::Vector2f(200, 200), "play", sf::Font());
  sf::Texture tx_play;
  if(!tx_play.loadFromFile("PlayPause.png")) {
    std::cerr << "couldn't load play button texture"
              << std::endl;
  }
  tx_play.setSmooth(true);
  play.setTexture(tx_play, idle_area, clicked_area);
  play.setPosition(100, 100);
  play.setWin98Looking(false);
  player.addMouseCatcher(&play);
  Button forward(sf::Vector2f(200, 200), "forward", sf::Font());
  sf::Texture tx_forward;
  if(!tx_forward.loadFromFile("Forward.png")) {
    std::cerr << "couldn't load forward button texture"
              << std::endl;
  }
  tx_forward.setSmooth(true);
  forward.setTexture(tx_forward, idle_area, clicked_area);
  forward.setPosition(500, 100);
  forward.setWin98Looking(false);
  player.addMouseCatcher(&forward);
  play.linkTo(&(data.playState));
  play.forceOff();
  forward.linkTo(&(data.forwardState), ButtonMode::on);
  window.registerInterface(player);
  window.arrange();

  GLSLRender myShader;
  Sampler1D mySampler;
  mySampler.create(Signal::size, NULL);
  myShader.loadFromMemory(GLSLRender::getVertexShaderCode(), lightsaber_signal_fs_src);
  myShader.setParameter("signal", mySampler);
  myShader.setParameter("thickness", 0.1f);
  myShader.setParameter("reversed", false);
  myShader.setParameter("color", sf::Color(0, 140, 0, 140));
  Signal copy;
  //start rtaudio driver
  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_NONINTERLEAVED;
  try {
    dac.openStream( &oParams,
                    NULL,
                    FORMAT,
                    Signal::frequency,
                    &bufferFrames,
                    &output,
                    (void *)&data,
                    &options );
    dac.startStream();
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage()
              << '\n' << std::endl;
    goto cleanup;
  }

  //while the end of the file is not reached
  while(window.isOpen()) {
    MSG message;
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    sf::Event event;
    while(window.pollEvent(event)) {
      switch(event.type) {
        case sf::Event::Closed:
          window.close();
          data.right_out.reset();
          data.left_out.reset();
          try {
            dac.stopStream();
          } catch (RtAudioError& e) {
            std::cout << '\n' << e.getMessage()
                      << '\n' <<std::endl;
          }
          break;
        default:
          window.useEvent(event);
          break;
      }
    }

    if (data.forwardState || data.myDecoder.ended()) {
      forward.forceOff();
      current_music_file++;
      if (current_music_file >= music_files.size()) {
        current_music_file=0;
      }
      if (music_files.size()) {
        data.myDecoder.open(music_files[current_music_file]);
      }
    }
    if (data.push) {
      //reset of the stereo sound
      data.right_out.reset();
      data.left_out.reset();
      //read the next sample to be played
      if (data.playState) {
        data.myDecoder.fetch(data.left_out,
                             data.right_out);
        copy = data.left_out;
        copy.glslize();
        mySampler.update(copy.samples, Signal::size, 0);
      std::cout << "samples fetched\n" << std::endl;
      data.push=0;
      }
    }

    if (dac.isStreamRunning() == true) {
      std::cout << "stream is running\n" << std::endl;
    }
    window.clear();
    window.setView(sf::View(sf::FloatRect(-0.5, -0.5, 1, 1)));
    window.draw(myShader);
    window.drawContent();
    window.display();
    sf::sleep(sf::milliseconds(1));
  }
  cleanup:
  dac.closeStream();
  return 0;
}


