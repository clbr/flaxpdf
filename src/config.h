/*
Copyright (C) 2016 Guy Turcotte

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h++>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

using namespace std;
using namespace libconfig;

struct recent_file_struct {
	std::string filename;
	int         columns;  
	float       xoff;
	float       yoff;
	float       zoom;
	int         zoom_mode;
	int         x;        // App Window x pos
	int         y;        // App Window y pos
	int         width;    // App Windows width
	int         height;   // App Window height
	bool        fullscreen;

	struct recent_file_struct *next;
};

extern recent_file_struct *recent_files;

extern void save_to_config(
		char * filename,
		int columns,
		float xoff,
		float yoff,
		float zoom,
		int zoommode,
		int x,
		int y,
		int w,
		int h,
		bool full);

extern void load_config();
extern void save_config();

#endif

