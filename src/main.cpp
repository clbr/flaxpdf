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

#include "main.h"
#include "wmicon.h"
//#include "icons.h"
#include "icons 32x32.h"
#include <FL/Fl_File_Icon.H>
#include <getopt.h>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace std;
using namespace libconfig;

Fl_Double_Window *win = NULL;
static Fl_Pack* win_pack = NULL;
static Fl_Pack *buttons = NULL;
static Fl_Pack *v = NULL;
static Fl_Button *showbtn = NULL;
static Fl_Button *fullscreenbtn = NULL;

static Fl_Scrollbar * vertical_scrollbar = NULL;

static Fl_PNG_Image *fullscreen_image = NULL;
static Fl_PNG_Image *fullscreenreverse_image = NULL;

Fl_Box *pagectr = (Fl_Box *) 0;
Fl_Box *debug1 = (Fl_Box *) 0,
       *debug2 = (Fl_Box *) 0,
       *debug3 = (Fl_Box *) 0,
       *debug4 = (Fl_Box *) 0,
       *debug5 = (Fl_Box *) 0,
       *debug6 = (Fl_Box *) 0,
       *debug7 = (Fl_Box *) 0;

Fl_Input *pagebox = NULL;
Fl_Input_Choice *zoombar = NULL;
Fl_Choice *columns = NULL;
Fl_Light_Button *selecting = NULL;

Fl_Window *recent_win = NULL;
Fl_Select_Browser *recent_select = NULL;

pdfview *view = NULL;

int writepipe;
bool fullscreen;

u8 details = 0;
openfile *file = NULL;

