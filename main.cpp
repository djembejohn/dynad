// Copyright 2016 John Bryden

// This file is part of Dynad.

//    Dynad is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    Dynad is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.

//    You should have received a copy of the GNU Affero General Public License
//    along with Dynad.  If not, see <http://www.gnu.org/licenses/>.

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <math.h>

#include "RtMidi.h"
#include "bcr2000Driver.h"
#include "synthesiser.h"

#include "Stk.h"
#include "RtAudio.h"
#include "RtError.h"

#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "lo/lo.h"


ofstream mainlog ("main.log");



BCR2000Driver bcr2000Driver;

using namespace stk;

// handlers for osc, rtaudio and midi

void osc_error(int num, const char *m, const char *path);
int generic_osc_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);


int rtaudio_callback(
		     void			*outbuf,
		     void			*inbuf,
		     unsigned int		nFrames,
		     double			streamtime,
		     RtAudioStreamStatus	status,
		     void * data
		     );


void midiCallback( double deltatime, std::vector< unsigned char > *message, void *classptr );
void midiCallbackBCR( double deltatime, std::vector< unsigned char > *message, void *classptr );


class MidiConnection
{
public:
  class MainApplication * mainApp_ptr;
  int channelNumber;
  ControlInputMap inputMap;
  shared_ptr<RtMidiIn> midiIn;
  shared_ptr<RtMidiOut> midiOut;

  string midiPortName;

  MidiConnection (MainApplication * _mainApp, int _channelNumber)
    : mainApp_ptr(_mainApp), channelNumber(_channelNumber),
      midiIn (shared_ptr<RtMidiIn> (new RtMidiIn())),
      midiOut (shared_ptr<RtMidiOut> (new RtMidiOut()))
  {}

  bool chooseMidiPort () {
    std::string portName;
    unsigned int nPorts = midiIn->getPortCount();
    if ( nPorts == 0 ) {
      std::cout << "No input ports available!" << std::endl;
      return false;
    }
    
    int port = -1;
    int noport = -1;
    vector<string> portNames;

    unsigned int i;

    for (i=0; i<nPorts; i++ ) {
      portName = midiIn->getPortName(i);
      portNames.push_back(portName);
      if (portName.substr(0,10)=="USB Oxygen")
	port = i;
      std::cout << "  Input port #" << i << ": " << portName << '\n';
    }

    portNames.push_back("No binding");
    noport = i;
    std::cout << "  Input port #" << i << ": " << "No binding" << '\n';
    
    cout << "Choose midi port";
    if (port >-1) 
      cout << " (default " << port << ")";
    cout << endl << ": ";
    string input;
    getline(cin,input);

    int inputPort = port;
    if (input.size()>0)
      stringstream(input) >> inputPort;
    
    if (inputPort >-1 && inputPort < noport) {
      midiPortName=portNames[inputPort];
      midiIn->openPort( inputPort );
      midiOut->openPort( inputPort );
    }
    else {
      string myNameIn="Dynad input channel:"+to_string(channelNumber);
      string myNameOut="Dynad output channel:"+to_string(channelNumber);
      midiPortName=portNames[noport];
      midiIn->openVirtualPort(myNameIn);
      midiOut->openVirtualPort(myNameOut);
      cerr << "Virtual port opened" << endl;
    }
    
    return true;
    
  }
};


class MainApplication: public boost::mutex
{
public:

  map<int, shared_ptr<ofstream> > synthRecorders;
  map<int,RPolySynthesiser> synths;
  RPolySynthesiser focalSynth;
  
  vector< MidiConnection * > midiConnections;
  shared_ptr<RtMidiIn> bcrIn;
  shared_ptr<RtMidiOut> bcrOut;


  double sampleRate;
  unsigned int numChannels;

  shared_ptr<RtAudio> audioOut;
  unsigned int bufsize = 512;

  double timePoint;

  bool finished;
  bool pause;

  bool performanceMode;

  MainApplication () 
    : sampleRate (48000), numChannels (2), 
      timePoint (0.0),
      finished(false),performanceMode(false) {
    synths[0] = RPolySynthesiser(new PolySynthesiser(16,6));
    focalSynth = synths[0];
  }

  ~MainApplication ()
  {
    for (auto mcon:midiConnections) {
      delete mcon;
    }
  }
  
