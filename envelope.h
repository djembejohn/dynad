#pragma once
#include "buffer.h"
#include "jabVector.h"

using namespace jab;
using namespace std;

class Envelope
{
 public:
  // these need to become controlled
  double attack;
  double decay;
  double sustain;
  double release;

  Envelope (double a,double d,double s,double r)
    : attack(a),decay(d),sustain(s),release(r)
    , tStarted(-1), tReleased(-1), internalLevel(1.0)
  {
  }

  void noteOn (double t, double level) {
    // don't start again if already playing
    if (tStarted == -1) {
      tStarted = t;
      tReleased = -1;
    }
    else if (tReleased) {
      // this is the tricky bit
      double level = getLevel(t);
      // now what t would produce this level in the attack phase?
      double proportion = exponentialLookUpTable.getInverseValueConcaveIncreasing(level);
      double tRelativeToStart = proportion*attack;
      tStarted = t-tRelativeToStart;
      tReleased = -1;
      cout << "Releasing at level "<<level
	   << ", proportion " << proportion
	   << ", relative to start is " << tRelativeToStart
	   << ", t="<< t
	   << ", new tStarted=" << tStarted <<endl;

      {
	double xval = tRelativeToStart/attack;
	double level = exponentialLookUpTable.getValueConcaveIncreasing(xval);
	cout << xval << " " <<  level << " "<< getLevel(t) << endl;
	
      }
    }
  }

  void noteOff (double t) {
    tReleased = t;
  }

  double getLevel(double t)
  {
    if (tStarted == -1)
      return 0.0;

    double level = 1.0;
    double tRelativeToStart = t-tStarted;
    if (tRelativeToStart <0) {
      cerr << "Time issue in Envelope::getLevel" << endl;
      cerr << t << " " << tRelativeToStart << " " << tStarted << endl;
      //      throw "Time is going backward?";
    }

    // need to make this release properly
    if (tRelativeToStart<attack && tReleased == -1.0){
      double xval = tRelativeToStart/attack;
      level = level * exponentialLookUpTable.getValueConcaveIncreasing(xval);
      lastLevelBeforeRelease = level;
    }
    else if (tRelativeToStart < attack+decay && tReleased == -1.0) {
      double xval = (tRelativeToStart-attack)/decay;
      level = level * sustain + level*(1.0-sustain)*exponentialLookUpTable.getValueConcaveDecreasing(xval);
      lastLevelBeforeRelease = level;
    }
    else if (tReleased == -1.0) {
      level = level * sustain;
      lastLevelBeforeRelease = level;
    }
    else {
      double xval =  (t-tReleased)/release;
      if (xval >=1.0) {
	level = 0.0;
	tStarted = -1;
      }
      else
	level = level * lastLevelBeforeRelease*exponentialLookUpTable.getValueConcaveDecreasing(xval);
    }
    return level;
  }

private:
  double tStarted;
  double tReleased;
  double lastLevelBeforeRelease;
  double internalLevel;
  bool verbose;
};

typedef shared_ptr<Envelope> REnvelope;
