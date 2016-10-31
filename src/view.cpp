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
#include <TextOutputDev.h>

// Quarter inch in double resolution
#define MARGIN 36
#define MARGINHALF 18

pdfview::pdfview(int x, int y, int w, int h): Fl_Widget(x, y, w, h),
		yoff(0), xoff(0),
		selx(0), sely(0), selx2(0), sely2(0),
		columns(1) {

	cachedsize = 7 * 1024 * 1024; // 7 megabytes

	u32 i;
	for (i = 0; i < CACHE_MAX; i++) {
		cache[i] = (u8 *) xcalloc(cachedsize, 1);
		cachedpage[i] = USHRT_MAX;
		pix[i] = None;
	}
}

void pdfview::resetselection() {
	selx = sely = selx2 = sely2 = 0;
}

void pdfview::set_scrollbar(Fl_Scrollbar* vscroll) {
	vertical_scrollbar = vscroll;
}

void pdfview::adjust_scrollbar_parameters() {
	float y = floorf(yoff);
	float frac = yoff - y;
	float k = file->pages - columns + 1;
	k = k < columns ? columns : k;
	int relpos = ((y / columns + frac) / (k / columns)) * 50000;
	int size = 50000 / (k / columns);
	vertical_scrollbar->value(relpos, size, 0, 50000);
}

// From the document position, update scrollbar position.
void pdfview::update_scrollbar() {
	float y = floorf(yoff);
	float frac = yoff - y;
	float k = file->pages - columns + 1;
	k = k < columns ? columns : k;
	int relpos = ((y / columns + frac) / (k / columns)) * 50000;
	vertical_scrollbar->value(relpos);
}

// From the scrollbar position, update position in the document.
void pdfview::update_position(const int vscroll_pos) {
	return;
	float frac = yoff - floorf(yoff);
	if (file != NULL && file->pages > 0) {
		yoff = (((float)(vscroll_pos)) / 50000) * file->pages + (frac * columns);
		adjust_yoff(0);
		redraw();
	}
}

void pdfview::go(const u32 page) {
	yoff = page;
	adjust_yoff(0);
	resetselection();
	redraw();
}

void pdfview::set_columns(int count) {
	columns = count;
	if (file && file->pages > 0) {
		adjust_scrollbar_parameters();
	}
	redraw();
}

void pdfview::set_params(int columns_count, float x, float y) {

	yoff = y;
	xoff = x;

	adjust_yoff(0);

	columns = columns_count;
	if (file && file->pages > 0) {
		adjust_scrollbar_parameters();
	}
	redraw();
}

void pdfview::reset() {
	yoff = 0;
	xoff = 0;

	resetselection();

	u32 i;
	for (i = 0; i < CACHE_MAX; i++) {
		cachedpage[i] = USHRT_MAX;
	}

	adjust_scrollbar_parameters();
}

static u32 fullh(u32 page) {
	if (!file->cache[page].ready)
		page = 0;

	if (file->mode == Z_TRIM)
		return file->cache[page].h;

	return
	    file->cache[page].h     +
		file->cache[page].top   +
		file->cache[page].bottom;
}

static u32 fullw(u32 page) {
	if (!file->cache[page].ready)
		page = 0;

	if (file->mode == Z_TRIM)
		return file->cache[page].w;

	return
	    file->cache[page].w    +
		file->cache[page].left +
		file->cache[page].right;
}

static bool hasmargins(const u32 page) {
	if (!file->cache[page].ready)
		return
	        file->cache[0].left   > MARGIN ||
			file->cache[0].right  > MARGIN ||
			file->cache[0].top    > MARGIN ||
			file->cache[0].bottom > MARGIN;

	return
	    file->cache[page].left   > MARGIN ||
		file->cache[page].right  > MARGIN ||
		file->cache[page].top    > MARGIN ||
		file->cache[page].bottom > MARGIN;
}