  void init() {
    cout << "Initialising sound output" << endl;
    if (setupAudio()) 
      throw "Error";
    cout << "Initialising Midi input" << endl;
    if (setupMidi())
      throw "Error";
    midiConnections[0]->inputMap.load("default.map");
    cout << "Initialising OSC input" << endl;

    lo_server_thread st = lo_server_thread_new("7770", osc_error);

    /* add method that will match any path and args */
    lo_server_thread_add_method(st, NULL, NULL, generic_osc_handler, this);

    lo_server_thread_start(st);

  }
  void run() {
    
    if (!audioOut) 
      throw "No audio output";

    audioOut->startStream();
    synths[0]->load ("default.prm");
    while (!finished) {
      #if 1
      cout << ">> ";
      cout.flush();
      string input;
      getline(cin,input);
      if (input[0] == 'p') {
	performanceMode = performanceMode? false:true;
	bcr2000Driver.active = performanceMode? false:true;
	if (performanceMode) 
	  cout << "Performance mode, BCR now standard controller" << endl;
	else
	  cout << "Programming mode, enhanced BCR control" << endl;
      }

      if (input[0] == 'n') {
	// create a new synth
	int channel=-1, numPhones=-1, numPartials = -1;
	stringstream sstr(input) ;
	string junk;
	sstr >> junk >> channel >> numPartials >> numPhones;
	cout << "New synth on channel: " << channel << endl
	     << "Partials = " << numPartials << endl
	     << "Phones = " << numPhones << endl;
	if (channel > 0 && numPartials>0 && numPhones>0) {
	  channel -=1;
	  synths[channel] = RPolySynthesiser(new PolySynthesiser (numPartials, numPhones));
	  focalSynth = synths[channel];
	}
	else
	  cerr << "Usage:" << endl << "n <midi channel> <num partials> <num phones>" << endl;
      }

      if (input[0] == 'i') {
	stringstream sstr(input) ;
	string junk;
	int channel = -1;
	string mapname;
	sstr >> junk >> channel >> mapname;
	if (mapname == "")
	  mapname = "default.map";
	if (channel > 0) 
	  channel -= 1;

	MidiConnection *mIn = new MidiConnection (this,channel);
	mIn->chooseMidiPort();
	int mconNumber = midiConnections.size();
	midiConnections.push_back(mIn);
	mIn->midiIn->setCallback( midiCallback,(void *) mIn);
	midiConnections[mconNumber]->inputMap.load(mapname);

	cout << "New connection in slot " << mconNumber << endl;
	cout << "Current connections" << endl
	     << "Slot channel device" << endl;

	for (unsigned int i = 0; i < midiConnections.size(); i++) {
	  cout << i << " " 
	       << midiConnections[i]->channelNumber+1 << " " 
	       << midiConnections[i]->midiPortName << endl;
	}

      }

      if (input[0] == 'q')
	finished = true;

      if (input[0] == 'o') {
	focalSynth->controlSet->outputControllersToConsole();
      }

      if (input[0] == 'r') {
	string junk;
	string fname;
	stringstream(input) >> junk >> fname;
	if (fname.size() >0) {
	  cout << "Recording to files starting with " << fname << endl;
	  for (auto & entry:synths) {
	    string sname = fname+"_"+to_string(entry.first)+".raw";
	    synthRecorders.emplace (make_pair(entry.first,shared_ptr<ofstream> (new ofstream (sname,ofstream::binary))));
	  }
	  cout << "Convert with avconv -f f64le -ar 48000 -ac 2 -i <fname>.raw <fname>.wav" << endl;
	}
	else {
	  cout << "Ceasing recording" << endl;
	  synthRecorders.clear();
	}
      }

      if (input[0] == 's') {
	if (input.size() > 3) {
	  focalSynth->save (input.substr(2));
	}
	else 
	  cout << "Enter a filename!" << endl;
      }
      if (input[0] == 'l') {
	if (input.size() > 3) {
	  focalSynth->load (input.substr(2));
	}
	else 
	  cout << "Enter a filename!" << endl;
      }
      if (input[0] == 'm') {
	string junk,fn1,fn2;
	stringstream (input) >> junk >> fn1 >> fn2;
	if (fn1.size() > 0 && fn2.size() > 0) {
	  focalSynth->loadMorph (fn1,fn2);
	}
	else 
	  cout << "Enter filenames!" << endl;
      }
      if (input[0] >='0' && input[0]<='9') {
	int numChannel = -1;
	stringstream(input) >> numChannel;
	numChannel-= 1;
	if (synths[numChannel]) {
	  focalSynth = synths[numChannel];
	  focalSynth->updateController();
	}
      }

#else
      sleep(1);
#endif

    }
    audioOut->stopStream();
    audioOut->closeStream();  
  }

