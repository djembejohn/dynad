#include "synthesiser.h"


PolySynthesiser::PolySynthesiser (int numPartials, int numPhones) 
  : recording("recording")
{
  controlSet = RControllerSet (new ControllerSet());
  
  masterVolume = controlSet->getController("masterVolume");

  
  for (int i = 0; i<numPhones; i++) {
    
    RPhone phone (new Phone (controlSet, masterVolume,numPartials));
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
  morphSet->inputSet1 = RControllerSet(new ControllerSet);
  morphSet->inputSet2 = RControllerSet(new ControllerSet);
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

void PolySynthesiser::playToBuffer (RBufferWithMutex buffer) {
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
  
  
}

void PolySynthesiser::noteOn (int noteVal, int velocity) {
  //  cout << "note on" << endl;
  if (noteVal < 0 || noteVal > 255) {
    return;
  }
  lock();
  if (!notesOn[noteVal])    {
    
    double earliest = 1e9;
    int phoneIndex = -1;
    for (size_t i = 0;i<timePhoneStarted.size(); i++) {
      cout << timePhoneStarted[i] << " ";
      if (timePhoneStarted[i]<earliest) {
	earliest = timePhoneStarted[i];
	phoneIndex = i;
      }
    }
    cout << phoneIndex << endl;
    
    RPhone phone = phones[phoneIndex];
    
    notesOn[noteBeingPlayed[phone]] = 0;
    
    notesOn[noteVal] = phone;
    noteBeingPlayed[phone] = noteVal;
    timePhoneStarted[phoneIndex] = t;
    
    phone->phoneEnvelope->noteOn(t, (double)velocity/127.0);
    phone->masterNote->receiveMidiControlMessage(noteVal);
  }
  unlock();
}

void PolySynthesiser::noteOff (int noteVal, int velocity) {
  if (noteVal <0 || noteVal > 255)
    return;
  lock();
  if (notesOn[noteVal]) {
    RPhone phone = notesOn[noteVal];
    noteBeingPlayed[phone] = 0;
    phone->phoneEnvelope->noteOff(t);
    notesOn[noteVal] = 0;
  }
  unlock();
}
