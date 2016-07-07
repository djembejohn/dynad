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

//extern ofstream envelopelog;

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

typedef shared_ptr<Envelope> REnvelope;
