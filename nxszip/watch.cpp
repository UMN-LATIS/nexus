/****************************************************************************
* VCGLib                                                            o o     *
* Visual and Computer Graphics Library                            o     o   *
*                                                                _   O  _   *
* Copyright(C) 2004                                                \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/

#include "watch.h"
#include <stdio.h>
#include <math.h>

using namespace std;

#ifdef WIN32
Watch::Watch(): elapsed(0) {
    QueryPerformanceFrequency(&freq);
}


void Watch::start(void) {
  QueryPerformanceCounter(&tstart);
  elapsed = 0;
}

double Watch::pause() {
  QueryPerformanceCounter(&tend);
  elapsed += diff();
  return (double)elapsed;
}

void Watch::unpause() {
  QueryPerformanceCounter(&tstart);
}

double Watch::time() {
  QueryPerformanceCounter(&tend);
  return (double)(elapsed + diff());
}

double Watch::diff() {
  return ((double)tend.QuadPart -
    (double)tstart.QuadPart)/
    ((double)freq.QuadPart);
}

#else

Watch::Watch(): elapsed() {}

void Watch::start() {
   gettimeofday(&tstart, &tz);
   elapsed = 0;
}

double Watch::pause() {
  gettimeofday(&tend, &tz);
  elapsed += diff();
  return (double)elapsed;
}

void Watch::unpause() {
  gettimeofday(&tstart, &tz);
}

double Watch::time() {
  gettimeofday(&tend, &tz);
  return (double)(elapsed + diff());
}

double Watch::diff() {
  double t1 =  (double)tstart.tv_sec + (double)tstart.tv_usec/(1000*1000);
  double t2 =  (double)tend.tv_sec + (double)tend.tv_usec/(1000*1000);
  return t2 - t1;
}
#endif

void Watch::reset() {
  elapsed = 0;
}

int Watch::usec() {
#ifdef WIN32
  return 0;
#else
  struct timeval ttime;
  gettimeofday(&ttime, &tz);
  return ttime.tv_usec;
#endif
}

string Watch::format(float t) {
   char tmp[256];
   int min = (int)floor(t/60.0);
   float sec = t - min*60;
   int hour = (int)floor(min/60.0);
   min -= hour * 60;

   if(hour == 0) {
      if(min == 0)
        sprintf(tmp, "%.3f sec", sec);
      else
        sprintf(tmp, "%d min %.0f sec", min, sec);
   } else
      sprintf(tmp, "%d hour %d min %.0f sec", hour, min, sec);
   return string(tmp);
}