  bool connectBCRPort( )
  {
    std::string portName;
    unsigned int nPorts = bcrIn->getPortCount();
    if ( nPorts == 0 ) {
      std::cout << "No input ports available!" << std::endl;
      return false;
    }
    
    int bcrport = -1;
    
    for (unsigned int i=0; i<nPorts; i++ ) {
      portName = bcrIn->getPortName(i);
      if (portName.substr(0,7)=="BCR2000" && bcrport == -1)
	bcrport = i;
    }

    if (bcrport == -1) {
      std::cout << "BCR2000 not found, just not going to talk to it then. \nSee if I care." << endl;
    }
    else {
      bcrOut->openPort( bcrport );
      bcrIn->openPort(bcrport);
    }
    
    return true;
  }
  
  
  int setupMidi () {
    MidiConnection *mIn = new MidiConnection (this,-1);

    bcrOut = shared_ptr<RtMidiOut> (new RtMidiOut());
    bcrIn = shared_ptr<RtMidiIn> (new RtMidiIn());
    // Call function to select port.
    if (!connectBCRPort()) {
      cerr << "Failed to set up midi" <<endl;
      return 1;
    }
    mIn->chooseMidiPort();
    
    if (!bcrOut || !bcrIn) {
      cerr << "Failed to set up midi" <<endl;
      return 1;
    }
    //	midiin->setCallback( &mycallback );
    
    midiConnections.push_back(mIn);

    mIn->midiIn->setCallback( midiCallback,(void *)mIn);
    // Ignore sysex, timing, or active sensing messages.
    // mIn.midiIn->ignoreTypes( false, false, false );

    bcrIn->setCallback( midiCallbackBCR,(void *) this );
    
    bcr2000Driver.setMidiOut (bcrOut);
    bcr2000Driver.initialiseBCR();

    return 0;
  }

  int setupAudio () {

    Stk::setSampleRate (sampleRate);
    try {
      //audioOut = shared_ptr<RtAudio> (new RtAudio(RtAudio::LINUX_ALSA));
      audioOut = shared_ptr<RtAudio> (new RtAudio());
    }catch  (RtError e){
      fprintf(stderr, "fail to create RtAudio: %s\n", e.what());
      return 1;
    }
    if (!audioOut){
      fprintf(stderr, "fail to allocate RtAudio\n");
      return 1;
    }
    /* probe audio devices */
    
    unsigned int devices = audioOut->getDeviceCount();
    // Scan through devices for various capabilities
    RtAudio::DeviceInfo info;
    for ( unsigned int i=0; i<devices; i++ ) {
      info = audioOut->getDeviceInfo( i );
      if ( info.probed == true ) {
	// Print, for example, the maximum number of output channels for each device
	std::cout << "device = " << i;
	std::cout << ": maximum output channels = " << info.outputChannels << "\n";
      }
    }
    unsigned int devId = audioOut->getDefaultOutputDevice();

    RtAudio::StreamParameters outParam;
    RtAudioFormat format = ( sizeof(StkFloat) == 8 ) ? RTAUDIO_FLOAT64 : RTAUDIO_FLOAT32;

    outParam.deviceId = devId;
    outParam.nChannels = numChannels;

    //    boost::function<int (void*,void*,unsigned int, double, unsigned int, void *)> fun =
    //      boost::bind (&MainApplication::rtaudio_callback, this, _1,_2,_3,_4,_5,_6);
    
    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_HOG_DEVICE;
    options.flags |= RTAUDIO_SCHEDULE_REALTIME;
    //    options.flags |= RTAUDIO_INTERLEAVED;

    audioOut->openStream(&outParam, NULL, format , sampleRate,
			 &bufsize, 
			 rtaudio_callback, 
			 this,
			 &options
			 );
    
    return 0;

  }
  

  

};


