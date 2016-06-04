#include "synthesiser.h"


PolySynthesiser::PolySynthesiser () 
  : recording("recording")
{
  controlSet = RControllerSet (new ControllerSet());
  
  KnobController cvar_volume (16, 0);
  RController volume (new ControllableLevelLinear (0.1));
  masterVolume = volume;
  controlSet->addController (volume,16,0,"masterVolume");
  
  for (int i = 0; i<20; i++) {
    
    phones.push_back(RPhone (new Phone (controlSet, volume)));
    
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
  bcr2000Driver.updateKnobsFromControlSet (controlSet);
  bcr2000Driver.initialiseBCR();
  unlock();
}

void PolySynthesiser::interpretControlMessage (KnobController con, int val) {
  lock();
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
    
    phone->masterEnvelope->noteOn(t, (double)velocity/127.0);
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
    phone->masterEnvelope->noteOff(t);
    notesOn[noteVal] = 0;
  }
  unlock();
}
