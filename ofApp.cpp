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

#include "ofApp.h"
#include "RtMidi.h"
#include "bcr2000Driver.h"
#include <boost/bind.hpp>

BCR2000Driver bcr2000Driver;

// There has to be a more elegant way of doing this using boost::bind.
// However, the main issue is if the class has been destroyed or not.
void midiCallback( double deltatime, std::vector< unsigned char > *message, void *classptr )
{
  unsigned int nBytes = message->size();
  int byte0 = 0;
  int byte1 = 0;
  int byte2 = 0;

  for ( unsigned int i=0; i<nBytes; i++ ) {
    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
    if (i == 0)
      byte0 = (int)message->at(i);
    if (i == 1)
      byte1 = (int)message->at(i);
    if (i == 2)
      byte2 = (int)message->at(i);
  }
 
  if ( nBytes > 0 )
    std::cout << "stamp = " << deltatime << std::endl;

  ofApp * myclass = (ofApp *)classptr;

  //  if (byte0 == 176 && byte1 == 101)
  //    byte1 = 102;

  if (byte0 == 144) {
    myclass->synth.noteOn (byte1,byte2);
  }
  else if (byte0 == 128) {
    myclass->synth.noteOff (byte1,byte2);
  }
  else {
    ControlVariable cvar(byte0,byte1);
    bcr2000Driver.interpretControlMessage(cvar,byte2,myclass->synth);
  }
}

int getNote (int key) {
  int note = -1;
  switch (key) {
  case 'a':
    note = 0;
    break;
  case 'w':
    note = 1;
    break;
  case 's':
    note = 2;
    break;
  case 'e':
    note = 3;
    break;
  case 'd':
    note = 4;
    break;
  case 'f':
    note = 5;
    break;
  case 't':
    note = 6;
    break;
  case 'g':
    note = 7;
    break;
  case 'y':
    note = 8;
    break;
  case 'h':
    note = 9;
    break;
  case 'u':
    note = 10;
    break;
  case 'j':
    note = 11;
    break;
  case 'k':
    note = 12;
    break;
  case 'o':
    note = 13;
    break;
  case 'l':
    note = 14;
    break;
  case 'p':
    note = 15;
    break;
  case ';':
    note = 16;
    break;
  case '\'':
    note = 17;
    break;
  case ']':
    note = 18;
    break;
  case '#':
    note = 19;
    break;
  default:
    note = -1;
    break;
  }
  return note;
}


bool chooseMidiPort( shared_ptr<RtMidiIn> rtmidi_in, shared_ptr<RtMidiOut> rtmidi_out )
{
  //  std::cout << "\nWould you like to open a virtual input port? [y/N] ";

  //  std::string keyHit;
  //  std::getline( std::cin, keyHit );
  //  if ( keyHit == "y" ) {
  //    rtmidi->openVirtualPort();
  //    return true;
  //  }

  std::string portName;
  unsigned int nPorts = rtmidi_in->getPortCount();
  if ( nPorts == 0 ) {
    std::cout << "No input ports available!" << std::endl;
    return false;
  }

  int bcrport = -1;

  for (unsigned int i=0; i<nPorts; i++ ) {
    portName = rtmidi_in->getPortName(i);
    //    if (portName=="BCR2000 20:0")
    //      bcrport = i;
    if (portName=="VMPK Output 130:0")
      bcrport = i;
    std::cout << "  Input port #" << i << ": " << portName << '\n';
  }

  if (bcrport == -1) {
    std::cout << "VMPK not found!" << endl;
    return false;
  }

  rtmidi_in->openPort( bcrport );
  rtmidi_out->openPort( bcrport );
  

  return true;
}


//--------------------------------------------------------------
void ofApp::setup(){
  
  midiIn = shared_ptr<RtMidiIn> (new RtMidiIn());
  midiOut = shared_ptr<RtMidiOut> (new RtMidiOut());
  // Call function to select port.
  chooseMidiPort( midiIn, midiOut );
  
  //	midiin->setCallback( &mycallback );
  midiIn->setCallback( midiCallback,(void *) this );
  // Don't ignore sysex, timing, or active sensing messages.
  midiIn->ignoreTypes( false, false, false );
  
  bcr2000Driver.setMidiOut (midiOut);
  bcr2000Driver.initialiseBCR();

  ofBackground(34, 34, 34);
  
  int bufferSize		= 1024;
  sampleRate 			= 48000;
  phase 				= 0;
  phaseAdder 			= 0.0f;
  phaseAdderTarget 	= 0.0f;
  volume				= 0.01f;
  bNoise 				= false;
  
  lAudio.assign(bufferSize, 0.0);
  rAudio.assign(bufferSize, 0.0);
	
  soundStream.printDeviceList();
  
  ofSoundStreamSettings settings;



	// if you want to set the device id to be different than the default:
	//
	//	auto devices = soundStream.getDeviceList();
	//	settings.setOutDevice(devices[3]);

	// you can also get devices for an specific api:
	//
	//	auto devices = soundStream.getDeviceList(ofSoundDevice::Api::PULSE);
	//	settings.setOutDevice(devices[0]);

	// or get the default device for an specific api:
	//
	// settings.api = ofSoundDevice::Api::PULSE;

	// or by name:
	//
	//	auto devices = soundStream.getMatchingDevices("default");
	//	if(!devices.empty()){
	//		settings.setOutDevice(devices[0]);
	//	}

#ifdef TARGET_LINUX
	// Latest linux versions default to the HDMI output
	// this usually fixes that. Also check the list of available
	// devices if sound doesn't work
	auto devices = soundStream.getMatchingDevices("default");
	if(!devices.empty()){
		settings.setOutDevice(devices[0]);
	}
#endif

	settings.setOutListener(this);
	settings.sampleRate = sampleRate;
	settings.numOutputChannels = 1;
	settings.numInputChannels = 0;
	settings.bufferSize = bufferSize;
	soundStream.setup(settings);

	// on OSX: if you want to use ofSoundPlayer together with ofSoundStream you need to synchronize buffersizes.
	// use ofFmodSetBuffersize(bufferSize) to set the buffersize in fmodx prior to loading a file.
}


