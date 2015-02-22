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

#include "view.h"

pdfview::pdfview(int x, int y, int w, int h): Fl_Widget(x, y, w, h) {

}

void pdfview::draw() {

}

int pdfview::handle(int e) {

	switch (e) {
		case FL_PUSH:
		case FL_FOCUS:
			return 1;
		break;
		case FL_DRAG:
			// TODO
		break;
		case FL_MOUSEWHEEL:
			// TODO
		break;
		case FL_KEYDOWN:
		case FL_SHORTCUT:
			// TODO
		break;
	}

	return Fl_Widget::handle(e);
}
