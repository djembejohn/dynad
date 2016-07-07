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

#include "bcr2000Driver.h"

  
BCR2000Driver::BCR2000Driver()
  :setNumber(0), columnModes(32,0), 
   columnKnobs(32,vector<int> (20,0)), topRowKnobMode(8,false)
{
  vector<unsigned int> temp {102,105,106,107,102};
  ccKnobNumbers = temp;
}

void BCR2000Driver::setMidiOut (shared_ptr<RtMidiOut> _midiOut) {
    midiOut = _midiOut;
}

void BCR2000Driver::changeSetNumber (ControlVariable cvar, int byte2) {
  if (byte2 >0) {
    if (cvar.b1>89 && cvar.b1<94) 
      setNumber = cvar.b1-90;

    for (int i = 0; i<8; i++) {
      updateColumnKnobs (i+setNumber*8);
      outputColumnMode (i+setNumber*8);
    }
  }
  
  outputSetNumber();
}

void BCR2000Driver::outputSetNumber () {
  for (int i = 0; i<4; i++) {
    vector<unsigned char> message;
    message.push_back(191);
    message.push_back(i+90);
    if (i == setNumber) 
      message.push_back(127);
    else
      message.push_back(0);
    midiOut->sendMessage(&message);
  }
}


void BCR2000Driver::changeColumnMode (ControlVariable cvar, int byte2) {
  int columnNumber = cvar.b0-176+setNumber*8;
  if (cvar.b1 == 108 && byte2 == 0) {
    // pressed the up button
    columnModes[columnNumber] -= 1;
    if (columnModes[columnNumber] <0)
      columnModes[columnNumber] = 3;
    cout << "up: " << columnNumber << " " << columnModes[columnNumber] << endl;
  }
  
  if (cvar.b1 == 109 && byte2 == 0) {
    columnModes[columnNumber] += 1;
    // pressed the down button
    if (columnModes[columnNumber] >3)
      columnModes[columnNumber] = 0;
    cout << "down: " << columnNumber << " " << columnModes[columnNumber] << endl;
  }
  
  if (cvar.b1 == 108 || cvar.b1 == 109) {
    
    outputColumnMode(columnNumber);
	
    if (byte2 == 0)
	  updateColumnKnobs (columnNumber);
  }   
  
}

void BCR2000Driver::outputColumnMode (int columnNumber) {
  int topButton = 127; // off for some reason
  if (columnModes[columnNumber] & 1)
    topButton = 0;
  int bottomButton = 127; // off for some reason
  if (columnModes[columnNumber] > 1)
    bottomButton = 0;
  
  vector<unsigned char> message;
  message.push_back(columnNumber % 8 + 176);
  message.push_back(108);
  message.push_back(topButton);
  midiOut->sendMessage(&message);
  
  message.clear();
  message.push_back(columnNumber % 8 + 176);
  message.push_back(109);
  message.push_back(bottomButton);
  midiOut->sendMessage(&message);
    
}

void BCR2000Driver::updateColumnKnobs (int columnNumber) {
  
  for (unsigned int i = 0; i < 4; i++) {
    int knobNumber = columnModes[columnNumber]*ccKnobNumbers.size() + i;
    int knobValue = 0;
    if (i == 0 && topRowKnobMode[columnNumber % 8])
      knobValue = columnKnobs[columnNumber][knobNumber+4];
    else
      knobValue = columnKnobs[columnNumber][knobNumber];
    
    vector<unsigned char> message;
    message.push_back(columnNumber % 8 + 176);
    message.push_back(ccKnobNumbers[i]);
    message.push_back(knobValue);
    midiOut->sendMessage(&message);
  }
}

void BCR2000Driver::interpretControlMessage (ControlVariable cvar, int byte2, RPolySynthesiser synth) {
  if (cvar.b0 == 191 && cvar.b1 >89 && cvar.b1 < 94) {
    changeSetNumber(cvar,byte2);
  }

  if (cvar.b0 >=176 && cvar.b0 <=183) {
    if (cvar.b1 == 108 || cvar.b1 == 109) 
      changeColumnMode (cvar,byte2);
    
    // needs to intercept knob changes, and thn record them and send
    // them to different control numbers according to the column
    // mode
    int columnNumber = cvar.b0-176+setNumber*8;
    for (int k = 0; k<4; k++) {
      if (cvar.b1 == (int)ccKnobNumbers[k]) {
	int knobNumber;
	// this checks if the knob is pressed down and records it as knob4.
	if (k == 0 && topRowKnobMode[columnNumber % 8])
	  knobNumber = columnModes[columnNumber]*ccKnobNumbers.size() + 4;
	else
	  knobNumber = columnModes[columnNumber]*ccKnobNumbers.size() + k;
	
	KnobController cont (columnNumber,knobNumber);
	synth->interpretControlMessage(cont,byte2);
	
	cout << columnNumber << ", " << knobNumber << ", " << byte2 << endl;
	columnKnobs[columnNumber][knobNumber] = byte2;
      }
    }
    
    
  }
}

void BCR2000Driver::initialiseBCR() {
  for(auto & mode:columnModes) 
    mode = 0;
  setNumber = 0;
  outputSetNumber();
  for (int columnNumber = 0; columnNumber<8; columnNumber++) {
    outputColumnMode(columnNumber);
    updateColumnKnobs(columnNumber);
  }
}

void BCR2000Driver::updateKnobsFromControlSet(RControllerSet controlSet) {
  for (auto & controller:controlSet->controllers) {
    setColumnKnobValue(controller.first, controller.second->generateMidiControlMessageValue());
  }
}


void BCR2000Driver::setColumnKnobValue(KnobController kc, int value) {
  columnKnobs[kc.column][kc.knobNumber] = value;
}

