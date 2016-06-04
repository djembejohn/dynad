#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <math.h>

#include "RtMidi.h"
#include "bcr2000Driver.h"
#include "synthesiser.h"

#include <RtAudio.h>
#include <RtError.h>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

BCR2000Driver bcr2000Driver;

int rtaudio_callback(
		     void			*outbuf,
		     void			*inbuf,
		     unsigned int		nFrames,
		     double			streamtime,
		     RtAudioStreamStatus	status,
		     void * data
		     );

void midiCallback( double deltatime, std::vector< unsigned char > *message, void *classptr );


class MainApplication: public boost::mutex
{
public:

  PolySynthesiser synth;
  
  shared_ptr<RtMidiIn> midiIn;
  shared_ptr<RtMidiOut> midiOut;

  double sampleRate;
  unsigned int numChannels;

  shared_ptr<RtAudio> audioOut;
  unsigned int bufsize = 4096;

  bool finished;
  bool pause;

  MainApplication () 
    : sampleRate (48000), numChannels (2), finished(false),pause(false) {
  }
  
  void init() {
    cout << "Initialising sound output" << endl;
    if (setupAudio()) 
      throw "Error";
    cout << "Initialising Midi input" << endl;
    if (setupMidi())
      throw "Error";
  }
  void run() {
    
    if (!audioOut) 
      throw "No audio output";

    audioOut->startStream();
    while (!finished) {
      #if 1
      cout << ">> ";
      cout.flush();
      string input;
      getline(cin,input);
      if (input[0] == 'q')
	pause = pause? false:true;

      if (input[0] == 'q')
	finished = true;
      if (input[0] == 's') {
	if (input.size() > 3) {
	  synth.save (input.substr(2));
	}
	else 
	  cout << "Enter a filename!" << endl;
      }
      if (input[0] == 'l') {
	if (input.size() > 3) {
	  synth.load (input.substr(2));
	}
	else 
	  cout << "Enter a filename!" << endl;
      }
#else
      sleep(1);
#endif

    }
    audioOut->stopStream();
    audioOut->closeStream();  
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
    int vmpport = -1;
    
    for (unsigned int i=0; i<nPorts; i++ ) {
      portName = rtmidi_in->getPortName(i);
      if (portName=="BCR2000 20:0")
	bcrport = i;
      if (portName.substr(0,11)=="VMPK Output")
	vmpport = i;
      std::cout << "  Input port #" << i << ": " << portName << '\n';
    }
    
    if (bcrport == -1) {
      std::cout << "BCR2000 not found!" << endl;
    }
    else
      rtmidi_out->openPort( bcrport );
    
    rtmidi_in->openPort( vmpport );
    
    
    return true;
  }
  
  
  int setupMidi () {
    midiIn = shared_ptr<RtMidiIn> (new RtMidiIn());
    midiOut = shared_ptr<RtMidiOut> (new RtMidiOut());
    // Call function to select port.
    if (!chooseMidiPort( midiIn, midiOut )) {
      cerr << "Failed to set up midi" <<endl;
      return 1;
  }
    
    if (!midiOut || !midiIn) {
      cerr << "Failed to set up midi" <<endl;
      return 1;
    }
    //	midiin->setCallback( &mycallback );
    midiIn->setCallback( midiCallback,(void *) this );
    // Don't ignore sysex, timing, or active sensing messages.
    midiIn->ignoreTypes( false, false, false );
    
    bcr2000Driver.setMidiOut (midiOut);
    bcr2000Driver.initialiseBCR();

    return 0;
  }

  int setupAudio () {

    try {
      audioOut = shared_ptr<RtAudio> (new RtAudio(RtAudio::LINUX_ALSA));
    }catch  (RtError e){
      fprintf(stderr, "fail to create RtAudio: %s\n", e.what());
      return 1;
    }
    if (!audioOut){
      fprintf(stderr, "fail to allocate RtAudio\n");
      return 1;
    }
    /* probe audio devices */
    unsigned int devId = audioOut->getDefaultOutputDevice();

    RtAudio::StreamParameters *outParam = new RtAudio::StreamParameters();

    outParam->deviceId = devId;
    outParam->nChannels = numChannels;

    //    boost::function<int (void*,void*,unsigned int, double, unsigned int, void *)> fun =
    //      boost::bind (&MainApplication::rtaudio_callback, this, _1,_2,_3,_4,_5,_6);
    
    sampleRate = 48000;

    audioOut->openStream(outParam, NULL, RTAUDIO_FLOAT32, 48000,
			 &bufsize, 
			 rtaudio_callback, 
			 this);
    
    return 0;

  }
  

  

};


int rtaudio_callback(
		     void			*outbuf,
		     void			*inbuf,
		     unsigned int		nFrames,
		     double			streamtime,
		     RtAudioStreamStatus	status,
		     void * data
		     )
{
  (void)inbuf;
  
  //  cout << "audio " << streamtime << endl;

  MainApplication * mainapp = (MainApplication *)data;
  
  

  RBufferWithMutex buffer (new BufferWithMutex
			   (
			    streamtime,
			    mainapp->sampleRate,
			    nFrames,
			    mainapp->numChannels,
			    (float *) outbuf
			    )
			   );
  //  cout << buffer->numberOfChannels << " " 
  //       << buffer->numberOfFrames << " " 
  //       << outbuf << endl;

  for (unsigned int i = 0; i<nFrames*mainapp->numChannels; i++) {
    buffer->at(i) = 0;
  }

  //  if (streamtime >2) {
  //    cout << "Finished" << endl;
  //    mainapp->finished = true;
  //  }
  if (!mainapp->pause)
    mainapp->synth.playToBuffer(buffer);
  
  //  cout << "buffer done" << endl;

  return 0;
}

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

  MainApplication * myclass = (MainApplication *)classptr;

  //  if (byte0 == 176 && byte1 == 101)
  //    byte1 = 102;

  if (byte0 == 144) {
    if (byte2 == 0)
      myclass->synth.noteOff (byte1,byte2);
    else 
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


int
main(void)
{

  MainApplication app;
  app.init();
  app.run();


  return 0;
}





