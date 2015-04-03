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
  //reset of the stereo sound
  oData->right_out.reset();
  oData->left_out.reset();
  //read the next sample to be played
  if (!oData->playState) {
  oData->myDecoder.fetch(oData->left_out,
                         oData->right_out);
  }

  memcpy((MY_TYPE*)(outputBuffer),
         oData->left_out.samples,
         nBufferFrames * sizeof( MY_TYPE));
  memcpy((MY_TYPE*)(outputBuffer)+nBufferFrames,
         oData->right_out.samples,
         nBufferFrames * sizeof( MY_TYPE));

  return oData->myDecoder.ended();
}

int main(int argc, char* argv[])
{
  unsigned int bufferFrames=0;
  //this line is important to be right on the start of the program
  //it configure the size of a signal
  Signal::globalConfigurationFromPow2(48000, 512);

  string_t music_file;
  if (argc >1) {
    //retrieve the name of the file to be played
    music_file = argv[1];
  }

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
  data.myDecoder.open(music_file);

  oParams.deviceId = dac.getDefaultOutputDevice();

  std::cout << "\nPlaying mp3 file " << music_file.toAnsiString()
            << " (buffer frames = " << bufferFrames
            << ")." << std::endl;

  //NEWindow window(sf::VideoMode(800, 400), "Millenium Player", sf::Font());
  sf::RenderWindow window(sf::VideoMode(800, 400),
                    "Millenium Player");

  Interface player(sf::Vector2u(400, 400),
                   sf::Vector2f(400, 400));
  Button play(sf::Vector2f(300, 300), "play", sf::Font());
  sf::Texture tx_play;
  if(!tx_play.loadFromFile("PlayPause.png")) {
    std::cerr << "couldn't load play button texture"
              << std::endl;
  }
  tx_play.setSmooth(true);
  sf::IntRect idle_area(0, 0, 400, 400);
  sf::IntRect clicked_area(400, 0, 400, 400);
  play.setTexture(tx_play, idle_area, clicked_area);
  play.setPosition(250, 50);
  play.setOutlineThickness(0);
  player.addMouseCatcher(&play);
  bool mousePresse=false, mem=false;
  sf::Vector2i mousePos;
  play.linkTo(&(data.playState));

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
    mousePresse=false;
    sf::Event event;
    while(window.pollEvent(event)) {
      switch(event.type) {
        case sf::Event::Closed:
          try {
            dac.stopStream();
          } catch (RtAudioError& e) {
            std::cout << '\n' << e.getMessage()
                      << '\n' <<std::endl;
            goto cleanup;
          }
          window.close();
          break;
        case sf::Event::MouseButtonReleased:
          mousePos = sf::Mouse::getPosition(window);
          play.onMouseRelease(mousePos.x, mousePos.y);
          break;
      }
    }

    if (dac.isStreamRunning() == false) window.close();
    window.clear();
    window.draw(player);
    window.display();
    sf::sleep(sf::milliseconds(100));
  }
  cleanup:
  dac.closeStream();
  return 0;
}

