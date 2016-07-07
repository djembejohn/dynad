// This file is part of Dynad.

//    Dynad is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    Dynad is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with Dynad.  If not, see <http://www.gnu.org/licenses/>.

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
