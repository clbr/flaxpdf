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

// Quarter inch in double resolution
#define MARGIN 36

pdfview::pdfview(int x, int y, int w, int h): Fl_Widget(x, y, w, h),
		yoff(0) {

	cachedsize = 7 * 1024 * 1024;

	u32 i;
	for (i = 0; i < CACHE_MAX; i++) {
		cache[i] = (u8 *) xcalloc(cachedsize, 1);
		cachedpage[i] = USHRT_MAX;
	}
}

void pdfview::reset() {
	yoff = 0;

	u32 i;
	for (i = 0; i < CACHE_MAX; i++) {
		cachedpage[i] = USHRT_MAX;
	}
}

static bool hasmargins(const u32 page) {
	if (!file->cache[page].ready)
		return file->cache[0].left > MARGIN ||
			file->cache[0].right > MARGIN ||
			file->cache[0].top > MARGIN ||
			file->cache[0].bottom > MARGIN;

	return file->cache[page].left > MARGIN ||
		file->cache[page].right > MARGIN ||
		file->cache[page].top > MARGIN ||
		file->cache[page].bottom > MARGIN;
}

static void updatevisible(const float yoff, const u32 w, const u32 h, const bool fromdraw) {
	// From the current zoom mode and view offset, update the visible page info
	const u32 prev = file->first_visible;
	file->first_visible = yoff < 0 ? 0 : yoff;
	u32 i;

	const u32 maxw = file->maxw ? file->maxw : file->cache[0].w;
	const u32 maxh = file->maxh ? file->maxh : file->cache[0].h;
	const u32 maxwmargin = hasmargins(file->first_visible) ? maxw + MARGIN * 2 : maxw;
	const u32 fullw = file->cache[0].w + file->cache[0].left + file->cache[0].right;
	const u32 fullh = file->cache[0].h + file->cache[0].top + file->cache[0].bottom;

	const float visible = yoff < 0 ? 0 : 1 - (yoff - floorf(yoff));

	u32 usedh = fullh;
	switch (file->mode) {
		case Z_TRIM:
			file->zoom = w / (float) maxwmargin;
			usedh = maxh;
		break;
		case Z_WIDTH:
			file->zoom = w / (float) fullw;
		break;
		case Z_PAGE:
			if (fullw > fullh) {
				file->zoom = w / (float) fullw;
			} else {
				file->zoom = h / (float) fullh;
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

	if (prev != file->first_visible) {
		char buf[10];
		snprintf(buf, 10, "%u", file->first_visible + 1);
		pagebox->value(buf);

		if (!fromdraw)
			pagebox->redraw();
	}
}

void pdfview::draw() {

	if (!file->cache)
		return;

	updatevisible(yoff, w(), h(), true);

	const Fl_Color pagecol = FL_WHITE;
	int X, Y, W, H;
	fl_clip_box(x(), y(), w(), h(), X, Y, W, H);
	if (!W)
		return;

	fl_rectf(X, Y, W, H, FL_GRAY + 1);

	struct cachedpage *cur = &file->cache[file->first_visible];
	if (!cur->ready)
		return;

	// Fill each page rect
	const float visible = yoff < 0 ? 0 : 1 - (yoff - floorf(yoff));
	Y = 0;
	H = (cur->h + cur->top + cur->bottom) * visible * file->zoom;
	if (file->mode == Z_CUSTOM) {
		X = x() + (cur->w + cur->left + cur->right) * file->zoom;
		W = (cur->w + cur->left + cur->right) * file->zoom;
	} else {
		X = x();
		W = w();

		if (file->mode == Z_TRIM)
			H = cur->h * visible * file->zoom;
	}
	fl_rectf(X, Y, W, H, pagecol);

	u32 i;
	const u32 max = file->last_visible;
	for (i = file->first_visible + 1; i <= max; i++) {
		Y += H + MARGIN * file->zoom;
		cur = &file->cache[i];
		if (!cur->ready)
			continue;

		H = (cur->h + cur->top + cur->bottom) * file->zoom;
		if (file->mode == Z_CUSTOM) {
			X = x() + (cur->w + cur->left + cur->right) * file->zoom;
			W = (cur->w + cur->left + cur->right) * file->zoom;
		} else {
			X = x();
			W = w();

			if (file->mode == Z_TRIM)
				H = cur->h * file->zoom;
		}

		fl_rectf(X, Y, W, H, pagecol);
	}
}

int pdfview::handle(int e) {

	const float move = 0.05f;
	static int lasty;

	switch (e) {
		case FL_PUSH:
			take_focus();
			lastx = Fl::event_x();
			lasty = Fl::event_y();
		case FL_FOCUS:
		case FL_ENTER:
			return 1;
		break;
		case FL_DRAG: {
			fl_cursor(FL_CURSOR_MOVE);

			const int my = Fl::event_y();

			const int movedy = my - lasty;

			yoff -= movedy / (float) h();

			if (yoff < 0)
				yoff = 0;
			if (yoff > file->pages)
				yoff = file->pages;

			lasty = my;

			if (file->cache)
				updatevisible(yoff, w(), h(), false);
			redraw();
		}
		break;
		case FL_MOUSEWHEEL:
			yoff += move * Fl::event_dy();
			if (yoff < 0)
				yoff = 0;

			if (file->cache)
				updatevisible(yoff, w(), h(), false);
			redraw();
		break;
		case FL_KEYDOWN:
		case FL_SHORTCUT:
			switch (Fl::event_key()) {
				case FL_Up:
					yoff -= move;
					if (yoff < 0)
						yoff = 0;
					redraw();
				break;
				case FL_Down:
					yoff += move;
					if (yoff > file->pages)
						yoff = file->pages;
					redraw();
				break;
				default:
					return 0;
			}

			if (file->cache)
				updatevisible(yoff, w(), h(), false);

			return 1;
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
