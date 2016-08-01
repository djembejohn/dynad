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

  bool active;
  
  int setNumber;

  shared_ptr<RtMidiOut> midiOut;
  
  vector <int> columnModes;

  vector < vector <int> > columnKnobs;

  vector <unsigned int> ccKnobNumbers;

  vector <bool> topRowKnobMode;

};

extern BCR2000Driver bcr2000Driver;
