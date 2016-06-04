#pragma once

#include "ofMain.h"
#include "RtMidi.h"
#include "generateTone.h"


class ofApp : public ofBaseApp{

 public:
  
  ofApp()
    : ofBaseApp(), octave(5)
    {}
  
  ~ofApp() {

  }


  void setup();
  void update();
  void draw();
  
  void keyPressed  (int key);
  void keyReleased(int key);
  void mouseMoved(int x, int y );
  void mouseDragged(int x, int y, int button);
  void mousePressed(int x, int y, int button);
  void mouseReleased(int x, int y, int button);
  void mouseEntered(int x, int y);
  void mouseExited(int x, int y);
  void windowResized(int w, int h);
  void dragEvent(ofDragInfo dragInfo);
  void gotMessage(ofMessage msg);
  
  void audioOut(ofSoundBuffer & buffer);
  //		void midiCallback(double deltatime, std::vector< unsigned char > *message, void */*userData*/ );
  
  PolySynthesiser synth;
  
  shared_ptr<RtMidiIn> midiIn;
  shared_ptr<RtMidiOut> midiOut;
  ofSoundStream soundStream;
  
  float 	pan;
  int		sampleRate;
  bool 	bNoise;
  float 	volume;

  int octave;

  vector <float> lAudio;
  vector <float> rAudio;
  
  //------------------- for the simple sine wave synthesis
  float 	targetFrequency;
  float 	phase;
  float 	phaseAdder;
  float 	phaseAdderTarget;
};
