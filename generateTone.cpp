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

bool operator == (KnobController const& lhs, KnobController const& rhs)
{
  if (lhs.column != rhs.column)
    return false;
  return lhs.knobNumber == rhs.knobNumber;
}