static void updatevisible(
	const float yoff,
	const u32 w,
	const u32 h,
	const u32 columns,
	const bool fromdraw) {

	// From the current zoom mode and view offset, update the visible page info
	// Will adjust the following parameters:
	// - file->first_visible
	// - file->last_visible
	// - file->zoom

	const u32 prev = file->first_visible;

	// Adjust file->first_visible

	file->first_visible = yoff < 0 ? 0 : yoff;
	if (file->first_visible > file->pages - 1)
		file->first_visible = file->pages - 1;
	u32 i;

    // Adjust file->zoom

	const u32 maxw = file->maxw ? file->maxw : file->cache[0].w;
	const u32 maxh = file->maxh ? file->maxh : file->cache[0].h;
	const u32 maxwmargin = (hasmargins(file->first_visible) ? maxw + MARGIN * 2 : maxw) +
								((columns - 1) * MARGINHALF);
	const u32 fullw = ::fullw(0);
	const u32 fullh = ::fullh(0);

	const float visible = yoff < 0 ? 0 : 1 - (yoff - floorf(yoff));

	u32 usedh = fullh;
	switch (file->mode) {
		case Z_TRIM:
			file->zoom = (w / columns) / (float) maxwmargin;
			usedh = maxh;
		break;
		case Z_WIDTH:
			file->zoom = (w / columns) / (float) fullw;
		break;
		case Z_PAGE:
			if ((((float)(fullw * columns)) / fullh) > (((float)w) / h)) {
				file->zoom = (w / columns) / (float) fullw;
			}
			else {
				file->zoom = h / (float) fullh;
			}
		break;
		case Z_CUSTOM:
		break;
	}

	// Adjust file->last_visible

	const u32 zoomedmargin = MARGIN * file->zoom;
	i = file->first_visible;
	u32 tmp = visible * usedh * file->zoom;
	tmp += zoomedmargin;
	while (tmp < h) {
		tmp += usedh * file->zoom;
		tmp += zoomedmargin;
		i += columns;
	}
	// Be conservative
	i += columns;
	if (i > file->pages - 1)
		i = file->pages - 1;
	file->last_visible = i;

	// If position has changed:
    //    - update current page visible number in the pagebox
	//    - request a redraw (if not already called by draw...)
	if (prev != file->first_visible) {
		char buf[10];
		snprintf(buf, 10, "%u", file->first_visible + 1);
		pagebox->value(buf);

		if (!fromdraw)
			pagebox->redraw();
	}

	view->update_scrollbar();
}