ofstream outlog("out.log");


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
  
  mainlog << "audio " << streamtime << ", " << status << endl;

  MainApplication * mainapp = (MainApplication *)data;
  
  //  StkFloat tempbuffer[nFrames*mainapp->numChannels];
  //  for (unsigned int i = 0; i<nFrames*mainapp->numChannels; i++) {
  //    tempbuffer[i] = 0;
    //  }


  typedef pair<RBufferWithMutex,int> RBuffSynthPair;

  vector<RBuffSynthPair> buffersandsynths;
  boost::thread_group group;

  RPhonesBuffersList phonesBuffers (new PhonesBuffersList);

  for (auto synthmidichannelpair:mainapp->synths) {

    int synthIndex = synthmidichannelpair.first;
    RPolySynthesiser rsynth = synthmidichannelpair.second;
    if (rsynth) {

      RBufferWithMutex buffer (new BufferWithMutex<>
			       (
				mainapp->timePoint,
				mainapp->sampleRate,
				nFrames,
				mainapp->numChannels
				)
			     );
      buffersandsynths.push_back(RBuffSynthPair(buffer,synthIndex));
      
      for (auto phone:rsynth->getPhones()) {
	phonesBuffers->addPair (phone,buffer);
      }
      
      
      //      auto bindoperation = boost::bind (&PolySynthesiser::playToBuffer, rsynth, buffer);
      //      group.create_thread  (bindoperation);
    }
  }

#if 1
  ThreadedPhonePlayer phonePlayer1 (phonesBuffers);
  ThreadedPhonePlayer phonePlayer2 (phonesBuffers);
  ThreadedPhonePlayer phonePlayer3 (phonesBuffers);
  ThreadedPhonePlayer phonePlayer4 (phonesBuffers);
  group.create_thread(boost::bind (&ThreadedPhonePlayer::threadedFunction,&phonePlayer1));
  group.create_thread(boost::bind (&ThreadedPhonePlayer::threadedFunction,&phonePlayer2));
  group.create_thread(boost::bind (&ThreadedPhonePlayer::threadedFunction,&phonePlayer3));
  group.create_thread(boost::bind (&ThreadedPhonePlayer::threadedFunction,&phonePlayer4));
  group.join_all();
#else
  ThreadedPhonePlayer phonePlayer1 (phonesBuffers);
  
  phonePlayer1.threadedFunction();
#endif
    

  mainapp->timePoint += (double)nFrames/mainapp->sampleRate;

  //  cout << buffer->numberOfChannels << " " 
  //       << buffer->numberOfFrames << " " 
  //       << outbuf << endl;

  int numberOfChannels = mainapp->numChannels;

  //  cout << "buffer done" << endl;
  int counter = 0;
  for (auto bufferandsynth: buffersandsynths) {
    RBufferWithMutex buffer = bufferandsynth.first;
    RPolySynthesiser synth = mainapp->synths[bufferandsynth.second];
    int synthIndex = bufferandsynth.second;

    synth->fx.updateFXFromControllers();

    double volume = synth->controlSet->controllerNames["masterVolume"]->getLevel();
    double pan = synth->controlSet->controllerNames["pan"]->getLevel();
    double distortionLevel = synth->controlSet->controllerNames["distortion"]->getLevel();

    safeVector<StkFloat> & synthbuf = buffer->getOutBufRef();
    StkFloat * outsamples = (StkFloat *)outbuf;
    for (unsigned int i=0;i<nFrames*numberOfChannels; i+=numberOfChannels) {
      // ha ha it just takes both channels
      // if I want a single channel version I'll need to fix that
      double lvalue = synthbuf[i];
      double rvalue = synthbuf[i+1];
      
      if (distortionLevel > 0 && false) {
	// horrible hack
	double distortionFactor = 3.0;
	double value = lvalue+rvalue;
	value = value/distortionFactor;
	if (value > 1.0-distortionLevel)
	  value = 1.0-distortionLevel;
	else if (value < -1.0 + distortionLevel)
	  value = -1.0+distortionLevel;	
	value = value*distortionFactor;
	//	cout << distortionLevel << endl;
      }


      double lval = lvalue*pan*2.0;
      double rval = rvalue*(1.0-pan)*2.0;


      double cmix =synth->fx.chorusMix->getLevel(); 
      if (cmix > 0.0) {
	double inval = (rval+lval)/2.0;
	synth->fx.chorus.tick(inval);
	const StkFrames& samples = synth->fx.chorus.lastFrame();
	
	lval = (1-cmix)*lval+cmix*samples[0];
	rval = (1-cmix)*rval+cmix*samples[1];
      }

      

      double emix = synth->fx.echoMix->getLevel();
      if (emix >0.0) {
	double inval = (rval+lval)/2.0;
	inval += synth->fx.echo.lastFrame()[0]*synth->fx.echoFeedback->getLevel();
	synth->fx.echo.tick(inval);
      
	const StkFrames& samples = synth->fx.echo.lastFrame();

	lval = (1-emix)*lval+emix*samples[0];
	rval = (1-emix)*rval+emix*samples[0];
      }

      double rmix = synth->fx.reverbMix->getLevel();
      if (rmix > 0.0) {
	synth->fx.reverb.tick(rval,lval);
	const StkFrames& samples = synth->fx.reverb.lastFrame();
	lval = (1-rmix)*lval+rmix*samples[0];
	rval = (1-rmix)*rval+rmix*samples[1];
      }

      lval = lval * volume;
      rval = rval * volume;

      if (counter == 0) {
	outsamples[i] = lval;
	if (numberOfChannels>1) 
	  outsamples[i+1] = rval;
      }
      else {
	outsamples[i] += lval;
	if (numberOfChannels>1) 
	  outsamples[i+1] += rval;
      }
      
      if (mainapp->synthRecorders.find(synthIndex) != mainapp->synthRecorders.end()) {
	mainapp->synthRecorders[synthIndex]->write (reinterpret_cast<char*>( &lval ), sizeof lval);
	if (numberOfChannels>1)
	  mainapp->synthRecorders[synthIndex]->write (reinterpret_cast<char*>( &rval ), sizeof rval);
      }
      
     

    }
    counter ++;
  }
  
 
  //  outlog << streamtime << endl;

  return 0;
}



