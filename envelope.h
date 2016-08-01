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

#pragma once
#include "buffer.h"
#include "jabVector.h"
#include <fstream>
#include "controller.h"
#include "boost/thread/mutex.hpp"

using namespace jab;
using namespace std;

enum EnvelopePhase {
  EP_Silent,
  EP_Attack,
  EP_Decay,
  EP_Sustain,
  EP_Release,
  EP_PreAttackFastRelease
};

//extern ofstream envelopelog;

// Envelopes are a bit of a mess, sorry!
//
// Just thinking about this, the real way envelopes should work is
// that they have an internal level and a target level and a rate to
// get to the target.
// 
// Then attack/decay/sustain/release just set this appropriately.

#if 0

class Envelope
{
 public:
  // Use an id to make the envelope trackable for debugging
  int id;
  bool verbose;

  // controllers for the different parameters of the envelope
  RController attack;
  RController decay;
  RController sustain;
  RController release;

  Envelope (RController a,RController d,RController s,RController r);
 
  void noteOn (double t, double level);
  void noteOff (double t);
  bool isPlaying ();

  double getLevel(double t);
  
  // Because many different partials use the same envelope, I can
  // speed things up by buffering envelope values
  void prepareBuffer(double start, double end, double inc);
  double getBufferedLevel(double t);

 private:

  double level;
  double noteLevel;

  // Whether it is in the a/d/s/r phase...
  EnvelopePhase phase;

  // each event (note on, note off, phase transition) updates the
  // previous and next event levels

  double tPreviousEvent;
  double tNextEvent;

  double previousLevel;
  double targetLevel;

  // for envelope ids
  static int env_id;
  static boost::mutex mx;

  // for the internal buffer
  double bufferStart;
  double bufferInc;
  safeVector<double> buffer;

  
};
#else

class Envelope
{
 public:
  // these need to become controlled
  RController attack;
  RController decay;
  RController sustain;
  RController release;
  bool verbose;
  int id;

  Envelope (RController a,RController d,RController s,RController r);
 
  void noteOn (double t, double level);
  void noteOff (double t);
  bool isPlaying ();

  double getLevel(double t);
  double getBufferedLevel(double t);

  void prepareBuffer(double start, double end, double inc);

private:
  bool releasePending;
  bool noteOnPending;

  double tStarted;
  double tReleased;
  double lastLevelBeforeRelease;
  double internalLevel;
  double tStored;
  double levelStored;
  
  int phase;
  
  static int env_id;
  static boost::mutex mx;
  
  double bufferStart;
  double bufferInc;
  safeVector<double> buffer;
};

#endif

typedef shared_ptr<Envelope> REnvelope;
