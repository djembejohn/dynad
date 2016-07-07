// This file is part of Dynad.

//    Dynad is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    Dynad is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with Dynad.  If not, see <http://www.gnu.org/licenses/>.

#include "synthesiser.h"


PolySynthesiser::PolySynthesiser (int _numPartials, int _numPhones) 
  : numPartials(_numPartials), numPhones(_numPhones),
    controlSet (RControllerSet (new ControllerSet(numPartials))),  
    fx(controlSet), recording("recording")
{


  RController  volume = controlSet->getController("masterVolume");

  
  for (int i = 0; i<numPhones; i++) {
    
    RPhone phone (new Phone (controlSet, volume,numPartials));
    phones.push_back(phone);
    
    //    if (i == 0)
    //      phone->phoneEnvelope->verbose = true;
    timePhoneStarted.push_back(0.0);
  }
  notesOn.resize(256,0);
}


void PolySynthesiser::save(string filename) {
  lock();
  cout << "Saving to " << filename << endl;
  controlSet->save(filename);
  unlock();
}

void PolySynthesiser::load(string filename) {
  lock();
  cout << "Loading from " << filename << endl;
  controlSet->load(filename);
  morphSet = 0;
  bcr2000Driver.updateKnobsFromControlSet (controlSet);
  bcr2000Driver.initialiseBCR();
  unlock();
}

void PolySynthesiser::updateController() {
  bcr2000Driver.updateKnobsFromControlSet (controlSet);
  bcr2000Driver.initialiseBCR();
}

void PolySynthesiser::loadMorph(string fname1, string fname2) {
  lock();
  morphSet = RMorphControllerSet(new MorphControllerSet);
  morphSet->outputSet = controlSet;
  morphSet->inputSet1 = RControllerSet(new ControllerSet(numPartials));
  morphSet->inputSet2 = RControllerSet(new ControllerSet(numPartials));
  morphSet->inputSet1->load (fname1);
  morphSet->inputSet2->load (fname2);
  morphSet->initControlVectors();
  unlock();
}


void PolySynthesiser::interpretControlMessage (KnobController con, int val) {
  lock();
  if (morphSet) if (morphSet->cvar_morphValue == con) {
      morphSet->morphOutput ((double)val/127.0);
  }
  RController controller = controlSet->getController(con);
  if (controller) 
    controller->receiveMidiControlMessage (val); 
  unlock();
}

const vector<RPhone> & PolySynthesiser::getPhones() {
  return phones;
}

void PolySynthesiser::playToBuffer (RBufferWithMutex buffer) {
  throw ("No longer implemented");
  #if 0
  lock();
  t = buffer->timeStart;
  if (t<0)
    cout << t << endl;

  for (auto phone:phones)
    phone->playToBuffer(buffer);
  
  //    t += (double)buffer.getNumFrames()/(double)buffer.getSampleRate();

  //  float * outbuf = buffer->getOutBuf();
  //  for (unsigned int i = 0; i< buffer->numberOfFrames*buffer->numberOfChannels; i++) {
  //    recording << outbuf[i] << endl;
  //  }
  
  unlock();
  
  
#endif
}

void PolySynthesiser::noteOn (int noteVal, int velocity, double timePoint) {
  //  cout << "note on" << endl;
  if (noteVal < 0 || noteVal > 255) {
    return;
  }
  lock();
  if (!notesOn[noteVal])    {
    
    double earliest = 1e9;
    int phoneIndex = -1;
    for (size_t i = 0;i<timePhoneStarted.size(); i++) {
      //      cout << timePhoneStarted[i] << " ";
      double thistime = timePhoneStarted[i];
      if (noteBeingPlayed[ phones[i] ] >0) {
	thistime += 1e8;
      }
      if (thistime<earliest) {
	earliest = thistime;
	phoneIndex = i;
      }
    }
    //    cout << endl << "Playing on: " << phoneIndex << endl;
    
    RPhone phone = phones[phoneIndex];
    
    notesOn[noteBeingPlayed[phone]] = 0;
    
    notesOn[noteVal] = phone;
    noteBeingPlayed[phone] = noteVal;
    timePhoneStarted[phoneIndex] = timePoint;
    
    phone->phoneEnvelope->noteOn(timePoint, (double)velocity/127.0);
    phone->masterNote->receiveMidiControlMessage(noteVal);
  }
  unlock();
}

void PolySynthesiser::noteOff (int noteVal, int velocity, double timePoint) {
  if (noteVal <0 || noteVal > 255)
    return;
  lock();
  if (notesOn[noteVal]) {
    RPhone phone = notesOn[noteVal];
    noteBeingPlayed[phone] = 0;
    phone->phoneEnvelope->noteOff(timePoint);
    notesOn[noteVal] = 0;
  }
  unlock();
}