void midiCallback( double deltatime, std::vector< unsigned char > *message, void *classptr )
{
  unsigned int nBytes = message->size();
  int byte0 = 0;
  int byte1 = 0;
  int byte2 = 0;

  for ( unsigned int i=0; i<nBytes; i++ ) {
    //    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
    if (i == 0)
      byte0 = (int)message->at(i);
    if (i == 1)
      byte1 = (int)message->at(i);
    if (i == 2)
      byte2 = (int)message->at(i);
  }
 
  //  if ( nBytes > 0 )
  //    std::cout << "stamp = " << deltatime << std::endl;

  MidiConnection * mCon_ptr = (MidiConnection *)classptr;

  MainApplication * myclass = mCon_ptr->mainApp_ptr;

  int channelNumber = mCon_ptr->channelNumber;
  if (channelNumber ==-1) 
    channelNumber = byte0 & 0xf;

  //  cout << "Received on:" << endl
  //       << mCon_ptr->channelNumber << " "
  //       << mCon_ptr->midiPortName << endl;

  if (byte0 > 143 && byte0<160) {
    if (myclass->synths[channelNumber]){
      if (byte2 == 0)
	myclass->synths[channelNumber]->noteOff (byte1,byte2,myclass->timePoint);
      else 
	myclass->synths[channelNumber]->noteOn (byte1,byte2,myclass->timePoint);
    }
  }
  else if (byte0 > 127 && byte0<144) {
    if (myclass->synths[channelNumber])
      myclass->synths[channelNumber]->noteOff (byte1,byte2,myclass->timePoint);
  }
  else {
    if ((byte0 & 0xf0) == 0xb0) {
      int controller = byte1;
      auto range = mCon_ptr->inputMap.inputMap.equal_range(controller);
      cout << "CC: " << channelNumber << " " << controller << " " << byte2;
      for (auto it = range.first; it != range.second; ++it) {
	string & controlName = it->second;
	cout << " -> " << controlName;
	if (myclass->synths[channelNumber])
	  myclass->synths[channelNumber]->interpretNamedControlMessage(controlName,byte2);
	
      }
      cout << endl;
    }
  }

}