void pdfview::draw() {

	if (!file->cache)
		return;

	updatevisible(yoff, w(), h(), columns, true);

	const Fl_Color pagecol = FL_WHITE;
	int X, Y, W, H;
	int Xs, Ys, Ws, Hs; // Saved values

	fl_clip_box(x(), y(), w(), h(), X, Y, W, H);

	// If nothing is visible on screen, nothing to do
	if (!W)
		return;

    // Paint the background with page separation color
	fl_rectf(X, Y, W, H, FL_GRAY + 1);

	struct cachedpage *cur = &file->cache[file->first_visible];
	if (!cur->ready)
		return;

	fl_push_clip(X, Y, W, H);

	// As the variables are used for calculations, reset them to full widget dimensions.
	// (doesn´t seems to be needed... they are adjusted to something else in the code
	//  that follow...)

	//X = x();
	//Y = y();
	//W = w();
	//H = h();

	// Fill each page rect
	const int zoomedmargin = file->zoom * MARGIN;
	const int zoomedmarginhalf = zoomedmargin / 2;
	s32 i, column;
	const float visible = yoff - floorf(yoff);
	int maxH = 0;

	H = pxrel(file->first_visible);
	Y = y() - visible * H;

	bool first = true;

	// Do the following for each visible page
	for (i = (int)file->first_visible, column = 0;
	     i <= (int)file->last_visible;
	     i++, column = (column + 1) % columns) {

		cur = &file->cache[i];
		if (!cur->ready)
			break;

		H = (fullh(i) + zoomedmargin) * file->zoom;
		if (file->mode == Z_CUSTOM || file->mode == Z_PAGE) {
			W = fullw(i) * file->zoom;
			X =  x() +
			    (w() - (W * columns) - ((columns - 1) * zoomedmarginhalf)) / 2 +
			    (xoff * W) +
			    (column * W) +
			    (column * zoomedmarginhalf);
		}
		else {
			W = (w() - ((columns - 1) * zoomedmarginhalf)) / columns;
			X =  x() + ( column * (W + zoomedmarginhalf));

			// In case of different page sizes, H needs to be adjusted per-page
			const float ratio = W / (float) fullw(i);
			H = fullh(i) * ratio + zoomedmargin;
		}

		if (first) {
			float xf = X;
			debug(debug1, xoff, "xoff");
			debug(debug2, xf, "X");
			debug(debug3, W, "W");
			debug(debug4, yoff, "yoff");
			debug(debug5, Y, "Y");
			debug(debug6, H, "H");
			first = false;
		}

		// XYWH is now the full area including grey margins.
		Y += zoomedmarginhalf;
		H -= zoomedmargin;

		// XYWH is now the page's area.
		if (Y >= (y() + h()))
			continue;

		fl_rectf(Xs = X, Ys = Y, Ws = W, Hs = H, pagecol);

		const bool margins = hasmargins(i);
		const bool trimmed = margins && file->mode == Z_TRIM;
		if (trimmed) {
			// If the page was trimmed, have the real one a bit smaller
			X += zoomedmarginhalf;
			Y += zoomedmarginhalf;
			W -= zoomedmargin;
			H -= zoomedmargin;
		} else if (margins) {
			// Restore the full size with empty borders
			X += cur->left * file->zoom;
			Y += cur->top * file->zoom;
			W -= (cur->left + cur->right) * file->zoom;
			H -= (cur->top + cur->bottom) * file->zoom;
		}

		// Render real content
		content(i, X, Y, W, H);

		// And undo.
		X = Xs;
		Y = Ys;
		W = Ws;
		H = Hs;

		/*
		if (trimmed) {
			// And undo.
			Y -= zoomedmarginhalf;
			H += zoomedmargin;
		} else if (margins) {
			// And undo.
			Y -= cur->top * file->zoom;
			H += (cur->top + cur->bottom) * file->zoom;
		}
		*/

		H += zoomedmargin;
		Y -= zoomedmarginhalf;
		if (column == (columns - 1)) {
			Y += maxH == 0 ? H : (maxH >= H ? maxH : H);
			maxH = 0;
		}
		else {
			maxH = maxH >= H ? maxH : H;
		}
	}

	fl_pop_clip();
}

u32 pdfview::pxrel(u32 page) const {
	u32 maxH, h, p;
	maxH = 0;
	for (p = page; (p < (page + columns)) && (p < file->pages); p++) {
		if (file->mode != Z_CUSTOM && file->mode != Z_PAGE) {
			const float ratio = (w() / columns) / (float) fullw(p);
			h = fullh(p) * ratio + MARGIN * file->zoom;
		}
		else
			h = (fullh(p) + MARGIN) * file->zoom;
		maxH = maxH > h ? maxH : h;
	}
	return maxH;
}

float pdfview::maxyoff() const {

	float f;

	u32 last = file->pages - 1;
	last -= (last % columns);

	if (!file->cache[last].ready)
		f = last + 0.5f;
	else {
		s32 sh = pxrel(last);

		const s32 hidden = sh - h();

		f = last;

		if (hidden > 0) {
			f += hidden / (float) sh;
		} else {
			f -= columns - (sh / (float) h());
			f += MARGIN / (float) h();
		}
	}

	if (f < 0)
		f = 0.0;
	return f;
}

