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

static void updatevisible(const float yoff, const u32 w, const u32 h) {
	// From the current zoom mode and view offset, update the visible page info
	file->first_visible = yoff;
	u32 i;

	const u32 MARGIN = 36; // Quarter inch in double resolution
	const u32 maxw = file->maxw ? file->maxw : file->cache[0].w;
	const u32 maxh = file->maxh ? file->maxh : file->cache[0].h;
	const bool hasmargins = file->cache[0].left > MARGIN ||
				file->cache[0].right > MARGIN ||
				file->cache[0].top > MARGIN ||
				file->cache[0].bottom > MARGIN;
	const u32 maxwmargin = hasmargins ? maxw + MARGIN * 2 : maxw;
	const u32 fullw = file->cache[0].w + file->cache[0].left + file->cache[0].right;
	const u32 fullh = file->cache[0].h + file->cache[0].top + file->cache[0].bottom;

	const float visible = 1 - (yoff - floorf(yoff));

	u32 usedh = fullh;
	switch (file->mode) {
		case Z_TRIM:
			file->zoom = w / maxwmargin;
			usedh = maxh;
		break;
		case Z_WIDTH:
			file->zoom = w / fullw;
		break;
		case Z_PAGE:
			if (fullw > fullh) {
				file->zoom = w / fullw;
			} else {
				file->zoom = h / fullh;
			}
		break;
		case Z_CUSTOM:
		break;
	}

	const u32 zoomedmargin = MARGIN * file->zoom;
	i = file->first_visible;
	u32 tmp = visible * usedh * file->zoom;
	tmp += zoomedmargin;
	while (tmp < h) {
		tmp += usedh * file->zoom;
		tmp += zoomedmargin;
		i++;
	}
	file->last_visible = i;
}

void pdfview::draw() {

	updatevisible(yoff, w(), h());

	const Fl_Color pagecol = FL_WHITE;
	int X, Y, W, H;
	fl_clip_box(x(), y(), w(), h(), X, Y, W, H);
	if (!W)
		return;

	fl_rectf(X, Y, W, H, FL_GRAY + 1);
}

int pdfview::handle(int e) {

	switch (e) {
		case FL_PUSH:
		case FL_FOCUS:
		case FL_ENTER:
			return 1;
		break;
		case FL_DRAG:
			fl_cursor(FL_CURSOR_MOVE);
			// TODO
		break;
		case FL_MOUSEWHEEL:
			// TODO
		break;
		case FL_KEYDOWN:
		case FL_SHORTCUT:
			// TODO
		break;
		case FL_MOVE:
			// Set the cursor appropriately
			if (!file->maxw)
				fl_cursor(FL_CURSOR_WAIT);
			else if (selecting->value())
				fl_cursor(FL_CURSOR_INSERT);
			else
				fl_cursor(FL_CURSOR_DEFAULT);
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

void pdfview::go(const u32 page) {
	yoff = page;
}
