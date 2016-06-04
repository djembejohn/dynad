#pragma once
#include "buffer.h"
#include "countedMutex.h"
#include <fstream>

using namespace std;

extern double midiNoteToFrequency (int note);
extern double stringToFrequency (string note);
extern int stringToMidiNote (string note);

// Trying to think about what I want for a controller
// 
// I want a layer that can interface with OSC and midi.
// Ideally it should be pretty close to OSC, so it seamlessly integrates
// It should all be pack-upable and save-able


// OK, so I'd like to simply put my controllers in a network. How can
// I do that but stop things going into loops?

// One option is to have some code which checks for loops in the
// controller network. OK, I'll make it flexible enough to do that for
// now and I'll implement the loop checking at some point in the future.

// What I want are the following features:
//
// The ability for some controllers to be grouped together into
// 'patches' Then a patch can be saved individually. I want to be able
// to morph between patches.
//
// To do that, architecturally, everything in the synth is designed to
// be real-time updatable. So while something is playing it checks the
// parameters for that as it happens.
//
// A problem with a network is that it's a bit confusing, but there's
// a future implementation problem on that front in how to organise
// it.  For now I'll just implement it with that degree of
// flexibility.

// So, each controller has a type and a name. Both of these are
// identified by unique strings. 

// Controllers are maintained as references. This allows it to control
// many things.

// So, everything in the synthesiser is an object in the network.
// 
// Messages come in from midi or OSC controllers and propagate through
// to where they should be.

// Loading/Saving
//
// There is an (abstract?) class factory for creating the objects. It
// reads in json from the save file.
//
// Each object has a type string and a name string.
// Fixed parameters are assigned with values - json style.
// Variable parameters are assigned with controllers.

class KnobController
{
 public:
  
  KnobController ()
    : column(-1), knobNumber (-1)
    {}

  KnobController (int _column, int _knobNumber)
    : column(_column), knobNumber(_knobNumber)
  {}

  int column;
  int knobNumber;

  friend bool operator < (KnobController const& lhs, KnobController const& rhs);

};

#if 1
class ControlVariable 
{
 public:
  ControlVariable (int _b0, int _b1)
    : b0(_b0), b1(_b1)
  {}

  int b0;
  int b1;

  friend bool operator < (ControlVariable const& lhs, ControlVariable const& rhs);


};
#endif

// abstract class which represents the interface for the controllers

class AController: public PCountedMutexHeapObject
{
 public:
  AController(double _level)
    : level (_level) 
  {}

  virtual void receiveMidiControlMessage (int value) = 0;

  virtual int generateMidiControlMessageValue () = 0;

  
  double getLevel () {
    return level;
  }

  void setLevel (double _level) {
    level = _level;
  }

 protected:
  double level; 

};



class NoteControlledFrequency: public AController
{
 public:

  NoteControlledFrequency (double _frequency)
    : AController(_frequency)
  {}
  void receiveMidiControlMessage(int value){
    // update the frequencyMultiplier in some way based on the value
    level = midiNoteToFrequency(value);
  }

  int generateMidiControlMessageValue () {
    cerr << "NoteControlledFrequency::generateMidiControlMessageValue is not implemented yet." << endl;
    return 54;
  }

};

class ControllableFrequencyMultiplier: public AController
{
 public:

  ControllableFrequencyMultiplier (double _level)
    : AController(_level)
  {}
  
  void receiveMidiControlMessage(int value){
    // update the frequencyMultiplier in some way based on the value
    level = (double)value+1.0;
  }
  int generateMidiControlMessageValue () {
    return (int)(level-1);
  }

};

class ControllableFrequencyLFO: public AController
{
 public:
  double frequency;
  ControllableFrequencyLFO (double _level=2.0)
    : AController( _level)
  {}
  void receiveMidiControlMessage(int value){
    level = 0.1+exponentialLookUpTable.getValueConcaveIncreasing (double(value)/(127.0))*20;
  }   
  int generateMidiControlMessageValue () {
    return (int)(127*exponentialLookUpTable.getInverseValueConcaveIncreasing((level-0.1)/20));
  }
};


class ControllableLevelSquared: public AController
{
 public:
  ControllableLevelSquared (double _level=0.0)
    : AController( _level)
  {}
  void receiveMidiControlMessage(int value){
    level = double(value*value)/(127.0*127.0);
    cout << "New squared level=" << level << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(sqrt(level*127*127));
  }

};

class ControllableLevelLinear: public AController
{
 public:
  ControllableLevelLinear (double _level=0.0)
    : AController( _level)
  {}
  void receiveMidiControlMessage(int value){
    level = double(value)/(127.0);
    cout << "New linear level=" << level << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(level *127);
  }
};


