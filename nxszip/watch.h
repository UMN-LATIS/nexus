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

#ifndef VCG_WATCH_H
#define VCG_WATCH_H

#ifdef WIN32
#ifndef _WINDOWS_
#define _WINSOCKAPI_
#include <windows.h>
#endif
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include <string>

class Watch {
public:
  Watch();
  void start();
  double pause();
  void unpause();
  void reset();
  double time();
  int usec();
  static std::string format(float t);
private:
  double diff();

#ifdef WIN32
  LARGE_INTEGER tstart, tend;
  LARGE_INTEGER freq;
#else
  struct timeval tstart, tend;
  struct timezone tz;
#endif
  double elapsed;
};
#endif
