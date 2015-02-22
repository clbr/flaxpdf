/*
Copyright (C) 2015 Lauri Kasanen

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

#ifndef MAIN_H
#define MAIN_H

#include "autoconfig.h"
#include "gettext.h"
#include "lrtypes.h"
#include "macros.h"
#include "helpers.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lzo/lzo1x.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_PNG_Image.H>

#include <PDFDoc.h>

extern Fl_Double_Window *win;
extern Fl_Box *pagectr;
extern Fl_Input *pagebox;
extern Fl_Input_Choice *zoombar;
extern Fl_Light_Button *selecting;
extern u8 details;

extern int readpipe, writepipe;

void loadfile(const char *);

struct cachedpage {
	u8 *data;
	u32 size;
	u32 uncompressed;

	u32 w, h;
	u16 left, right, top, bottom;

	bool ready;
};

enum zoommode {
	Z_TRIM = 0,
	Z_PAGE,
	Z_WIDTH,
	Z_CUSTOM
};

struct openfile {
	cachedpage *cache;
	PDFDoc *pdf;
	u32 maxw;

	u32 pages;

	u32 first_visible;
	u32 last_visible;

	float zoom;
	zoommode mode;
	pthread_t tid;
};

extern openfile *file;

#endif