static Fl_Menu_Item menu_zoombar[] = {
	{"Trim",  0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Width", 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Page",  0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{0,0,0,0,0,0,0,0,0}
};

static Fl_Menu_Item menu_columns[] = {
	{"1 Col.", 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"2 Col.", 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"3 Col.", 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"4 Col.", 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"5 Col.", 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{0,0,0,0,0,0,0,0,0}
};

void debug(Fl_Box * ctrl, const float value, const char * hint) {

  char tmp[20];

  snprintf(tmp, 20, "%4.2f", value);
  ctrl->copy_label(tmp);
  ctrl->tooltip(hint);
  ctrl->redraw_label();
}

void cb_fullscreen(Fl_Button*, void*) {
	if (fullscreen) {
		fullscreen = false;
		fullscreenbtn->image(fullscreen_image);
		fullscreenbtn->tooltip(_("Enter Full Screen"));
		win->fullscreen_off();
	}
	else {
		fullscreen = true;
		fullscreenbtn->image(fullscreenreverse_image);
		fullscreenbtn->tooltip(_("Exit Full Screen"));
		win->fullscreen();
	}
}

static void cb_Open(Fl_Button*, void*) {
	loadfile(NULL, NULL);
}

static void display_zoom(const float what)
{
	char tmp[10];
	snprintf(tmp, 10, "%.0f%%", what * 100);
	zoombar->value(tmp);
}

static void applyzoom(const float what) {
	file->zoom = what > 0.01f ? what : 0.01f;
	file->mode = Z_CUSTOM;

	display_zoom(what);

	view->resetselection();
	view->redraw();
}

static void cb_zoombar(Fl_Input_Choice *w, void*) {
	const char * const val = w->value();
	if (isdigit(val[0])) {
		const float f = atof(val) / 100;
		applyzoom(f);
	} else {
		if (!strcmp(val, menu_zoombar[0].text)) {
			file->mode = Z_TRIM;
		} else if (!strcmp(val, menu_zoombar[1].text)) {
			file->mode = Z_WIDTH;
		} else if (!strcmp(val, menu_zoombar[2].text)) {
			file->mode = Z_PAGE;
		} else {
			//fl_alert(_("Unrecognized zoom level"));
		}
	}

	view->redraw();
}

static void adjust_display_from_recent(recent_file_struct &recent)
{
	view->set_params(recent.columns, recent.xoff, recent.yoff);
	fullscreen = recent.fullscreen;
	cb_fullscreen(NULL, NULL);
	if (recent.zoom_mode == Z_CUSTOM)
		display_zoom(recent.zoom);
	else
		zoombar->value(menu_zoombar[recent.zoom_mode].text);
	win->damage(FL_DAMAGE_ALL);
	win->flush();
	win->position(recent.x, recent.y);
	win->size(recent.width, recent.height);
}

void cb_recent_select(Fl_Select_Browser *, void *) {

	recent_win->hide();

	int i = recent_select->value();

	fl_message("Selected: %d", i);

	if (i <= 0) return;

	i -= 1;

	recent_file_struct *rf = recent_files;
	while ((i > 0) && (rf != NULL)) {
 		rf = rf->next;
 		i -= 1;
	}

	if ((rf != NULL) && loadfile(NULL, rf))
		adjust_display_from_recent(*rf);
}

static void cb_OpenRecent(Fl_Button*, void*) {

	if (recent_win == NULL){
		recent_win = new Fl_Window(10, 10, 250, 180, "Recent Files");
		Fl_Pack *p = new Fl_Pack(10, 10, 230, 160);
		p->type(1);
		{
			recent_select = new Fl_Select_Browser(0, 0, 230, 160);
			recent_select->callback((Fl_Callback*)cb_recent_select);
			recent_select->resizable();
			recent_select->show();
		}
		p->resizable();
	}
	else {
		recent_select->clear();
	}

	recent_file_struct *rf = recent_files;
	while (rf != NULL) {
		recent_select->add(rf->filename.c_str());
		rf = rf->next;
	}

	recent_win->set_modal();
	recent_win->show();
}

static void cb_columns(Fl_Choice *w, void*) {
	const int idx = w->value();
	view->set_columns(idx + 1);
}

void cb_page_up(Fl_Button*, void*) {
	Fl::e_keysym = FL_Page_Up;
	Fl::handle_(FL_KEYDOWN, win);
	Fl::handle_(FL_KEYUP, win);
}

void cb_page_down(Fl_Button*, void*) {
	Fl::e_keysym = FL_Page_Down;
	Fl::handle_(FL_KEYDOWN, win);
	Fl::handle_(FL_KEYUP, win);
}

void cb_Zoomin(Fl_Button*, void*) {
	file->zoom *= 1.2f;
	applyzoom(file->zoom);
}

void cb_Zoomout(Fl_Button*, void*) {
	file->zoom *= 0.833333;
	applyzoom(file->zoom);
}

void cb_hide(Fl_Widget*, void*) {

	if (buttons->visible()) {
		buttons->hide();
		showbtn->show();
	} else {
		showbtn->hide();
		buttons->show();
	}
}

void cb_vertical_scrollbar(Fl_Widget*, void*) {
	view->update_position(vertical_scrollbar->value());
}

void cb_exit(Fl_Widget*, void*) {
	save_to_config(
		file->filename,
		view->get_columns(),
		view->get_xoff(),
		view->get_yoff(),
		file->zoom,
		file->mode,
		win->x(),
		win->y(),
		win->w(),
		win->h(),
		fullscreen);
	save_config();
	exit(0);
}

static void goto_page(Fl_Input *w, void*) {
	const u32 which = atoi(w->value()) - 1;
	if (which >= file->pages) {
		fl_alert(_("Page out of bounds"));
		return;
	}

	view->go(which);
}

static void reader(FL_SOCKET fd, void*) {

	// A thread has something to say to the main thread.
	u8 buf;
	sread(fd, &buf, 1);

	switch (buf) {
		case MSG_REFRESH:
			view->redraw();
		break;
		case MSG_READY:
			fl_cursor(FL_CURSOR_DEFAULT);
		break;
		default:
			die(_("Unrecognized thread message\n"));
	}
}

static void checkX() {
	// Make sure everything's cool
	if (fl_visual->red_mask != 0xff0000 ||
		fl_visual->green_mask != 0xff00 ||
		fl_visual->blue_mask != 0xff)
		die(_("Visual doesn't match our expectations\n"));

	int a, b;
	if (!XRenderQueryExtension(fl_display, &a, &b))
		die(_("No XRender on this server\n"));
}

static void selecting_changed(Fl_Widget *, void *) {
	view->resetselection();
	view->redraw();
}

int main(int argc, char **argv) {

	srand(time(NULL));

	#if ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain("flaxpdf", LOCALEDIR);
	textdomain("flaxpdf");
	#endif

	const struct option opts[] = {
		{ "details", 0, NULL, 'd' },
		{ "help",    0, NULL, 'h' },
		{ "version", 0, NULL, 'v' },
		{ NULL,      0, NULL,  0  }
	};

	while (1) {
		const int c = getopt_long(argc, argv, "dhv", opts, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'd':
				details++;
			break;
			case 'v':
				printf("%s\n", PACKAGE_STRING);
				return 0;
			break;
			case 'h':
			default:
				printf(_("Usage: %s [options] file.pdf\n\n"
					"	-d --details	Print RAM, timing details (use twice for more)\n"
					"	-h --help	This help\n"
					"	-v --version	Print version\n"),
					argv[0]);
				return 0;
			break;
		}
	}

	Fl::scheme("gtk+");
	Fl_File_Icon::load_system_icons();

	file = (openfile *) xcalloc(1, sizeof(openfile));
	int ptmp[2];
	if (pipe(ptmp))
		die(_("Failed in pipe()\n"));
	writepipe = ptmp[1];

	Fl::add_fd(ptmp[0], FL_READ, reader);

	#define img(a) a, sizeof(a)

	fullscreen_image = new Fl_PNG_Image("fullscreen.png", img(view_fullscreen_png));
	fullscreenreverse_image = new Fl_PNG_Image("fullscreenreverse.png", img(view_restore_png));

	fullscreen = false;

	int pos;

	win = new Fl_Double_Window(700, 700, "FlaxPDF");
	win_pack = new Fl_Pack(0, 0, 700, 700);
	win_pack->type(1);

	{ buttons = new Fl_Pack(0, 0, 64, 700);
		{ Fl_Button* o = new Fl_Button(0, pos = 0, 64, 32);
			o->tooltip(_("Hide toolbar (F8)"));
			o->callback(cb_hide);
			o->image(new Fl_PNG_Image("back.png", img(media_seek_backward_png)));
		} // Fl_Button* o
		{ Fl_Button* o = new Fl_Button(0, pos += 32, 64, 48);
			o->tooltip(_("Open a new file"));
			o->callback((Fl_Callback*)cb_Open);
			o->image(new Fl_PNG_Image("fileopen.png", img(document_open_png)));
		} // Fl_Button* o
		{ Fl_Button* o = new Fl_Button(0, pos += 48, 64, 48);
			o->tooltip(_("Open a recent file"));
			o->callback((Fl_Callback*)cb_OpenRecent);
			o->image(new Fl_PNG_Image("fileopen.png", img(emblem_documents_png)));
		} // Fl_Button* o
		{ pagebox = new Fl_Input(0, pos += 48, 64, 24);
			pagebox->value("0");
			pagebox->callback((Fl_Callback*)goto_page);
			pagebox->when(FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED);
		} // Fl_Box* pagectr
		{ pagectr = new Fl_Box(0, pos += 24, 64, 24, "/ 0");
			pagectr->tooltip(_("Document Page Count"));
			pagectr->box(FL_ENGRAVED_FRAME);
			pagectr->align(FL_ALIGN_WRAP);
		} // Fl_Box* pagectr
		{ zoombar = new Fl_Input_Choice(0, pos += 24, 64, 24);
			zoombar->tooltip(_("Preset zoom choices"));
			zoombar->callback((Fl_Callback*)cb_zoombar);
			zoombar->menu(menu_zoombar);
			zoombar->value(0);
			zoombar->textsize(11);
			zoombar->input()->when(FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED);
		} // Fl_Input_Choice* zoombar
		{ Fl_Pack* zooms = new Fl_Pack(0, pos += 24, 64, 32);
			zooms->type(Fl_Pack::HORIZONTAL);
			{ Fl_Button* o = new Fl_Button(0, 0, 32, 32);
				o->tooltip(_("Zoom in"));
				o->callback((Fl_Callback*)cb_Zoomin);
				o->image(new Fl_PNG_Image("zoomin.png", img(zoom_in_png)));
			} // Fl_Button* o
			{ Fl_Button* o = new Fl_Button(32, 0, 32, 32);
				o->tooltip(_("Zoom out"));
				o->callback((Fl_Callback*)cb_Zoomout);
				o->image(new Fl_PNG_Image("zoomout.png", img(zoom_out_png)));
			} // Fl_Button* o
			zooms->end();
			zooms->show();
		} // Fl_Pack* zooms
		{ selecting = new Fl_Light_Button(0, pos += 32, 64, 38);
			selecting->tooltip(_("Select text"));
			selecting->align(FL_ALIGN_CENTER);
			selecting->image(new Fl_PNG_Image("text.png", img(edit_select_all_png)));
			selecting->callback(selecting_changed);
		} // Fl_Light_Button* selecting

		{ columns = new Fl_Choice(0, pos += 38, 64, 24);
			columns->tooltip(_("Number of Columns"));
			columns->callback((Fl_Callback*)cb_columns);
			columns->menu(menu_columns);
			columns->value(0);
			columns->textsize(11);
			columns->align(FL_ALIGN_CENTER);
			//columns->input()->when(FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED);
		} // Fl_Input_Choice* zoombar
		{ Fl_Pack* page_moves = new Fl_Pack(0, pos += 24, 64, 42);
			page_moves->type(Fl_Pack::HORIZONTAL);
			{ Fl_Button* o = new Fl_Button(0, 0, 32, 42);
				o->tooltip(_("Page Up"));
				o->callback((Fl_Callback*)cb_page_up);
				o->image(new Fl_PNG_Image("pageup.png", img(go_up_png)));
			} // Fl_Button* o
			{ Fl_Button* o = new Fl_Button(32, 0, 32, 42);
				o->tooltip(_("Page Down"));
				o->callback((Fl_Callback*)cb_page_down);
				o->image(new Fl_PNG_Image("pagedown.png", img(go_down_png)));
			} // Fl_Button* o
			page_moves->end();
			page_moves->show();
		} // Fl_Pack* page_moves
		{ fullscreenbtn = new Fl_Button(0, pos += 42, 64, 38);
			fullscreenbtn->image(fullscreen_image);
			fullscreenbtn->callback((Fl_Callback*)cb_fullscreen);
			fullscreenbtn->tooltip(_("Enter Full Screen"));
		}
		{ debug1 = new Fl_Box(0, pos += 38, 64, 32, "");
			debug1->box(FL_ENGRAVED_FRAME);
			debug1->align(FL_ALIGN_WRAP);
		} // Fl_Box* debug1
		{ debug2 = new Fl_Box(0, pos += 32, 64, 32, "");
			debug2->box(FL_ENGRAVED_FRAME);
			debug2->align(FL_ALIGN_WRAP);
		} // Fl_Box* debug2
		{ debug3 = new Fl_Box(0, pos += 32, 64, 32, "");
			debug3->box(FL_ENGRAVED_FRAME);
			debug3->align(FL_ALIGN_WRAP);
		} // Fl_Box* debug3
		{ debug4 = new Fl_Box(0, pos += 32, 64, 32, "");
			debug4->box(FL_ENGRAVED_FRAME);
			debug4->align(FL_ALIGN_WRAP);
		} // Fl_Box* debug4
		{ debug5 = new Fl_Box(0, pos += 32, 64, 32, "");
			debug5->box(FL_ENGRAVED_FRAME);
			debug5->align(FL_ALIGN_WRAP);
		} // Fl_Box* debug5
		{ debug6 = new Fl_Box(0, pos += 32, 64, 32, "");
			debug6->box(FL_ENGRAVED_FRAME);
			debug6->align(FL_ALIGN_WRAP);
		} // Fl_Box* debug6
		{ debug7 = new Fl_Box(0, pos += 32, 64, 32, "");
			debug7->box(FL_ENGRAVED_FRAME);
			debug7->align(FL_ALIGN_WRAP);
		} // Fl_Box* debug7
		{ Fl_Box *fill_box = new Fl_Box(0, pos += 32, 64, 64, "");
			Fl_Group::current()->resizable(fill_box);
		}
		{ Fl_Button* exitbtn = new Fl_Button(0, 700, 64, 38);
			//exitbtn->image(fullscreen_image);
			exitbtn->callback((Fl_Callback*)cb_exit);
			exitbtn->image(new Fl_PNG_Image("exit.png", img(application_exit_png)));
			exitbtn->tooltip(_("Exit"));
		}

		buttons->end();
		buttons->spacing(4);
		buttons->show();
	} // Fl_Pack* buttons
	{ showbtn = new Fl_Button(0, 0, 6, 700);
		showbtn->hide();
		showbtn->callback(cb_hide);
	}
	{ v = new Fl_Pack(64, 0, 700-64, 700);
		v->type(Fl_Pack::HORIZONTAL);
		{ view = new pdfview(0, 0, 700-64-16, 700);
			Fl_Group::current()->resizable(view);
		}
		{ vertical_scrollbar = new Fl_Scrollbar(700-64-16, 0, 16, 700);
			vertical_scrollbar->callback((Fl_Callback*)cb_vertical_scrollbar);
		} // Fl_Box* o
		view->set_scrollbar(vertical_scrollbar);
		v->end();
		v->show();
	}

	Fl_Group::current()->resizable(v);
	win_pack->end();
	win_pack->show();

	win->resizable(win_pack);
	win->callback((Fl_Callback*)cb_exit);
	win->size_range(500, 500);
	win->end();

	Fl_PNG_Image wmicon("wmicon.png", img(wmicon_png));
	win->icon(&wmicon);

	fl_open_display();
	win->show();
	checkX();

	#undef img

	if (lzo_init() != LZO_E_OK)
		die(_("LZO init failed\n"));

	// Set the width to half of the screen, 90% of height
	//win->size(Fl::w() * 0.4f, Fl::h() * 0.9f);

	load_config();

	bool used_recent = false;

	if (optind < argc)
		used_recent = loadfile(argv[optind], recent_files);
	else
		used_recent = loadfile(NULL, recent_files);

    if (used_recent) {
    	adjust_display_from_recent(*recent_files);
    }

	view->take_focus();

	return Fl::run();
}