void midiCallbackBCR( double deltatime, std::vector< unsigned char > *message, void *classptr )
{
  unsigned int nBytes = message->size();
  int byte0 = 0;
  int byte1 = 0;
  int byte2 = 0;

  for ( unsigned int i=0; i<nBytes; i++ ) {
    //    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
    if (i == 0)
      byte0 = (int)message->at(i);
    if (i == 1)
      byte1 = (int)message->at(i);
    if (i == 2)
      byte2 = (int)message->at(i);
  }
 
  //  if ( nBytes > 0 )
  //    std::cout << "stamp = " << deltatime << std::endl;

  MainApplication * myclass = (MainApplication *)classptr;

  //  if (byte0 == 176 && byte1 == 101)
  //    byte1 = 102;

  if ((byte0 & 0xf0) == 0xb0 && !myclass->performanceMode) {
    int channel = byte0-0xb0;
    int controller = byte1;
    ControlVariable cvar(channel,controller);
    bcr2000Driver.interpretControlMessage(cvar,byte2,myclass->focalSynth);    
  }

}


int generic_osc_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)
{
 #if 0
    int i;
   printf("path: <%s>\n", path);
    for (i = 0; i < argc; i++) {
        printf("arg %d '%c' ", i, types[i]);
        lo_arg_pp((lo_type)types[i], argv[i]);
        printf("\n");
    }
    printf("\n");
    fflush(stdout);
#endif

    MainApplication * myclass = (MainApplication *)user_data;

    // Should be /controllername/partialnumber
    // or /controllername
    // for xy it will be /first:second
   
    string controllerName = path+1;
    size_t colon= controllerName.find(":");
    if (colon != string::npos) {
      string first = controllerName.substr(0,colon);
      string second = controllerName.substr(colon+1);
      bool success;
      success = myclass->focalSynth->interpretOSCControlMessage(first,argv[0]->f);
      success = myclass->focalSynth->interpretOSCControlMessage(second,argv[1]->f) && success;

      if (success) {
	cout << "Received from " << first << ": " << argv[0]->f << ", " << second << ": " << argv[1]->f << endl;
      }

    }
    else if (controllerName == "accxyz") {
      double x = argv[0]->f;
      double y = argv[1]->f;
      double z = argv[2]->f;
      x = x+y+z;
    }
    else {
      string dname = convertOSCStringToDynadString(controllerName);
      //      cout <<"\"" << dname << "\"" << endl;
      if (argc>0) {
	if (myclass->focalSynth->interpretOSCControlMessage(dname,argv[0]->f)) {
	  cout << "Received from " << controllerName << " to " << dname << ": " << argv[0]->f << endl;
	}
	else {
	  cout << "Unknown from " << controllerName << " ";
	  for (int i = 0; i<argc; i++) {
	  lo_arg_pp((lo_type)types[i], argv[i]);
	  cout << ", ";
	  }
	  cout << endl;
	}
      }
    }

#if 0
    //    cout << path << " " << argc << endl;
    string spath = path;
    if (spath == "/3/xy1") {
      if (argc > 1 && myclass && myclass->focalSynth) {
	RController c1 = myclass->focalSynth->controlSet->getController ("normalSweep_frq");
	RController c2 = myclass->focalSynth->controlSet->getController ("normalSweep_wd");
	cout << "Received from " << spath << ": " << argv[0]->f << ", " << argv[1]->f << endl;
	c1->receiveOSCControlMessage (argv[0]->f);
	c2->receiveOSCControlMessage (argv[1]->f);
	
      }
    }
    else if (spath == "/3/xy2") {
      if (argc > 1 && myclass && myclass->focalSynth) {
	RController c1 = myclass->focalSynth->controlSet->getController ("poissonSweep_lambda");
	RController c2 = myclass->focalSynth->controlSet->getController ("poissonSweep");
	cout << "Received from " << spath << ": " << argv[0]->f << ", " << argv[1]->f << endl;
	c1->receiveOSCControlMessage (argv[0]->f);
	c2->receiveOSCControlMessage (argv[1]->f);
	
      }
    }
    else if (spath.substr(0,14) == "/2/multifader1") {
      int partial = stoi(spath.substr(15,2)) -1;
      string pname = "level_"+to_string(partial);
      RController pcont = myclass->focalSynth->controlSet->getController (pname);
      pcont->receiveOSCControlMessage(argv[0]->f);
	cout << "Received from " << spath << ": " << argv[0]->f << endl;
    }
    else
      cout << "Unknown from " << spath << endl;

#endif
    return 1;
}

void osc_error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

int
main(void)
{

  MainApplication app;
  app.init();
  app.run();


  return 0;
}