class ControllableLevelConcaveIncreasing: public AController
{
 public:
  ControllableLevelConcaveIncreasing (double _level=0.0, double _levelMultiplier = 1.0)
    : AController( _level), levelMultiplier(_levelMultiplier)
  {}
  void receiveMidiControlMessage(int value){
    level = levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (double(value)/(127.0));
    cout << "New concave exponential level=" << level << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(level/levelMultiplier)*127);
  }

 private:
  double levelMultiplier;
};


// the idea is that you create one of these and then pass it to the
// object that is being controlled, and obs your buncle
typedef PCountedMutexHolder <AController> RController;

typedef PCountedMutexHolder <NoteControlledFrequency> RNoteControlledFrequency;
typedef PCountedMutexHolder <ControllableFrequencyMultiplier> RControllableFrequencyMultiplier;
typedef PCountedMutexHolder <ControllableFrequencyLFO> RControllableFrequencyLFO;

typedef PCountedMutexHolder <ControllableLevelSquared> RControllerSquared;
typedef PCountedMutexHolder <ControllableLevelConcaveIncreasing> RControllerConcaveIncreasing;
typedef PCountedMutexHolder <ControllableLevelLinear> RControllerLinear;



class ControllerSet 
{
 public:

  // ok, give me a controller and a string
  //
  // I know which knob goes with that controller


  void addPartialController (RController controller, int column, int row, string name) {
    KnobController kc (column,row);
    controllerNames[name+"_"+to_string(column)] = controller;
    knobs[name+"_"+to_string(column)] = kc;
    controllers[kc] = controller;
  }

 void addController (RController controller, int column, int row, string name) {
    KnobController kc = KnobController (column,row);
    controllerNames[name] = controller;
    knobs[name] = kc;
    controllers[kc] = controller;
  }

  ControllerSet ()  {
    for (int i = 0; i<16; i++) {

      RControllableFrequencyMultiplier freqMult (new ControllableFrequencyMultiplier (1.0*(i+1)));
      addPartialController (freqMult.GetPointer(),i,0,"freqMult");

      RControllerConcaveIncreasing level (new ControllableLevelConcaveIncreasing (exp(-i)));
      addPartialController (level.GetPointer(),i,1,"level");
      
      RControllerLinear frequencyNoise(new ControllableLevelLinear (0.00));
      addPartialController (frequencyNoise.GetPointer(),i,2,"frequencyNoise");

      RControllerConcaveIncreasing shape (new ControllableLevelConcaveIncreasing (0.0));
      addPartialController(shape.GetPointer(),i,3,"shape");

      RControllerConcaveIncreasing distortion (new ControllableLevelConcaveIncreasing (0.0));
      addPartialController(distortion.GetPointer(),i,6,"distortion");
      
      RControllableFrequencyLFO amposcfreq (new ControllableFrequencyLFO (2.0));
      addPartialController(amposcfreq.GetPointer(),i,10,"amposcfreq");

      RControllerConcaveIncreasing amposclevel (new ControllableLevelConcaveIncreasing (0.0));
      addPartialController(amposclevel.GetPointer(),i,11,"amposclevel");
      
      RControllableFrequencyLFO freqoscfreq (new ControllableFrequencyLFO (2.0));
      addPartialController(freqoscfreq.GetPointer(),i,12,"freqoscfreq");

      RControllerConcaveIncreasing freqosclevel (new ControllableLevelConcaveIncreasing (0.0));
      addPartialController(freqosclevel.GetPointer(),i,13,"freqosclevel");

    }
  }

  map<string,RController> controllerNames;
  map<string,KnobController > knobs;
  map<KnobController, RController> controllers;
  // add save and load and morph etc

  RController getController (KnobController kc) {
    map<KnobController,RController>::iterator cni = controllers.find(kc);
    if (cni != controllers.end())
	return cni->second;
    else
      return 0;
  }

  
  RController getController (string name, int column) {
    name = name + "_" + to_string(column);
    map<string,RController>::iterator cni = controllerNames.find(name);
    if (cni != controllerNames.end())
	return cni->second;
    else
      return 0;
  }


  void save (string filename) {
    ofstream outf (filename);
    for (auto controller:controllerNames) {
      outf << controller.first << " " 
	   << controller.second->getLevel() << endl;
    }
  }

  void load (string filename) {
    ifstream instr (filename);
    string line;
    while (getline (instr,line)) {
      string cname;
      double value;
      stringstream(line) >> cname >> value;
      controllerNames[cname]->setLevel (value);
    }
  }


};

typedef shared_ptr<ControllerSet> RControllerSet;
