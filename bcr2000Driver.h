#pragma once
#include "controller.h"
#include "RtMidi.h"
#include "generateTone.h"
#include "synthesiser.h"

typedef shared_ptr<class PolySynthesiser> RPolySynthesiser;

class BCR2000Driver
{
public:

  BCR2000Driver();
  void setMidiOut (shared_ptr<RtMidiOut> _midiOut);
  void changeSetNumber (ControlVariable cvar, int byte2);
  void outputSetNumber ();
  void changeColumnMode (ControlVariable cvar, int byte2);
  void outputColumnMode (int columnNumber);
  void updateColumnKnobs (int columnNumber);
  void interpretControlMessage (ControlVariable cvar, int byte2, RPolySynthesiser synth);
  void initialiseBCR();
  void updateKnobsFromControlSet(RControllerSet controlSet);
  void setColumnKnobValue(KnobController kc, int value);
  
  int setNumber;

  shared_ptr<RtMidiOut> midiOut;
  
  vector <int> columnModes;

  vector < vector <int> > columnKnobs;

  vector <unsigned int> ccKnobNumbers;

  vector <bool> topRowKnobMode;

};

extern BCR2000Driver bcr2000Driver;
