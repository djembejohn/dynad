// This file is part of Dynad.

//    Dynad is free software: you can redistribute it and/or modify it
//    under the terms of the GNU Affero General Public License as
//    published by the Free Software Foundation, either version 3 of
//    the License, or (at your option) any later version.

//    Dynad is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU Affero General Public License
//    along with Dynad.  If not, see <http://www.gnu.org/licenses/>.

#pragma once
#include "buffer.h"
#include "countedMutex.h"
#include <boost/math/distributions/lognormal.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <fstream>

using namespace std;

extern StkFloat midiNoteToFrequency (int note);
extern StkFloat stringToFrequency (string note);
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

  

class ControlVariable 
{
 public:
  //  ControlVariable (int _byte0, int _byte1)
  //    : byte0(_byte0), byte1(_byte1)
  //  {}

  ControlVariable (int _channel, int _controller)
    : channel(_channel), controller(_controller)
  {}

  int channel;
  int controller;

  friend bool operator < (ControlVariable const& lhs, ControlVariable const& rhs);


};

// abstract class which represents the interface for the controllers

class AController: public PCountedMutexHeapObject
{
 public:
 AController(string _name="", StkFloat _level=1.0)
   : name(_name), level (_level), outputLevel (level), adjustment(0.0)
  {}
  
  // return true if this should update the controllers on the input
  // device
  virtual void receiveMidiControlMessage (int value) = 0;
  
  virtual int generateMidiControlMessageValue () = 0;

  StkFloat getOutputLevel() {
    StkFloat outputLevel = level + adjustment;
    if (outputLevel <0)
      outputLevel = 0;
    //    if (name == "level_0") 
    //      conlog << level << " " << targetLevel << " " << outputChange << " " << outputLevel << endl;
    return outputLevel;
  }
  
  StkFloat getLevel () {
    return level;
  }
  
  virtual bool setLevel (StkFloat _level) {
    level = _level;
    return false;
  }

  void setAdjustment (StkFloat amount) {
    //    cout << "New adjustment: " << amount << endl;
    adjustment = amount;
  }

  void setName (string & newName) {
    name = newName;
  }
  
  string getName () {
    return name;
  }

  virtual void postLoadUpdate () {
  }

 private:
  string name;
  StkFloat level; 

  StkFloat outputLevel;
  StkFloat adjustment;


};

typedef PCountedMutexHolder <AController> RController;

class SmoothedController
{
 public:
    RController controller;
 
    SmoothedController (RController _controller, StkFloat _smoothingFactor)
      : controller(_controller), level (controller->getOutputLevel()), smoothingFactor(_smoothingFactor)
      {}

    StkFloat getOutputLevel() {
      StkFloat targetLevel = controller->getOutputLevel();
      StkFloat change = targetLevel-level;
      change = change*smoothingFactor;
      level += change;

      return level;
    }
    
 private:
    StkFloat level;
    StkFloat smoothingFactor;

 };


class NoteControlledFrequency: public AController
{
 public:

  NoteControlledFrequency (string name, StkFloat _frequency)
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

  ControllableFrequencyMultiplier (string name, StkFloat _level)
    : AController(name,_level)
  {}
  
  void receiveMidiControlMessage(int value){
    // update the frequencyMultiplier in some way based on the value
    setLevel ((StkFloat)value+1.0);
  }
  int generateMidiControlMessageValue () {
    return (int)(getLevel()-1);
  }

};

class ControllableFrequencyLFO: public AController
{
 public:
  StkFloat frequency;
  ControllableFrequencyLFO (string name, StkFloat _level=2.0)
    : AController(name, _level)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (0.1+exponentialLookUpTable.getValueConcaveIncreasing (StkFloat(value)/(127.0))*20);
  }   
  int generateMidiControlMessageValue () {
    return (int)(127*exponentialLookUpTable.getInverseValueConcaveIncreasing((getLevel()-0.1)/20));
  }
};


class ControllableLevelSquared: public AController
{
 public:
  ControllableLevelSquared (string name, StkFloat _level=0.0)
    : AController(name, _level)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (StkFloat(value*value)/(127.0*127.0));
    //    cout << "New squared level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(sqrt(getLevel()*127*127));
  }

};

class ControllableLevelLinear: public AController
{
 public:
  ControllableLevelLinear (string name, StkFloat _level=0.0, StkFloat _min = 0.0, StkFloat _max = 1.0)
    : AController(name, _level),maxLevel(_max),minLevel(_min)
  {}


  void receiveMidiControlMessage(int value){
    setLevel (minLevel + (maxLevel-minLevel)*StkFloat(value)/(127.0));
    //    cout << "New linear level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)((getLevel()-minLevel) *127.0/(maxLevel-minLevel));
  }

  StkFloat maxLevel;
  StkFloat minLevel;
};


