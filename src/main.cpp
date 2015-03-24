#include <iostream>
#include <SFML/Graphics.hpp>
#include <libtools.h>

sf::Mutex m_sound;
bool end_sound;

int sound(string_t music_file) {

  //load bass dll
  if (load_bass_procs("bass.dll") == -1) {
    std::cerr << "Couldn't load the library bass.dll" << std::endl;
    return 0xDEAD;
  }
  //initialize bass
  if (!BASS_EnsureBassInit(0, Signal::frequency, 0)) {
    std::cerr << "Couldn't initialize bass" << std::endl;
    return 0xDEAD;
  }
  //decoder to read the music file
  BassDecoder myDecoder;
  myDecoder.open(music_file);

  //windows driver to play the music file
  WinmmDriver myDriver(Signal::frequency);
  //time
  sf::Clock t;
  //stereo sound
  Signal right_out;
  Signal left_out;
  bool sendSignalSuccess=false;
  int length_send=0;
  //sleep_time is the length of time to play a signal fully
  //period in milisecond * by the size of a signal
  int sleep_time=(float)(1.0/Signal::frequency)*1000000.0*Signal::size;
  //retrieve the title and the artist of the music file
  std::cout << "Title : " << string_t_to_std(myDecoder.name()) << std::endl;
  std::cout << "Artist : " << string_t_to_std(myDecoder.author()) << std::endl;
  //little fun with filter
  AnalogBandPassFilter2 myLowPassLeft(100);
  AnalogBandPassFilter2 myLowPassRight(100);
  while(!end_sound) {
    //reset of the stereo sound
    right_out.reset();
    left_out.reset();
    //read the next sample to be played
    myDecoder.fetch(left_out, right_out);
    //apply the filter
    myLowPassLeft.setFrequency(500+400*cos(t.getElapsedTime().asSeconds()*10));
    myLowPassRight.setFrequency(500+400*cos(t.getElapsedTime().asSeconds()*10));
    myLowPassLeft.step(&left_out);
    myLowPassRight.step(&right_out);
    std::cout << "frequency : " << 500+400*cos(t.getElapsedTime().asSeconds()*10) << std::endl;
    //send the audio
    do {
      length_send = myDriver.getBufferedSamplesCount();
      if (length_send > Signal::size*2) {
        //wait if the buffer is still full
        sf::sleep(sf::microseconds(sleep_time));
      }
      sendSignalSuccess = myDriver.pushStereoSignal(left_out, right_out);
    }while(!sendSignalSuccess);
  }
}

void calculus() {

}

int main(int argc, char* argv[])
{
  //this line is important to be right on the start of the program
  //it configure the size of a signal
  Signal::globalConfigurationFromPow2(44100, 1024);

  string_t music_file;
  if (argc >1) {
    //retrieve the name of the file to be played
    music_file = argv[1];
  }
  end_sound=false;
  sf::Thread th_sound(&sound, music_file);
  th_sound.launch();

  //NEWindow window(sf::VideoMode(800, 400), "Millenium Player", sf::Font());
  sf::Window window(sf::VideoMode(800, 400), "Millenium Player");

  //while the end of the file is not reached
  while(window.isOpen()) {

    sf::Event event;
    while(window.pollEvent(event)) {
      switch(event.type) {
        case sf::Event::Closed:
          m_sound.lock();
          end_sound=true;
          m_sound.unlock();
          th_sound.wait();
          window.close();
          break;
      }
    }
    sf::sleep(sf::milliseconds(1));
  }
  th_sound.wait();
  return 0;
}

