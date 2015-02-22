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

pdfview::pdfview(int x, int y, int w, int h): Fl_Widget(x, y, w, h),
		xoff(0), yoff(0) {

	cachedsize = 7 * 1024 * 1024;

	u32 i;
	for (i = 0; i < CACHE_MAX; i++) {
		cache[i] = (u8 *) xcalloc(cachedsize, 1);
		cachedpage[i] = USHRT_MAX;
	}
}

void pdfview::draw() {
	int X, Y, W, H;
	fl_clip_box(x(), y(), w(), h(), X, Y, W, H);
	fl_rectf(X, Y, W, H, FL_GRAY + 1);

	const Fl_Color pagecol = FL_WHITE;

	// From the current zoom mode and view offset, update the visible page info

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

u8 pdfview::iscached(const u32 page) const {
	u32 i;
	for (i = 0; i < CACHE_MAX; i++) {
		if (cachedpage[i] == page)
			return i;
	}

	return UCHAR_MAX;
}

void pdfview::docache(const u32 page) {

	// Insert it to cache. Pick the slot at random.
	u32 i;

	if (file->cache[page].uncompressed > cachedsize) {
		cachedsize = file->cache[page].uncompressed;

		for (i = 0; i < CACHE_MAX; i++) {
			cache[i] = (u8 *) realloc(cache[i], cachedsize);
		}
	}

	// Be safe
	if (!file->cache[page].ready)
		return;

	const u32 dst = rand() % CACHE_MAX;

	lzo_uint dstsize = cachedsize;
	const int ret = lzo1x_decompress(file->cache[page].data,
					file->cache[page].size,
					cache[dst],
					&dstsize,
					NULL);
	if (ret != LZO_E_OK || dstsize != file->cache[page].uncompressed)
		die(_("Error decompressing\n"));

	cachedpage[dst] = page;
}
