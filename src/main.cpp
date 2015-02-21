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
#include "icons.h"
#include <getopt.h>

Fl_Double_Window *win = (Fl_Double_Window *) 0;
static Fl_Pack *buttons = (Fl_Pack *) 0;
static Fl_Button *showbtn = NULL;
Fl_Box *pagectr = (Fl_Box *) 0;
Fl_Input *pagebox = NULL;
Fl_Input_Choice *zoombar = (Fl_Input_Choice *) 0;
Fl_Light_Button *selecting = NULL;

static Fl_Menu_Item menu_zoombar[] = {
	{_("Trimmed"), 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{_("Width"), 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{_("Page"), 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{0,0,0,0,0,0,0,0,0}
};

static void cb_Open(Fl_Button*, void*) {
}

static void cb_zoombar(Fl_Input_Choice*, void*) {
}

static void cb_Zoomin(Fl_Button*, void*) {
}

static void cb_Zoomout(Fl_Button*, void*) {
}

static void cb_hide(Fl_Widget*, void*) {

	if (buttons->visible()) {
		buttons->hide();
		showbtn->show();
	} else {
		showbtn->hide();
		buttons->show();
	}
}

static void goto_page(Fl_Input*, void*) {

}

int main(int argc, char **argv) {

	const struct option opts[] = {
		{"help", 0, NULL, 'h'},
		{"version", 0, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	while (1) {
		const int c = getopt_long(argc, argv, "hv", opts, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'v':
				printf("%s\n", PACKAGE_STRING);
				return 0;
			break;
			case 'h':
			default:
				printf("Usage: %s [options] file.pdf\n\n"
					"	-h --help	This help\n"
					"	-v --version	Print version\n",
					argv[0]);
				return 0;
			break;
		}
	}

	Fl::scheme("gtk+");

	#define img(a) a, sizeof(a)

	win = new Fl_Double_Window(705, 700, _("FlaxPDF"));
	Fl_Pack* o = new Fl_Pack(0, 0, 705, 700);
	o->type(1);
	{ buttons = new Fl_Pack(0, -4, 64, 704);
		{ Fl_Button* o = new Fl_Button(0, 0, 64, 64);
			o->tooltip(_("Open a new file"));
			o->callback((Fl_Callback*)cb_Open);
			o->image(new Fl_PNG_Image("fileopen.png", img(fileopen_png)));
		} // Fl_Button* o
		{ pagebox = new Fl_Input(0, 64, 64, 32);
			pagebox->value("0");
			pagebox->callback((Fl_Callback*)goto_page);
		} // Fl_Box* pagectr
		{ pagectr = new Fl_Box(0, 64, 64, 32, "/ 0");
			pagectr->box(FL_ENGRAVED_FRAME);
			pagectr->align(FL_ALIGN_WRAP);
		} // Fl_Box* pagectr
		{ zoombar = new Fl_Input_Choice(0, 128, 64, 32);
			zoombar->tooltip(_("Preset zoom choices"));
			zoombar->callback((Fl_Callback*)cb_zoombar);
			zoombar->menu(menu_zoombar);
			zoombar->value(0);
		} // Fl_Input_Choice* zoombar
		{ Fl_Button* o = new Fl_Button(0, 160, 64, 64);
			o->tooltip(_("Zoom in"));
			o->callback((Fl_Callback*)cb_Zoomin);
			o->image(new Fl_PNG_Image("zoomin.png", img(zoomin_png)));
		} // Fl_Button* o
		{ Fl_Button* o = new Fl_Button(0, 224, 64, 64);
			o->tooltip(_("Zoom out"));
			o->callback((Fl_Callback*)cb_Zoomout);
			o->image(new Fl_PNG_Image("zoomout.png", img(zoomout_png)));
		} // Fl_Button* o
		{ selecting = new Fl_Light_Button(0, 288, 64, 64);
			selecting->tooltip(_("Select text"));
			selecting->align(FL_ALIGN_CENTER);
			selecting->image(new Fl_PNG_Image("text.png", img(text_png)));
		} // Fl_Light_Button* o
		{ Fl_Button* o = new Fl_Button(0, 224, 64, 64);
			o->tooltip(_("Hide toolbar"));
			o->callback(cb_hide);
			o->image(new Fl_PNG_Image("back.png", img(back_png)));
		} // Fl_Button* o
		buttons->end();
		buttons->spacing(4);
		buttons->show();
	} // Fl_Pack* buttons
	{ showbtn = new Fl_Button(0, 0, 6, 700);
		showbtn->hide();
		showbtn->callback(cb_hide);
	}
	{ Fl_Box* o = new Fl_Box(64, 0, 641, 700);
		Fl_Group::current()->resizable(o);
		o->show();
	} // Fl_Box* o
	o->end();
	o->show();
	Fl_Group::current()->resizable(o);

	win->size_range(700, 700);
	win->end();

	Fl_PNG_Image wmicon("wmicon.png", img(wmicon_png));
	win->icon(&wmicon);

	#undef img

	win->show();

	if (optind < argc)
		loadfile(argv[optind]);
	else
		loadfile(NULL);

	return Fl::run();
}