class ControllableLevelConcaveIncreasing: public AController
{
 public:
  ControllableLevelConcaveIncreasing (string name, StkFloat _level=0.0, StkFloat _levelMultiplier = 1.0)
    : AController(name, _level), levelMultiplier(_levelMultiplier)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (StkFloat(value)/(127.0)));
    //    cout << "New concave exponential level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(getLevel()/levelMultiplier)*127);
  }

 private:
  StkFloat levelMultiplier;
};

class ControllableLevelConcaveDecreasing: public AController
{
 public:
  ControllableLevelConcaveDecreasing (string name, StkFloat _level=0.0, StkFloat _levelMultiplier = 1.0)
    : AController(name, _level), levelMultiplier(_levelMultiplier)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveDecreasing (StkFloat(value)/(127.0)));
    //    cout << "New concave exponential level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveDecreasing(getLevel()/levelMultiplier)*127);
  }

 private:
  StkFloat levelMultiplier;
};




// the idea is that you create one of these and then pass it to the
// object that is being controlled, and obs your buncle

typedef PCountedMutexHolder <NoteControlledFrequency> RNoteControlledFrequency;
typedef PCountedMutexHolder <ControllableFrequencyMultiplier> RControllableFrequencyMultiplier;
typedef PCountedMutexHolder <ControllableFrequencyLFO> RControllableFrequencyLFO;

typedef PCountedMutexHolder <ControllableLevelSquared> RControllerSquared;
typedef PCountedMutexHolder <ControllableLevelConcaveIncreasing> RControllerConcaveIncreasing;
typedef PCountedMutexHolder <ControllableLevelConcaveDecreasing> RControllerConcaveDecreasing;
typedef PCountedMutexHolder <ControllableLevelLinear> RControllerLinear;

// this controller defines a way to receive messages from sub
// controllers
class AMasterController : public AController 
{
 public:

  AMasterController (string name, StkFloat level)
    : AController (name,level)
  { }
  
  // this is from the input
  virtual bool receiveLevelFromSubController (string & name, StkFloat level) = 0;

};

typedef PCountedMutexHolder<AMasterController> RMasterController;

// this controller receives a message and passes it onto another
// controller
class SubController : public AController
{
 public:

  SubController (string name, RMasterController _master, string _conclass, StkFloat level,StkFloat smoothingFactor = 1.0)
    : AController(name,level), master(_master), conclass(_conclass)
  {
    master->receiveLevelFromSubController (name,level);
  }
  
  bool setLevel (StkFloat level) {
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
  SubControllableLevelConcaveIncreasing (string name, RMasterController master, string conclass, StkFloat level=0.0, StkFloat _levelMultiplier = 1.0)
    : SubController(name,master,conclass,level), levelMultiplier(_levelMultiplier)
  {}
  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (StkFloat(value)/(127.0)));
    //    cout << "New concave exponential level=" << level << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(getLevel()/levelMultiplier)*127);
  }

 private:
  StkFloat levelMultiplier;
};


class SubControllableLevelLinear: public SubController
{
 public:
  SubControllableLevelLinear (string name,RMasterController master, string conclass, StkFloat level=0.0, StkFloat _min = 0.0, StkFloat _max = 1.0)
    : SubController(name, master,conclass,level), maxLevel(_max),minLevel(_min)
  {}


  void receiveMidiControlMessage(int value){
    setLevel (minLevel + (maxLevel-minLevel)*StkFloat(value)/(127.0));
    //    cout << "New linear level=" << getLevel() << endl;
  }   
  int generateMidiControlMessageValue () {
    return (int)((getLevel()-minLevel) *127.0/(maxLevel-minLevel));
  }

 private:
  StkFloat maxLevel;
  StkFloat minLevel;
};


// multi partial controllers

// This one takes a vector of controllers, and then adjusts their levels

class ControllerNormalSweep : public AMasterController
{
 public:

  ControllerNormalSweep (string name, vector<RController> & _controllers, StkFloat level, StkFloat _levelMultiplier=1.0)
    : AMasterController(name,level), levelMultiplier(_levelMultiplier), controllers(_controllers)
  {
    variables["wd"] = 1.0;
    variables["frq"] = 1.0;
    variables["ns"] = 0.0;
  }
  
