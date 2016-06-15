#pragma once
#include "buffer.h"
#include "countedMutex.h"
#include <boost/math/distributions/lognormal.hpp>
#include <fstream>

using namespace std;

extern double midiNoteToFrequency (int note);
extern double stringToFrequency (string note);
extern int stringToMidiNote (string note);

typedef boost::mt19937 base_generator_type;
typedef boost::lognormal_distribution<> lognormal_distribution_type;
typedef boost::variate_generator<base_generator_type&, lognormal_distribution_type> gen_type;

extern base_generator_type RNGGEN;
extern ofstream conlog;

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
  friend bool operator == (KnobController const& lhs, KnobController const& rhs);

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
 AController(string _name="", double _level=1.0)
   : name(_name), level (_level), outputLevel (level), adjustment(0.0)
  {}
  
  // return true if this should update the controllers on the input
  // device
  virtual void receiveMidiControlMessage (int value) = 0;
  
  virtual int generateMidiControlMessageValue () = 0;

  double getOutputLevel() {
    double outputLevel = level + adjustment;
    if (outputLevel <0)
      outputLevel = 0;
    //    if (name == "level_0") 
    //      conlog << level << " " << targetLevel << " " << outputChange << " " << outputLevel << endl;
    return outputLevel;
  }
  
  double getLevel () {
    return level;
  }
  
  bool setLevel (double _level) {
    level = _level;
    return false;
  }

  void setAdjustment (double amount) {
    adjustment = amount;
  }

  void setName (string & newName) {
    name = newName;
  }
  
  string getName () {
    return name;
  }

 private:
  string name;
  double level; 

  double outputLevel;
  double adjustment;


};

typedef PCountedMutexHolder <AController> RController;

class SmoothedController
{
 public:
    RController controller;
 
    SmoothedController (RController _controller, double _smoothingFactor)
      : controller(_controller), level (controller->getOutputLevel()), smoothingFactor(_smoothingFactor)
      {}

    double getOutputLevel() {
      double targetLevel = controller->getOutputLevel();
      double change = targetLevel-level;
      change = change*smoothingFactor;
      level += change;

      return level;
    }
    
 private:
    double level;
    double smoothingFactor;

 };


class NoteControlledFrequency: public AController
{
 public:

  NoteControlledFrequency (string name, double _frequency)
    : AController(name, _frequency)
  {}

  void receiveMidiControlMessage(int value){
    // update the frequencyMultiplier in some way based on the value
    setLevel (midiNoteToFrequency(value));
  }

  int generateMidiControlMessageValue () {
    cerr << "NoteControlledFrequency::generateMidiControlMessageValue is not implemented yet." << endl;
    return 54;
  }

};

class ControllableFrequencyMultiplier: public AController
{
 public:

  ControllableFrequencyMultiplier (string name, double _level)
    : AController(name,_level)
  {}
  
  void receiveMidiControlMessage(int value){
    // update the frequencyMultiplier in some way based on the value
    setLevel ((double)value+1.0);
  }
  int generateMidiControlMessageValue () {
    return (int)(getLevel()-1);
  }

};

class ControllableFrequencyLFO: public AController
{
 public:
  double frequency;
  ControllableFrequencyLFO (string name, double _level=2.0)
    : AController(name, _level)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (0.1+exponentialLookUpTable.getValueConcaveIncreasing (double(value)/(127.0))*20);
  }   
  int generateMidiControlMessageValue () {
    return (int)(127*exponentialLookUpTable.getInverseValueConcaveIncreasing((getLevel()-0.1)/20));
  }
};


class ControllableLevelSquared: public AController
{
 public:
  ControllableLevelSquared (string name, double _level=0.0)
    : AController(name, _level)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (double(value*value)/(127.0*127.0));
    //    cout << "New squared level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(sqrt(getLevel()*127*127));
  }

};

class ControllableLevelLinear: public AController
{
 public:
  ControllableLevelLinear (string name, double _level=0.0, double _min = 0.0, double _max = 1.0)
    : AController(name, _level),maxLevel(_max),minLevel(_min)
  {}


  void receiveMidiControlMessage(int value){
    setLevel (minLevel + (maxLevel-minLevel)*double(value)/(127.0));
    //    cout << "New linear level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)((getLevel()-minLevel) *127.0/(maxLevel-minLevel));
  }

  double maxLevel;
  double minLevel;
};


class ControllableLevelConcaveIncreasing: public AController
{
 public:
  ControllableLevelConcaveIncreasing (string name, double _level=0.0, double _levelMultiplier = 1.0)
    : AController(name, _level), levelMultiplier(_levelMultiplier)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (double(value)/(127.0)));
    //    cout << "New concave exponential level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(getLevel()/levelMultiplier)*127);
  }

 private:
  double levelMultiplier;
};


// the idea is that you create one of these and then pass it to the
// object that is being controlled, and obs your buncle

