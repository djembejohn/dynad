#pragma once
#include "buffer.h"
#include "jabVector.h"
#include <fstream>
#include "controller.h"

using namespace jab;
using namespace std;

extern ofstream envelopelog;

class Envelope
{
 public:
  // these need to become controlled
  RController attack;
  RController decay;
  RController sustain;
  RController release;
  bool verbose;

  Envelope (RController a,RController d,RController s,RController r)
    : attack(a),decay(d),sustain(s),release(r),
    verbose(false), releasePending(true),noteOnPending(false),
    tStarted(-1), tReleased(-1), 
    internalLevel(1.0), tStored (-1e9), levelStored(0)
  {
  }

  void noteOn (double t, double level) {
    // don't start again if already playing
    cout << "note pending on" << endl;
    noteOnPending  = true;
  }

  void noteOff (double t) {
    if (verbose)
      cout << "Note off at " << t << endl;
    releasePending = true;
  }

  double getLevel(double t)
  {
    if (noteOnPending) {
      noteOnPending = false;
      if (tStarted == -1) {
	cout << "note on" << endl;
	tStarted = t;
	tReleased = -1;
	releasePending = false;
      }
      else if (tReleased>0) {
	// this is the tricky bit
	double level = getLevel(t);
	// now what t would produce this level in the attack phase?
	double proportion = exponentialLookUpTable.getInverseValueConcaveIncreasing(level);
	double tRelativeToStart = proportion*attack->getLevel();
	tStarted = t-tRelativeToStart;
	tReleased = -1;
	releasePending = false;
	cout << "Releasing at level "<<level
	     << ", proportion " << proportion
	     << ", relative to start is " << tRelativeToStart
	     << ", t="<< t
	     << ", new tStarted=" << tStarted <<endl;
	
	{
	  double xval = tRelativeToStart/attack->getLevel();
	  double level = exponentialLookUpTable.getValueConcaveIncreasing(xval);
	  cout << xval << " " <<  level << " "<< getLevel(t) << endl;
	  
	}
      }
      
    }
    
    if (tStarted == -1)
      return 0.0;
    if (releasePending) {
      tReleased = t;
      releasePending = false;
    }
    
    double level = 1.0;
    double tRelativeToStart = t-tStarted;
    if (tRelativeToStart <0) {
      cerr << "Time issue in Envelope::getLevel" << endl;
      cerr << t << " " << tRelativeToStart << " " << tStarted << endl;
      //      throw "Time is going backward?";
    }

    // need to make this release properly
    if (tRelativeToStart<attack->getLevel() && tReleased == -1.0){
      double xval = tRelativeToStart/attack->getLevel();
      level = level * exponentialLookUpTable.getValueConcaveIncreasing(xval);
      lastLevelBeforeRelease = level;
    }
    else if (tRelativeToStart < attack->getLevel()+decay->getLevel() && tReleased == -1.0) {
      double xval = (tRelativeToStart-attack->getLevel())/decay->getLevel();
      level = level * sustain->getLevel() + level*(1.0-sustain->getLevel())*exponentialLookUpTable.getValueConcaveDecreasing(xval);
      lastLevelBeforeRelease = level;
    }
    else if (tReleased == -1.0) {
      level = level * sustain->getLevel();
      lastLevelBeforeRelease = level;
    }
    else {
      double xval =  (t-tReleased)/release->getLevel();
      if (xval >=1.0) {
	level = 0.0;
	tStarted = -1;
      }
      else
	level = level * lastLevelBeforeRelease*exponentialLookUpTable.getValueConcaveDecreasing(xval);
    }
    tStored = t;
    levelStored = level;
    if (verbose)
      envelopelog << t << " " << level << endl;
    return level;
  }

private:
  bool releasePending;
  bool noteOnPending;

  double tStarted;
  double tReleased;
  double lastLevelBeforeRelease;
  double internalLevel;
  double tStored;
  double levelStored;
};

typedef shared_ptr<Envelope> REnvelope;