int pdfview::handle(int e) {

	if (!file->cache)
		return Fl_Widget::handle(e);

	const float move = 0.05f;
	static int lasty, lastx;

	switch (e) {
		case FL_RELEASE:
			// Was this a dragging text selection?
			if (selecting->value() && Fl::event_button() == FL_LEFT_MOUSE &&
				selx && sely && selx2 && sely2 && selx != selx2 &&
				sely != sely2) {

				const u32 zoomedmargin = MARGIN * file->zoom;
				const u32 zoomedmarginhalf = zoomedmargin / 2;

				// Which page is selx,sely on?
				u32 page = yoff;
				const float floored = yoff - floorf(yoff);
				const float visible = 1 - floored;
				u32 zoomedh = fullh(page) * visible * file->zoom;
				const u32 ratiominus = hasmargins(page) ? zoomedmargin : 0;
				const float ratio = w() / (float) fullw(page);
				const float ratiox = (w() - ratiominus) / (float) fullw(page);
				if (file->mode != Z_CUSTOM && file->mode != Z_PAGE) {
					zoomedh = fullh(page) * visible * ratio;
				}

				if (y() + zoomedh < sely)
					page++;

				// We assume nobody selects text with a tiny zoom.
				u32 X, Y, W, H;
				if (selx < selx2) {
					X = selx;
					W = selx2 - X;
				} else {
					X = selx2;
					W = selx - X;
				}

				if (sely < sely2) {
					Y = sely;
					H = sely2 - Y;
				} else {
					Y = sely2;
					H = sely - Y;
				}

				// Convert to widget coords
				X -= x();
				Y -= y();

				// Offset
				switch (file->mode) {
					case Z_TRIM:
						// X and Y start in widget space, 0 to w/h.
						Y += floored * (fullh(page) * ratiox +
								zoomedmargin + ratiominus);
						Y -= zoomedmarginhalf; // Grey margin

						if (hasmargins(page)) {
							X -= zoomedmarginhalf;
							Y -= zoomedmarginhalf;
						}

						// Now page's area.

						X += file->cache[page].left * ratiox;
						Y += file->cache[page].top * ratiox;

						X /= ratiox;
						Y /= ratiox;
						W /= ratiox;
						H /= ratiox;
					break;
					case Z_WIDTH:
						// X and Y start in widget space, 0 to w/h.
						Y += floored * (fullh(page) * ratio +
								zoomedmargin);
						Y -= zoomedmarginhalf; // Grey margin

						X /= ratio;
						Y /= ratio;
						W /= ratio;
						H /= ratio;
					break;
					case Z_PAGE:
					case Z_CUSTOM:
						// X and Y start in widget space, 0 to w/h.
						Y += floored * (fullh(page) * file->zoom +
								zoomedmargin);
						Y -= zoomedmarginhalf; // Grey margin

						X -= (w() - (fullw(page) * file->zoom)) / 2 +
							xoff * (fullw(page) * file->zoom);

						X /= file->zoom;
						Y /= file->zoom;
						W /= file->zoom;
						H /= file->zoom;
					break;
				}

				TextOutputDev * const dev = new TextOutputDev(NULL, true,
								0, false, false);
				file->pdf->displayPage(dev, page + 1, 144, 144, 0,
								true, false, false);
				GooString *str = dev->getText(X, Y, X + W, Y + H);
				const char * const cstr = str->getCString();

				// Put it to clipboard
				Fl::copy(cstr, strlen(cstr));

				delete str;
				delete dev;
			}
		break;
		case FL_PUSH:
			take_focus();
			lasty = Fl::event_y();
			lastx = Fl::event_x();

			resetselection();
			if (selecting->value() && Fl::event_button() == FL_LEFT_MOUSE) {
				selx = lastx;
				sely = lasty;
			}
			redraw();

			// Fall-through
		case FL_FOCUS:
		case FL_ENTER:
			return 1;
		break;
		case FL_DRAG: {
			const int my = Fl::event_y();
			const int mx = Fl::event_x();
			const int movedy = my - lasty;
			const int movedx = mx - lastx;

			if (selecting->value() && Fl::event_button() == FL_LEFT_MOUSE) {
				selx2 = mx;
				sely2 = my;
				redraw();
				return 1;
			}

			fl_cursor(FL_CURSOR_MOVE);

			if (file->maxh) {
				if (file->mode != Z_TRIM)
					//yoff -= (movedy / file->zoom) / fullh(0);
					adjust_yoff(-((movedy / file->zoom) / fullh(0)));
				else
					//yoff -= (movedy / file->zoom) / file->maxh;
					adjust_yoff(-((movedy / file->zoom) / file->maxh));
			} else {
				//yoff -= (movedy / (float) h()) / file->zoom;
				adjust_yoff(-((movedy / (float) h()) / file->zoom));
			}

			xoff += (movedx / file->zoom) / fullw(0);

			//if (yoff < 0)
			//	yoff = 0;
			//if (yoff >= maxyoff())
			//	yoff = maxyoff();

			if (xoff < -1)
				xoff = -1;
			if (xoff > 1)
				xoff = 1;
			if (file->mode != Z_CUSTOM)
				xoff = 0;

			lasty = my;
			lastx = mx;

			if (file->cache)
				updatevisible(yoff, w(), h(), columns, false);
			redraw();
		}
		break;
		case FL_MOUSEWHEEL:
			if (Fl::event_ctrl()) {
				if (Fl::event_dy() > 0)
					cb_Zoomout(NULL, NULL);
				else
					cb_Zoomin(NULL, NULL);
			} else {
				adjust_yoff(move * Fl::event_dy());
			}

			resetselection();
			if (file->cache)
				updatevisible(yoff, w(), h(), columns, false);
			redraw();
		break;
		case FL_KEYDOWN:
		case FL_SHORTCUT:
			resetselection();
			switch (Fl::event_key()) {
				case 'k':
					if (Fl::event_ctrl()) {
						yoff = 0;
					} else {
						if (floorf(yoff) == yoff) 
							adjust_floor_yoff(-1.0);
						else
							adjust_floor_yoff(0.0);
						//const u32 page = yoff;
						//s32 sh = pxrel(page);
						//s32 shp = sh;
						//if (page > 0)
						//	shp = pxrel(page - 1);
						//if (h() + 2 * MARGIN * file->zoom >= sh) {
						//	/* scroll up like Page_Up */
						//	if (floorf(yoff) + MARGIN * file->zoom / (float) sh >= yoff) {
						//		yoff = floorf(yoff - 1) + MARGIN * file->zoom / 2 / (float) shp;
						//	} else {
						//		yoff = floorf(yoff) + MARGIN * file->zoom / 2 / (float) sh;
						//	}
						//} else {
						//	/* scroll up less than one page height */
						//	float d = (h() - MARGIN) / (float) sh;
						//	if (((u32) yoff) == ((u32) (yoff - d))) {
						//		/* not scrolling over page border */
						//		yoff -= d;
						//	} else {
						//		/* scrolling over page border */
						//		d -= (yoff - floorf(yoff));
						//		yoff = floorf(yoff);
						//		/* ratio of prev page can be different */
						//		d = d * (float) sh / (float) shp;
						//		yoff -= d;
						//	}
						//}
						//if (yoff < 0)
						//	yoff = 0;
						//if (yoff >= maxyoff())
						//	yoff = maxyoff();
					}
					redraw();
				break;
				case 'j':
					if (Fl::event_ctrl()) {
						yoff = maxyoff();
					} else {
						adjust_floor_yoff(1);
						//const u32 page = yoff;
						//s32 sh = pxrel(page);
						//s32 shn = sh;
						//if (page + 1 <= file->pages - 1)
						//	shn = pxrel(page + 1);
						//if (h() + 2 * MARGIN * file->zoom >= sh) {
						//	/* scroll down like Page_Down */
						//	yoff = floorf(yoff + 1) + MARGIN * file->zoom / 2 / (float) shn;
						//} else {
						//	/* scroll down less than one page height */
						//	float d = (h() - MARGIN) / (float) sh;
						//	if (((u32) yoff) == ((u32) (yoff + d))) {
						//		/* not scrolling over page border */
						//		yoff += d;
						//	} else {
						//		/* scrolling over page border */
						//		d -= (ceilf(yoff) - yoff);
						//		yoff = ceilf(yoff);
						//		/* ratio of next page can be different */
						//		d = d * (float) sh / (float) shn;
						//		yoff += d;
						//	}
						//}
						//if (yoff < 0)
						//	yoff = 0;
						//if (yoff >= maxyoff())
						//	yoff = maxyoff();
					}
					redraw();
				break;
				case FL_Up:
					adjust_yoff(-move);
					redraw();
				break;
				case FL_Down:
					adjust_yoff(move);
					redraw();
				break;
				case FL_Page_Up:
				{
					if (floorf(yoff) == yoff) 
						adjust_floor_yoff(-1.0);
					else
						adjust_floor_yoff(0.0);
					//const u32 page = yoff;
					//s32 sh = pxrel(page);
					//s32 shp = sh;
					//if (page)
					//	shp = pxrel(page - 1);
					//if (floorf(yoff) + MARGIN * file->zoom / (float) sh >= yoff) {
					//	yoff = floorf(yoff - 1) + MARGIN * file->zoom / 2 / (float) shp;
					//}
					//else {
					//	yoff = floorf(yoff) + MARGIN * file->zoom / 2 / (float) sh;
					//}
					//if (yoff < 0)
					//	yoff = 0;
					redraw();
				}
				break;
				case FL_Page_Down:
				{
					adjust_floor_yoff(1);
					//u32 page = floorf(yoff);
					//s32 shn = pxrel(page);
					//if (page + 1 <= file->pages - 1)
					//	shn = pxrel(page + 1);
					//yoff = floorf(yoff + 1) + MARGIN * file->zoom / 2 / (float) shn;
					//if (yoff >= maxyoff())
					//	yoff = maxyoff();
					redraw();
				}
				break;
				case FL_Home:
					if (Fl::event_ctrl()) {
						yoff = 0;
					} else {
						yoff = floorf(yoff);
					}
					redraw();
				break;
				case FL_End:
					if (Fl::event_ctrl()) {
						yoff = maxyoff();
					} else {
						u32 page = yoff;
						s32 sh = pxrel(page);

						if (file->cache[page].ready) {
							const s32 hidden = sh - h();
							float tmp = floorf(yoff) + hidden / (float) sh;
							if (tmp > yoff)
								yoff = tmp;
							if (yoff < 0)
								yoff = 0;
							if (yoff >= maxyoff())
								yoff = maxyoff();
						} else {
							yoff = ceilf(yoff) - 0.4f;
						}
					}
					redraw();
				break;
				case FL_F + 8:
					cb_hide(NULL, NULL); // Hide toolbar
				break;
				default:
					return 0;
			}

			if (file->cache)
				updatevisible(yoff, w(), h(), columns, false);

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

void pdfview::adjust_yoff(float offset) {

	if (offset != 0.0) {
		float y = floorf(yoff); 
		yoff += offset;
		float diff = floorf(yoff) - y;
		yoff = yoff + (diff * columns) - diff; 
	}

	if (yoff < 0)
		yoff = 0;
	else {
		float y = maxyoff();

		if (yoff > y)
			yoff = y;
	}
}

void pdfview::adjust_floor_yoff(float offset) {

	if (offset != 0.0) {
		float y = floorf(yoff); 
		yoff = y + offset;
		float diff = floorf(yoff) - y;
		yoff = yoff + (diff * columns) - diff; 
	}

	if (yoff < 0)
		yoff = 0;
	else {
		float max = maxyoff();

		if (yoff > max)
			yoff = max;
	}
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
	const struct cachedpage * const cur = &file->cache[page];
	u32 i;

	if (cur->uncompressed > cachedsize) {
		cachedsize = cur->uncompressed;

		for (i = 0; i < CACHE_MAX; i++) {
			cache[i] = (u8 *) realloc(cache[i], cachedsize);
		}
	}

	// Be safe
	if (!cur->ready)
		return;

	const u32 dst = rand() % CACHE_MAX;

	lzo_uint dstsize = cachedsize;
	const int ret = lzo1x_decompress(cur->data,
					cur->size,
					cache[dst],
					&dstsize,
					NULL);
	if (ret != LZO_E_OK || dstsize != cur->uncompressed)
		die(_("Error decompressing\n"));

	cachedpage[dst] = page;

	// Create the Pixmap
	if (pix[dst] != None)
		XFreePixmap(fl_display, pix[dst]);

	pix[dst] = XCreatePixmap(fl_display, fl_window, cur->w, cur->h, 24);
	if (pix[dst] == None)
		return;

	fl_push_no_clip();

	XImage *xi = XCreateImage(fl_display, fl_visual->visual, 24, ZPixmap, 0,
					(char *) cache[dst], cur->w, cur->h,
					32, 0);
	if (xi == NULL) die("xi null\n");

	XPutImage(fl_display, pix[dst], fl_gc, xi, 0, 0, 0, 0, cur->w, cur->h);

	fl_pop_clip();

	xi->data = NULL;
	XDestroyImage(xi);
}

void pdfview::content(const u32 page, const s32 X, const s32 Y,
			const u32 W, const u32 H) {

	// Do a gpu-accelerated bilinear blit
	u8 c = iscached(page);
	if (c == UCHAR_MAX)
		docache(page);
	c = iscached(page);

	const struct cachedpage * const cur = &file->cache[page];

	XRenderPictureAttributes srcattr;
	memset(&srcattr, 0, sizeof(XRenderPictureAttributes));
	XRenderPictFormat *fmt = XRenderFindStandardFormat(fl_display, PictStandardRGB24);

	// This corresponds to GL_CLAMP_TO_EDGE.
	srcattr.repeat = RepeatPad;

	Picture src = XRenderCreatePicture(fl_display, pix[c], fmt, CPRepeat, &srcattr);
	Picture dst = XRenderCreatePicture(fl_display, fl_window, fmt, 0, &srcattr);

	const Fl_Region clipr = fl_clip_region();
	XRenderSetPictureClipRegion(fl_display, dst, clipr);

	XRenderSetPictureFilter(fl_display, src, "bilinear", NULL, 0);
	XTransform xf;
	memset(&xf, 0, sizeof(XTransform));
	xf.matrix[0][0] = (65536 * cur->w) / W;
	xf.matrix[1][1] = (65536 * cur->h) / H;
	xf.matrix[2][2] = 65536;
	XRenderSetPictureTransform(fl_display, src, &xf);

	XRenderComposite(fl_display, PictOpSrc, src, None, dst, 0, 0, 0, 0, X, Y, W, H);
//	XCopyArea(fl_display, pix[c], fl_window, fl_gc, 0, 0, W, H, X, Y);
//	fl_draw_image(cache[c], X, Y, W, H, 4, file->cache[page].w * 4);

	if (selecting->value() && selx2 && sely2 && selx != selx2 && sely != sely2) {
		// Draw a selection rectangle over this area
		const XRenderColor col = {0, 0, 16384, 16384};

		u16 x, y, w, h;
		if (selx < selx2) {
			x = selx;
			w = selx2 - x;
		} else {
			x = selx2;
			w = selx - x;
		}

		if (sely < sely2) {
			y = sely;
			h = sely2 - y;
		} else {
			y = sely2;
			h = sely - y;
		}

		XRenderFillRectangle(fl_display, PictOpOver, dst, &col,
					x, y, w, h);
	}

	XRenderFreePicture(fl_display, src);
	XRenderFreePicture(fl_display, dst);
}
