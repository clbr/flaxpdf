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

Fl_Double_Window *win = (Fl_Double_Window *) 0;
static Fl_Pack *buttons = (Fl_Pack *) 0;
Fl_Box *pagectr = (Fl_Box *) 0;
Fl_Input_Choice *zoombar = (Fl_Input_Choice *) 0;

static Fl_Menu_Item menu_zoombar[] = {
 {_("Trimmed"), 0,	0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("Width"), 0,	0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("Page"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

static void cb_Open(Fl_Button*, void*) {
//	hi();
}

static void cb_zoombar(Fl_Input_Choice*, void*) {
//	hi();
}

static void cb_Zoom(Fl_Button*, void*) {
//	hi();
}

static void cb_Zoom1(Fl_Button*, void*) {
//	hi();
}

int main(int argc, char **argv) {

	Fl::scheme("gtk+");

	win = new Fl_Double_Window(705, 700, _("FlaxPDF"));
	Fl_Pack* o = new Fl_Pack(0, 0, 705, 700);
	o->type(1);
	{ buttons = new Fl_Pack(0, -4, 64, 704);
		{ Fl_Button* o = new Fl_Button(0, 0, 64, 64, _("Open"));
			o->tooltip(_("Open a new file"));
			o->callback((Fl_Callback*)cb_Open);
		} // Fl_Button* o
		{ pagectr = new Fl_Box(0, 64, 64, 64, _("0 / 0"));
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
			o->callback((Fl_Callback*)cb_Zoom);
		} // Fl_Button* o
		{ Fl_Button* o = new Fl_Button(0, 224, 64, 64, _("Zoom out"));
			o->tooltip(_("Zoom out"));
			o->callback((Fl_Callback*)cb_Zoom1);
		} // Fl_Button* o
		{ Fl_Light_Button* o = new Fl_Light_Button(0, 288, 64, 64, _("Select text"));
			o->tooltip(_("Select text"));
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
	win->show();

	return Fl::run();
}
