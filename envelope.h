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
