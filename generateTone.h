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

#include <list>
#include <map>
#include <memory>
#include <fstream>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/thread.hpp>
#include "limits.h"
#include "jabVector.h"
//#include "counted.h"
#include "envelope.h"

#include "controller.h"
#include "oscillator.h"
#include "bcr2000Driver.h"




// to do
//
// envelopes X
// amplitude oscillators
// buttons on bcr changing modes (easy version)
// different wave shapes
// saving controller sets
// morphing between controller sets
// oscillating between controller sets
// more partials
// get phone keyboard working
// midi back to bcr
// fine tuning partial freq
// tidy code into separate files
// polyphony?
// wav out?
// vst?
// upload to git
// show stuff on the screen