typedef PCountedMutexHolder <NoteControlledFrequency> RNoteControlledFrequency;
typedef PCountedMutexHolder <ControllableFrequencyMultiplier> RControllableFrequencyMultiplier;
typedef PCountedMutexHolder <ControllableFrequencyLFO> RControllableFrequencyLFO;

typedef PCountedMutexHolder <ControllableLevelSquared> RControllerSquared;
typedef PCountedMutexHolder <ControllableLevelConcaveIncreasing> RControllerConcaveIncreasing;
typedef PCountedMutexHolder <ControllableLevelLinear> RControllerLinear;

// this controller defines a way to receive messages from sub
// controllers
class AMasterController : public AController 
{
 public:

  AMasterController (string name, double level)
    : AController (name,level)
  { }
  
  // this is from the input
  virtual bool receiveLevelFromSubController (string & name, double level) = 0;

};

typedef PCountedMutexHolder<AMasterController> RMasterController;

// this controller receives a message and passes it onto another
// controller
class SubController : public AController
{
 public:

  SubController (string name, RMasterController _master, string _conclass, double level,double smoothingFactor = 1.0)
    : AController(name,level), master(_master), conclass(_conclass)
  {
    master->receiveLevelFromSubController (name,level);
  }
  
  bool setLevel (double level) {
    bool response = master->receiveLevelFromSubController (conclass, level);
    AController::setLevel(level);
    return response;
  }
  
 protected:
  RMasterController master;
  string conclass;
  
};


class SubControllableLevelConcaveIncreasing: public SubController
{
 public:
  SubControllableLevelConcaveIncreasing (string name, RMasterController master, string conclass, double level=0.0, double _levelMultiplier = 1.0)
    : SubController(name,master,conclass,level), levelMultiplier(_levelMultiplier)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (double(value)/(127.0)));
    //    cout << "New concave exponential level=" << level << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(getLevel()/levelMultiplier)*127);
  }

 private:
  double levelMultiplier;
};


class SubControllableLevelLinear: public SubController
{
 public:
  SubControllableLevelLinear (string name,RMasterController master, string conclass, double level=0.0, double _min = 0.0, double _max = 1.0)
    : SubController(name, master,conclass,level), maxLevel(_max),minLevel(_min)
  {}


  void receiveMidiControlMessage(int value){
    setLevel (minLevel + (maxLevel-minLevel)*double(value)/(127.0));
    cout << "New linear level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)((getLevel()-minLevel) *127.0/(maxLevel-minLevel));
  }

 private:
  double maxLevel;
  double minLevel;
};


// multi partial controllers

// This one takes a vector of controllers, and then adjusts their levels

class ControllerNormalSweep : public AMasterController
{
 public:

  ControllerNormalSweep (string name, vector<RController> & _controllers, double level, double _levelMultiplier=1.0)
    : AMasterController(name,level), levelMultiplier(_levelMultiplier), controllers(_controllers), previousChanges(16,0.0)
  {
    variables["wd"] = 1.0;
    variables["frq"] = 1.0;
  }
  
  bool receiveLevelFromSubController (string & conclass, double level) {
    variables[conclass] = level;
    updateControllers ();
    return true;
  }

  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (double(value)/(127.0)));
    updateControllers();
  }

  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(getLevel()/levelMultiplier)*127);

  }

  void updateControllers ()  {

    double freq = variables["frq"]; 
    double width = variables["wd"];
    double noise = variables["ns"];
    double mf1 = 1.0/sqrt(2.0*width*width*local_pi);
    double mf2 = 1.0/(2.0*width*width);
    
    double level = getLevel();

    cout << "level="<<level<<", freq="<<freq<<", width="<<width<< endl;



    for (int i = 0; i<16; i++) {
      double xval = (double)i-freq;
      double thisval = level*mf1*exp(-xval*xval*mf2);
      double noiseval = thisval;
      if (noise>0) {
	gen_type gen(RNGGEN, lognormal_distribution_type (thisval, noise));
	noiseval = gen();
      }
      cout << thisval << " ";
      controllers[i]->setAdjustment(noiseval);
    }
    cout << endl;
  

  }

 private:

  double levelMultiplier;
  map<string,double> variables;
  vector<RController> controllers;

  vector<double> previousChanges;

};



class ControllerSet 
{
 public:

  // ok, give me a controller and a string
  //
  // I know which knob goes with that controller
  string getColumnName (string name, int column) {
    return (name+"_"+to_string(column));
  }

 void addController (RController controller, int column, int row) {
    KnobController kc = KnobController (column,row);
    controllerNames[controller->getName()] = controller;
    knobs[controller->getName()] = kc;
    controllers[kc] = controller;
  }

