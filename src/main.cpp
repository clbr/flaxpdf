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

Fl_Double_Window *win = (Fl_Double_Window *) 0;
static Fl_Pack *buttons = (Fl_Pack *) 0;
Fl_Box *pagectr = (Fl_Box *) 0;
Fl_Input_Choice *zoombar = (Fl_Input_Choice *) 0;
Fl_Light_Button *selecting = NULL;

static Fl_Menu_Item menu_zoombar[] = {
 {_("Trimmed"), 0,	0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("Width"), 0,	0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("Page"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
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

static void goto_page(Fl_Input*, void*) {

}

int main(int argc, char **argv) {

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
		{ Fl_Input *o = new Fl_Input(0, 64, 64, 32);
			o->value("0");
			o->callback((Fl_Callback*)goto_page);
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
		{ Fl_Button* o = new Fl_Button(0, 160, 64, 64, _("Zoom in"));
			o->tooltip(_("Zoom in"));
			o->callback((Fl_Callback*)cb_Zoomin);
		} // Fl_Button* o
		{ Fl_Button* o = new Fl_Button(0, 224, 64, 64, _("Zoom out"));
			o->tooltip(_("Zoom out"));
			o->callback((Fl_Callback*)cb_Zoomout);
		} // Fl_Button* o
		{ selecting = new Fl_Light_Button(0, 288, 64, 64, _("Select text"));
			selecting->tooltip(_("Select text"));
		} // Fl_Light_Button* o
		buttons->end();
		buttons->spacing(4);
		buttons->show();
	} // Fl_Pack* buttons
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

	return Fl::run();
}
