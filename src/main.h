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

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lzo/lzo1x.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/x.H>

#include <PDFDoc.h>

#include "autoconfig.h"
#include "gettext.h"
#include "lrtypes.h"
#include "macros.h"
#include "helpers.h"
#include "view.h"
#include "config.h"

extern Fl_Double_Window *win;
extern Fl_Box *pagectr;
extern Fl_Input *pagebox;
extern Fl_Input_Choice *zoombar;
extern Fl_Light_Button *selecting;
extern Fl_Box *debug1, *debug2, *debug3, *debug4, *debug5, *debug6, *debug7;
extern u8 details;

extern int writepipe;

void debug(Fl_Box * ctrl, const float value, const char * hint);
void debug(Fl_Box * ctrl, const s32   value, const char * hint);
void debug(Fl_Box * ctrl, const u32   value, const char * hint);

bool loadfile(const char *, recent_file_struct *recent_files);

const int MAX_COLUMNS_COUNT = 5;

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
	Z_WIDTH,
	Z_PAGE,
	Z_PGTRIM,
	Z_CUSTOM
};

enum msg {
	MSG_REFRESH = 0,
	MSG_READY
};

struct openfile {
	char * filename;
	cachedpage *cache;
	PDFDoc *pdf;
	u32 maxw, maxh;

	u32 pages;

	u32 first_visible;
	u32 last_visible;

	float zoom;
	zoommode mode;
	pthread_t tid;
};

extern openfile *file;

void cb_Zoomin(Fl_Button*, void*);
void cb_Zoomout(Fl_Button*, void*);
void cb_hide(Fl_Widget*, void*);

#endif