  ControllerSet ()  {

    RController volume (new ControllableLevelLinear ("masterVolume",0.1,0.0,1.0));
    addController (volume,16,0);

    RController attack (new ControllableLevelLinear ("masterAttack",1.0,0.01,3.0));
    addController (attack,17,0);
    RController decay (new ControllableLevelLinear ("masterDecay",0.001,0.01,1.0));
    addController (decay,17,1);
    RController sustain (new ControllableLevelLinear ("masterSustain",1.0));
    addController (sustain,17,2);
    RController release (new ControllableLevelLinear ("masterRelease",2.0,0.01,10.0));
    addController (release,17,3);


    vector<RController> partialLevels;

    for (int i = 0; i<16; i++) {

      RControllableFrequencyMultiplier freqMult (new ControllableFrequencyMultiplier (getColumnName("freqMult",i),1.0*(i+1)));
      addController (freqMult.GetPointer(),i,0);

      RControllerConcaveIncreasing level (new ControllableLevelConcaveIncreasing (getColumnName("level",i),exp(-i),1.0));
      addController (level.GetPointer(),i,1);
      
      partialLevels.push_back(RController(level.GetPointer()));

      RControllerLinear frequencyNoise(new ControllableLevelLinear (getColumnName("frequencyNoise",i),0.00));
      addController (frequencyNoise.GetPointer(),i,2);


      RControllerConcaveIncreasing shape (new ControllableLevelConcaveIncreasing (getColumnName("shape",i),0.0));
      addController(shape.GetPointer(),i,6);

      RControllerConcaveIncreasing saw (new ControllableLevelConcaveIncreasing (getColumnName("saw",i),0.0));
      addController(saw.GetPointer(),i,7);

      RControllerConcaveIncreasing distortion (new ControllableLevelConcaveIncreasing (getColumnName("distortion",i),0.0));
      addController(distortion.GetPointer(),i,8);


      
      RControllableFrequencyLFO amposcfreq (new ControllableFrequencyLFO (getColumnName("amposcfreq",i),2.0));
      addController(amposcfreq.GetPointer(),i,10);

      RControllerConcaveIncreasing amposclevel (new ControllableLevelConcaveIncreasing (getColumnName("amposclevel",i),0.0));
      addController(amposclevel.GetPointer(),i,11);
      
      RControllableFrequencyLFO freqoscfreq (new ControllableFrequencyLFO (getColumnName("freqoscfreq",i),2.0));
      addController(freqoscfreq.GetPointer(),i,12);

      RControllerConcaveIncreasing freqosclevel (new ControllableLevelConcaveIncreasing (getColumnName("freqosclevel",i),0.0));
      addController(freqosclevel.GetPointer(),i,13);

    }
    
    ControllerNormalSweep * conc = new ControllerNormalSweep ("normalSweep",partialLevels,0.0,5.0);
    RMasterController normalSweep (conc);
    addController(normalSweep.GetPointer(),18,0);
    RController freq (new SubControllableLevelLinear
		      (
		       "normalSweep_frq",
		       normalSweep, "frq", 0.0,0.5,20.0
		       )
		      );
    addController(freq,18,1);
    RController width (new SubControllableLevelLinear
		       (
			"normalSweep_wd",
			normalSweep, "wd", 2, 0.01,10.0
			)
		       );
    addController(width,18,2);

    RController noise (new SubControllableLevelLinear
		       (
			"normalSweep_ns",
			normalSweep, "ns", 0, 0.00,0.1
			)
		       );
    addController(noise,18,3);
    
    for (auto & ci:controllerNames)
      cout << ci.first << " ";
    cout << endl;

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

  RController getController (string name) {
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

class MorphControllerSet 
{
 public:
  MorphControllerSet ()
    :  cvar_morphValue (24, 0) 
    {}
  RControllerSet outputSet;

  RControllerSet inputSet1;
  RControllerSet inputSet2;
  
  vector <RController> outcontrollers;
  vector <RController> in1;
  vector <RController> in2;

  KnobController cvar_morphValue;
  
  void initControlVectors () {
    outcontrollers.reserve (outputSet->controllerNames.size());
    in1.reserve(outputSet->controllerNames.size());
    in2.reserve(outputSet->controllerNames.size());
    for (auto controller:outputSet->controllerNames) {
      string name = controller.first;
      if (inputSet1->controllerNames.find(name) != inputSet1->controllerNames.end() 
	  && inputSet2->controllerNames.find(name) != inputSet2->controllerNames.end() ) {
	outcontrollers.push_back(controller.second);
	in1.push_back(inputSet1->controllerNames[name]);
	in2.push_back(inputSet2->controllerNames[name]);
      }
      else {
	cerr << "Ignoring missing controller '" << name << "' in input set" << endl;
      }
    }
  }

  void morphOutput (double value) {
    if (value > 1 || value < 0) {
      cerr << "Inappropriate morph value" << endl;
      throw "bla";
    }
    cout << "Morph " << value << endl;
      
    for (unsigned int i = 0; i< outcontrollers.size(); i++) {
      double newLevel = (1.0-value)*in1[i]->getLevel() + value*in2[i]->getLevel();
      outcontrollers[i]->setLevel(newLevel);
    }
  }
  

};

typedef shared_ptr<MorphControllerSet> RMorphControllerSet;