  bool receiveLevelFromSubController (string & conclass, StkFloat level) {
    //    cout << "Master received level "<< level << " from " << conclass << endl;
    variables[conclass] = level;
    updateControllers ();
    return true;
  }

  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (StkFloat(value)/(127.0)));
    updateControllers();
  }

  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(getLevel()/levelMultiplier)*127);

  }

  void postLoadUpdate() {
    //    cout << "Updating controllers" << endl;
    updateControllers();
  }

  void updateControllers ()  {

    StkFloat freq = variables["frq"]; 
    StkFloat width = variables["wd"];
    StkFloat noise = variables["ns"];
    StkFloat mf1 = 1.0/sqrt(2.0*width*width*local_pi);
    StkFloat mf2 = 1.0/(2.0*width*width);
    
    StkFloat level = getLevel();

    //    cout << "level="<<level<<", freq="<<freq<<", width="<<width<< endl;


    int counter = 0;
    for (auto controller: controllers) {
      StkFloat xval = (StkFloat)counter-freq;
      StkFloat thisval = level*mf1*exp(-xval*xval*mf2);
      StkFloat noiseval = thisval;
      if (noise>0 && thisval>1e-6) {
	gen_type gen(RNGGEN, lognormal_distribution_type (thisval, noise));
	noiseval = gen();
      }
      //      cout << thisval << " " << noiseval;
      controller->setAdjustment(noiseval);
      counter ++;
    }
    //    cout << endl;
  

  }

 private:

  StkFloat levelMultiplier;
  map<string,StkFloat> variables;
  vector<RController> controllers;


};


class ControllerPoissonSweep : public AMasterController
{
 public:

  ControllerPoissonSweep (string name, vector<RController> & _controllers, StkFloat level, StkFloat _levelMultiplier=1.0)
    : AMasterController(name,level), levelMultiplier(_levelMultiplier), controllers(_controllers)
  {
    variables["lambda"] = 1.0;
  }
  
  bool receiveLevelFromSubController (string & conclass, StkFloat level) {
    //    cout << "Master received level "<< level << " from " << conclass << endl;
    variables[conclass] = level;
    updateControllers ();
    return true;
  }

  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (StkFloat(value)/(127.0)));
    updateControllers();
  }

  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(getLevel()/levelMultiplier)*127);

  }

  void postLoadUpdate() {
    //    cout << "Updating controllers" << endl;
    updateControllers();
  }

  void updateControllers ()  {

    StkFloat lambda = variables["lambda"]; 
    
    StkFloat level = getLevel();

    //    cout << "level="<<level<<", freq="<<freq<<", width="<<width<< endl;


    int counter = 0;
    boost::math::poisson_distribution<> poisson (lambda);
    for (auto controller: controllers) {
      StkFloat xval = (StkFloat)counter;
      
      StkFloat thisval = level*boost::math::pdf(poisson,xval);
      //      cout << thisval << " " << noiseval;
      controller->setAdjustment(thisval);
      counter ++;
    }
    //    cout << endl;
  

  }

 private:

  StkFloat levelMultiplier;
  map<string,StkFloat> variables;
  vector<RController> controllers;


};


class ControllerFrequencyAdjuster : public AMasterController
{
 public:

  ControllerFrequencyAdjuster (string name, vector<RController> & _controllers, StkFloat level, StkFloat _levelMultiplier=1.0)
    : AMasterController(name,level), levelMultiplier(_levelMultiplier), controllers(_controllers)
  {
    //    variables["wd"] = 1.0;
  }
  
  bool receiveLevelFromSubController (string & conclass, StkFloat level) {
    //    cout << "Master received level "<< level << " from " << conclass << endl;
    variables[conclass] = level;
    updateControllers ();
    return true;
  }

  void receiveMidiControlMessage(int value){
    setLevel (levelMultiplier * exponentialLookUpTable.getValueConcaveIncreasing (StkFloat(value)/(127.0)));
    updateControllers();
  }

  int generateMidiControlMessageValue () {
    return (int)(exponentialLookUpTable.getInverseValueConcaveIncreasing(getLevel()/levelMultiplier)*127);

  }

  void postLoadUpdate() {
    //    cout << "Updating controllers" << endl;
    updateControllers();
  }

  void updateControllers ()  {

    // need to implement a sigmoid function for this so it can go
    // negative
    StkFloat amount = getLevel()+1.0;

    for (auto controller: controllers) {
      StkFloat freqmult = controller->getLevel();
      
      if (freqmult!=1.0) {
	//	StkFloat newval = freqmult*amount;
	//	controller->setAdjustment(newval-freqmult);
	controller->setAdjustment(amount-1.0);
      }
    }
    //    cout << endl;
  

  }

 private:

  StkFloat levelMultiplier;
  map<string,StkFloat> variables;
  vector<RController> controllers;

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