//--------------------------------------------------------------
void ofApp::update(){

}

#if 0
//----Old function----------------------------------------------
void ofApp::draw(){

  ofSetColor(225);
  ofDrawBitmapString("AUDIO OUTPUT EXAMPLE", 32, 32);
  ofDrawBitmapString("press 's' to unpause the audio\npress 'e' to pause the audio", 31, 92);
  
  ofNoFill();
  
  // draw the left channel:
  ofPushStyle();
  ofPushMatrix();
  ofTranslate(32, 150, 0);
  
  ofSetColor(225);
  ofDrawBitmapString("Left Channel", 4, 18);
  
  ofSetLineWidth(1);	
  ofDrawRectangle(0, 0, 900, 200);
  
  ofSetColor(245, 58, 135);
  ofSetLineWidth(3);
  
  ofBeginShape();
  for (unsigned int i = 0; i < lAudio.size(); i++){
    float x =  ofMap(i, 0, lAudio.size(), 0, 900, true);
    ofVertex(x, 100 -lAudio[i]*180.0f);
  }
  ofEndShape(false);
  
  ofPopMatrix();
  ofPopStyle();
  
  // draw the right channel:
  ofPushStyle();
  ofPushMatrix();
  ofTranslate(32, 350, 0);
  
  ofSetColor(225);
  ofDrawBitmapString("Right Channel", 4, 18);
  
  ofSetLineWidth(1);	
  ofDrawRectangle(0, 0, 900, 200);
  
  ofSetColor(245, 58, 135);
  ofSetLineWidth(3);
  
  ofBeginShape();
  for (unsigned int i = 0; i < rAudio.size(); i++){
    float x =  ofMap(i, 0, rAudio.size(), 0, 900, true);
    ofVertex(x, 100 -rAudio[i]*180.0f);
  }
  ofEndShape(false);
  
  ofPopMatrix();
  ofPopStyle();
  
		
  ofSetColor(225);
  string reportString = "volume: ("+ofToString(volume, 2)+") modify with -/+ keys\npan: ("+ofToString(pan, 2)+") modify with mouse x\nsynthesis: ";
  if( !bNoise ){
    reportString += "sine wave (" + ofToString(targetFrequency, 2) + "hz) modify with mouse y";
  }else{
    reportString += "noise";	
  }
  ofDrawBitmapString(reportString, 32, 579);
  
}
#else

void ofApp::draw(){

  ofSetColor(225);
  ofDrawBitmapString("John's Synthesiser", 32, 32);
}
#endif

//--------------------------------------------------------------
void ofApp::keyPressed  (int key)
{
  // plug this into master volume!
  if (key == '-' || key == '_' ){
    volume -= 0.05;
    volume = MAX(volume, 0);
  } else if (key == '+' || key == '=' ){
    volume += 0.05;
    volume = MIN(volume, 1);
  }
  


  int note = getNote(key);
  if (note != -1) {
    synth.noteOn (note+octave*12,1.0);
  }
  
  if( key == '.' ){
    octave +=1;
    if (octave > 9)
      octave = 9;
      
  }
  
  if( key == ',' ){
    octave -=1;
    if (octave <0)
      octave = 0;
  }
  
}
 
//--------------------------------------------------------------
void ofApp::keyReleased  (int key){
  int note = getNote(key);
  if (note != -1) {
    //    char str[2];
    //    str[0] = (char) key;
    //    str[1] = 0;
    //    cout << "Key off " << str << endl;
    synth.noteOff (note+octave*12,1.0);
  }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
#if 1
  int width = ofGetWidth();
  pan = (float)x / (float)width;
  float height = (float)ofGetHeight();
  float heightPct = ((height-y) / height);
  targetFrequency = 2000.0f * heightPct;
  phaseAdderTarget = (targetFrequency / (float) sampleRate) * TWO_PI;
#endif
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
#if 0
  int width = ofGetWidth();
  pan = (float)x / (float)width;
#endif
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
#if 0
  bNoise = true;
#endif
}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
#if 0
  bNoise = false;
#endif
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
  
}
 
//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
  
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

#if 0
//----------Old function----------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer){
  //pan = 0.5f;
  float leftScale = 1 - pan;
  float rightScale = pan;
  
  // sin (n) seems to have trouble when n is very large, so we
	// keep phase in the range of 0-TWO_PI like this:
  while (phase > TWO_PI){
    phase -= TWO_PI;
  }
  
  if ( bNoise == true){
    // ---------------------- noise --------------
    for (int i = 0; i < buffer.getNumFrames(); i++){
      lAudio[i] = buffer[i*buffer.getNumChannels()    ] = ofRandom(0, 1) * volume * leftScale;
      //      rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = ofRandom(0, 1) * volume * rightScale;
    }
  } else {
    phaseAdder = 0.95f * phaseAdder + 0.05f * phaseAdderTarget;
    for (int i = 0; i < buffer.getNumFrames(); i++){
      phase += phaseAdder;
      float sample = sin(phase);
      lAudio[i] = buffer[i*buffer.getNumChannels()    ] = sample * volume * leftScale;
      //      rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = sample * volume * rightScale;
    }
  }
  
}
#else
void ofApp::audioOut(ofSoundBuffer & buffer){
  synth.playToBuffer(buffer,lAudio);
}
#endif

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

