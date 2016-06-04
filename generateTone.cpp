#include "generateTone.h"


double midiNoteToFrequency (int note)
{
  return 440* pow (2.0, (double(note)-69.0)/12);
}

int stringToMidiNote (string note)
{
  int n = 0;
  switch (note[0]) {
  case 'a':
    n = 9;
    break;
  case 'b':
    n= 11;
    break;
  case 'c':
    n=0;
    break;
  case 'd':
    n=2;
    break;
  case 'e':
    n=4;
    break;
  case 'f':
    n=5;
    break;
  case 'g':
    n=7;
    break;
  }

  size_t ncounter = 1;
  if (note[1] == 'b') {
    n -= 1;
    ncounter += 1;
  }
  if (note[1] == '#') {
    n += 1;
    ncounter += 1;
  }

  int octave = stoi (note.substr(ncounter));

  n += octave * 12;
  return n;
 
}

double stringToFrequency(string str)
{
  int n = stringToMidiNote(str);
  double frequency = midiNoteToFrequency(n);
  //  cout << note << " " << frequency << endl;
  return frequency;
}

bool operator < (KnobController const& lhs, KnobController const& rhs)
{
  if (lhs.column < rhs.column)
    return true;
  if (lhs.column == rhs.column)
    return lhs.knobNumber < rhs.knobNumber;
  return false;
}