  ControllerSet (int numPartials)  {

    RController volume (new ControllableLevelConcaveIncreasing ("masterVolume",0.1));
    RController pan (new ControllableLevelLinear ("pan",0.5));
    addController (volume,16,0);
    addController (pan,16,1);

    RController distortion (new ControllableLevelConcaveIncreasing ("distortion",0.0));
    addController (distortion,16,2);



    RController attack (new ControllableLevelLinear ("masterAttack",1.0,0.01,3.0));
    addController (attack,17,0);
    RController decay (new ControllableLevelLinear ("masterDecay",0.001,0.01,1.0));
    addController (decay,17,1);
    RController sustain (new ControllableLevelLinear ("masterSustain",1.0));
    addController (sustain,17,2);
    RController release (new ControllableLevelLinear ("masterRelease",2.0,0.01,10.0));
    addController (release,17,3);

    RController chorusModDepth (new ControllableLevelLinear ("chorusModDepth"));
    RController chorusModFrequency (new ControllableLevelLinear ("chorusModFrequency"));
    RController chorusMix (new ControllableLevelLinear ("chorusMix",0.0));

    RController echoDelay (new ControllableLevelLinear ("echoDelay",48000/4,1,48000*5));
    RController echoFeedback (new ControllableLevelLinear ("echoFeedback",0.3));
    RController echoMix (new ControllableLevelLinear ("echoMix",0.0));

    RController reverbRoomSize (new ControllableLevelLinear ("reverbRoomSize",0.75));
    RController reverbDamping (new ControllableLevelLinear ("reverbDamping",0.25));
    RController reverbWidth (new ControllableLevelLinear ("reverbWidth",1.0));
    RController reverbMix (new ControllableLevelLinear ("reverbMix",0.0));


    addController (chorusMix,21,0);
    addController (chorusModDepth,12,1);
    addController (chorusModFrequency,21,2);


    addController (echoMix,22,0);
    addController (echoDelay,22,1);
    addController (echoFeedback,22,2);
  
    addController (reverbMix,23,0);
    addController (reverbRoomSize,23,1);
    addController (reverbDamping,23,2);
    addController (reverbWidth,23,3);

    vector<RController> partialLevels;
    vector<RController> partialFrequencies;

    for (int i = 0; i<numPartials; i++) {

      RControllableFrequencyMultiplier freqMult (new ControllableFrequencyMultiplier (getColumnName("freqMult",i),1.0*(i+1)));
      addController (freqMult.GetPointer(),i,0);

      partialFrequencies.push_back(RController(freqMult));

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


    ControllerFrequencyAdjuster * fadj = new ControllerFrequencyAdjuster ("freqAdj",partialFrequencies,0.0,5.0);
    RMasterController freqAdj (fadj);
    addController(freqAdj.GetPointer(),19,0);


    ControllerPoissonSweep * pois = new ControllerPoissonSweep ("poissonSweep",partialLevels,0.0,10.0);
    RMasterController poissonSweep (pois);
    addController(poissonSweep.GetPointer(),19,2);

    RController lambda (new SubControllableLevelLinear
		      (
		       "poissonSweep_lambda",
		       poissonSweep, "lambda", 0.01,0.01,numPartials+5
		       )
		      );
    addController(lambda,19,3);

    
    //    for (auto & ci:controllerNames)
    //      cout << ci.first << " ";
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
      StkFloat value;
      stringstream(line) >> cname >> value;
      RController con = controllerNames[cname];
      if (con)
	con->setLevel (value);
    }
    for (auto conandname:controllerNames) {
      conandname.second->postLoadUpdate();
    }
  }

  void outputControllersToConsole () {
    for (auto controller:controllerNames) {
      cout << controller.first << " " 
	   << controller.second->getLevel() 
	   << " "  << controller.second->getOutputLevel() 
	   << endl;
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

  void morphOutput (StkFloat value) {
    if (value > 1 || value < 0) {
      cerr << "Inappropriate morph value" << endl;
      throw "bla";
    }
    cout << "Morph " << value << endl;
      
    for (unsigned int i = 0; i< outcontrollers.size(); i++) {
      StkFloat newLevel = (1.0-value)*in1[i]->getLevel() + value*in2[i]->getLevel();
      outcontrollers[i]->setLevel(newLevel);
    }
  }
  

};

typedef shared_ptr<MorphControllerSet> RMorphControllerSet;

typedef pair<string,int> DynadController;

// just starting with control variables
class ControlInputMap
{
 public:
  
  multimap<ControlVariable,DynadController> inputMap;
  multimap<KnobController,DynadController> inputMapBCR;

  void load (string fname) {
    inputMap.clear();
    inputMapBCR.clear();
    ifstream instr (fname);
    string line;
    while (getline(instr,line)) {
      string type;
      int entry1;
      int entry2;
      string controlName;
      int channel = -1;
      // types are bcr, mcn
      stringstream(line) >> type >> entry1 >> entry2 >> controlName >> channel;

      if (channel>0) {
	channel -= 1;
	if (type == "bcr") {
	  KnobController kc (entry1,entry2);
	  inputMapBCR.insert (make_pair (kc,make_pair(controlName,channel)));
	}
	else if (type == "mcn") {
	  ControlVariable cv (entry1-1,entry2);
	  inputMap.insert (make_pair (cv,make_pair(controlName,channel)));  
	}
      }
    }
  }
};
