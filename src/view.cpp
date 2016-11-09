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

/*
The pdfview class has been modified extensively to allow for multiple
columns visualization of a document. You can think of this view as a
matrix of pages, each line of this matrix composed of as many columns
as required by the user.
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

void pdfview::reset_selection() {
	selx = sely = selx2 = sely2 = 0;
}

void pdfview::compute_screen_size() {
	screen_x = x();
	screen_y = y();
	screen_height = h();
	screen_width = w();
}


void pdfview::go(const u32 page) {
	yoff = page;
	adjust_yoff(0);
	reset_selection();
	redraw();
}

void pdfview::set_columns(int count) {
	if ((count >= 1) && (count <= 5)) {
		columns = count;
		redraw();
	}
}

void pdfview::set_params(int columns_count, float x, float y) {

	yoff = y;
	xoff = x;

	set_columns(columns_count);
	go(y);
}

void pdfview::reset() {
	yoff = 0;
	xoff = 0;

	adjust_yoff(0);

	reset_selection();

	u32 i;
	for (i = 0; i < CACHE_MAX; i++) {
		cachedpage[i] = USHRT_MAX;
	}
}

u32 pdfview::pageh(u32 page) const {
	if (file->cache[page].ready) {
		if (file->mode == Z_TRIM || file->mode == Z_PGTRIM)
			return file->cache[page].h;
		else 
			return  file->cache[page].h     +
					file->cache[page].top   +
					file->cache[page].bottom;
	}
	return 0;
}

u32 pdfview::pagew(u32 page) const {
	if (file->cache[page].ready) {
		if (file->mode == Z_TRIM || file->mode == Z_PGTRIM)
			return file->cache[page].w;
		else 
			return  file->cache[page].w     +
					file->cache[page].left  +
					file->cache[page].right;
	}
	return 0;
}

// Compute the height of a line of pages, selecting the page that is the
// highest.
u32 pdfview::fullh(u32 page) const {

	if (!file->cache[page].ready)
		page = 0;

	u32 fh = 0;
	u32 i;

	if (file->mode == Z_TRIM || file->mode == Z_PGTRIM) {
		for (i = page; (i < (page + columns)) && (i < file->pages); i++) {
			if (file->cache[i].ready && (file->cache[i].h > fh))
				fh = file->cache[i].h;
		}
	}
	else {
		for (i = page; (i < (page + columns)) && (i < file->pages); i++) {
			if (file->cache[i].ready) {
				u32 h = file->cache[i].h     +
						file->cache[i].top   +
						file->cache[i].bottom;
				if (h > fh)
					fh = h;
			}
		}				
	}

	return fh;
}

u32 pdfview::fullw(u32 page) const {
	u32 fw = 0;
	u32 i;

	if (file->mode == Z_TRIM || file->mode == Z_PGTRIM) {
		for (i = page; i < (page + columns); i++) {
			if (file->cache[i].ready && (i < file->pages))
				fw += file->cache[i].w;
			else
				fw += file->cache[0].w;
		}
	}
	else {
		for (i = page; i < (page + columns); i++) {
			if (file->cache[i].ready && (i < file->pages)) 
				fw += file->cache[i].w    +
					  file->cache[i].left +
					  file->cache[i].right;
			else
				fw += file->cache[0].w    +
					  file->cache[0].left +
					  file->cache[0].right;
		}
	}

	// Add the margins between columns
	return fw + (columns - 1) * MARGINHALF;
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

float pdfview::line_zoom_factor(const u32 first_page, u32 &width, u32 &height) const {

    // compute the required zoom factor to fit the line of pages on the screen,
    // according to the zoom mode parameter if not a custom zoom.

	const u32 line_width  = fullw(first_page);
	const u32 line_height = fullh(first_page);

	float zoom_factor;

	switch (file->mode) {
		case Z_TRIM:
		case Z_WIDTH:
			zoom_factor = (float)screen_width / line_width;
		break;
		case Z_PAGE:
		case Z_PGTRIM:
			if (((float)line_width / line_height) > ((float)screen_width / screen_height)) {
				zoom_factor = (float)screen_width / line_width;
			}
			else {
				zoom_factor = (float)screen_height / line_height;
			}
		break;
		default:
			zoom_factor = file->zoom;
		break;
	}

	width  = line_width;
	height = line_height;

	file->zoom = zoom_factor;

	return zoom_factor;
}

void pdfview::update_visible(const bool fromdraw) const {

	// From the current zoom mode and view offset, update the visible page info
	// Will adjust the following parameters:
	//
	// - file->first_visible
	// - file->last_visible
	// - pagebox->value
	//
	// This method as been extensively modified to take into account multicolumns
	// and the fact that no page will be expected to be of the same size as the others,
	// both for vertical and horizontal limits. Because of these constraints, the
	// zoom factor cannot be applied to the whole screen but for each "line" of
	// pages.

	const u32 prev = file->first_visible;

	// Those are very very conservative numbers to compute last_visible. 
	// The end of the draw process will adjust precisely the value
	static const u32 max_lines_per_screen[MAX_COLUMNS_COUNT] = { 2, 3, 4, 5, 6 };

	// Adjust file->first_visible

	file->first_visible = yoff < 0 ? 0 : yoff;
	if (file->first_visible > file->pages - 1)
		file->first_visible = file->pages - 1;

	// Adjust file->last_visible

	u32 new_last_visible = file->first_visible + max_lines_per_screen[columns - 1] * columns;
	if (new_last_visible >= file->pages) 
		file->last_visible = file->pages - 1;
	else
		file->last_visible = new_last_visible;

	// If position has changed:
    //    - update current page visible number in the pagebox
	//    - request a redraw (if not already called by draw...)
	if (prev != file->first_visible) {
		char buf[10];
		snprintf(buf, 10, "%u", file->first_visible + 1);
		pagebox->value(buf);

		if (!fromdraw)
			pagebox->redraw();

		Fl::check();
	}
}

// Compute the vertical screen size of a line of pages
u32 pdfview::pxrel(u32 page) const {
	float zoom;
	u32 line_width, line_height;
	
	zoom = line_zoom_factor(page, line_width, line_height);
	return line_height * zoom;
}

void pdfview::draw() {

	if (!file->cache)
		return;

	compute_screen_size();
	update_visible(true);

	const Fl_Color pagecol = FL_WHITE;
	int X, Y, W, H;
	int Xs, Ys, Ws, Hs; // Saved values

	fl_clip_box(screen_x, screen_y, screen_width, screen_height, X, Y, W, H);

	// If nothing is visible on screen, nothing to do
	if (W == 0 || H == 0)
		return;

    // Paint the background with page separation color
	fl_rectf(X, Y, W, H, FL_GRAY + 1);

	struct cachedpage *cur = &file->cache[file->first_visible];
	if (!cur->ready)
		return;

	fl_push_clip(X, Y, W, H);

	if (file->mode != Z_CUSTOM)
		xoff = 0.0f;

	// As the variables are used for calculations, reset them to full widget dimensions.
	// (doesn´t seems to be needed... they are adjusted to something else in the code
	//  that follow...)

	s32 current_screen_pos = 0;
	u32 first_page_in_line = file->first_visible;
	bool first_line = true;

	const float invisibleY = yoff - floorf(yoff);

	page_pos_struct *pp = page_pos_on_screen;
	page_pos_count = 0;

	u32 page = file->first_visible;

	// Do the following for each line of pages
	while (current_screen_pos < screen_height && (first_page_in_line < file->pages)) {

		float zoom;
		u32 line_width, line_height;
		zoom = line_zoom_factor(first_page_in_line, line_width, line_height);

		const int zoomedmargin     = zoom * MARGIN;
		const int zoomedmarginhalf = zoomedmargin / 2;

		H = line_height * zoom; // Line of pages height in screen pixels

		if (first_line) {
			Y = screen_y - invisibleY * H;

			first_line = false;
		}

		X = screen_x + screen_width / 2 - zoom * line_width / 2 + (zoom * xoff * line_width);

		// Do the following for each page in the line

		s32 column = 0;
		page = first_page_in_line;

		while ((column < columns) && (page < file->pages)) {

			cur = &file->cache[page];
			if (!cur->ready)
				break;

			H = pageh(page) * zoom;
			W = pagew(page) * zoom;

			fl_rectf(Xs = X, Ys = Y, Ws = W, Hs = H, pagecol);

			const bool margins = hasmargins(page);
			const bool trimmed = margins && (file->mode == Z_TRIM || file->mode == Z_PGTRIM);

			float ratio_x = 1.0f;
			float ratio_y = 1.0f;

			if (trimmed) {
				// If the page was trimmed, have the real one a bit smaller
				X += zoomedmarginhalf;
				Y += zoomedmarginhalf;
				W -= zoomedmargin;
				H -= zoomedmargin;

				// These changes in size have an impact on the zoom factor
				// previously used. We need to keep track of these changes
				// to help into the text selection process.
				ratio_x = W / (float)(W + zoomedmargin);
				ratio_y = H / (float)(H + zoomedmargin);
			} 
			else if (margins) {
				// Restore the full size with empty borders
				X += cur->left * zoom;
				Y += cur->top  * zoom;
				W -= (cur->left + cur->right) * zoom;
				H -= (cur->top + cur->bottom) * zoom;
			}

			// Render real content
			content(page, X, Y, W, H);

			// For each displayed page, we keep those parameters
			// to permit the localization of the page on screen when
			// a selection is made to retrieve the text underneath
			if (page_pos_count < PAGES_ON_SCREEN_MAX) {

				pp->page    = page;
				pp->X0      = Xs;
				pp->Y0      = Ys;				
				pp->X       = X;
				pp->Y       = Y;
				pp->W       = W;
				pp->H       = H;
				pp->zoom    = zoom;
				pp->ratio_x = ratio_x;
				pp->ratio_y = ratio_y;

				// if (page_pos_count == 0) 
				// 	printf("Copy X0:%d Y0:%d X:%d Y:%d W:%d H:%d Zoom: %6.3f page: %d\n", 
				// 		pp->X0, pp->Y0, pp->X, pp->Y, pp->W, pp->H, pp->zoom, pp->page + 1);

				page_pos_count++;
				pp++;
			}

			// And undo.
			X = Xs;
			Y = Ys;
			W = Ws;
			H = Hs;

			X += zoomedmarginhalf + (pagew(page) * zoom);
			page++; column++;
		}

		// Prepare for next line of pages

		Y += (line_height * zoom) + zoomedmarginhalf;

		first_page_in_line += columns;
		current_screen_pos = Y - screen_y;
	}

	file->last_visible = page;

	fl_pop_clip();
}

// Compute the maximum yoff value, taking care of the number of 
// columns displayed and the screen size. Start at the end of the
// document, getting up to the point where the line of pages will be
// out of reach on screen.
float pdfview::maxyoff() const {

	float zoom, f;
	u32   line_width, line_height, h;

	s32 last = file->pages - 1;
	last    -= (last % columns);

	if (!file->cache[last].ready)
		f = last + 0.5f;
	else {
		s32 H = screen_height;

		while (true) {
			zoom = line_zoom_factor(last, line_width, line_height);
			H   -= (h = zoom * (line_height + MARGINHALF));

			if (H <= 0) {
				H += (MARGINHALF * zoom);
				f = last + (float)(-H) / (zoom * line_height);
				break;
			} 

			last -= columns;
			if (last < 0) {
				f = 0.0f;
				break;
			}
		}
	}
	return f;
}

#include <FL/fl_draw.H>

void pdfview::end_of_selection() {

	s32 X, Y, W, H;
	if (selx < selx2) {
		X = selx;
		W = selx2 - X;
	} 
	else {
		X = selx2;
		W = selx - X;
	}

	if (sely < sely2) {
		Y = sely;
		H = sely2 - Y;
	} 
	else {
		Y = sely2;
		H = sely - Y;
	}

	// Search for the page caracteristics saved before with
	// the draw method.
	u32 idx = 0;
	page_pos_struct *pp = page_pos_on_screen;

	while (idx < page_pos_count) {
		if ((X >= pp->X0)         &&
			(Y >= pp->Y0)         &&
			(X < (pp->X + pp->W)) &&
			(Y < (pp->Y + pp->H)))
			break;
		idx++;
		pp++;
	}
	if (idx >= page_pos_count)
		return; // Not found

	// Adjust the selection rectangle to be inside the real page data
	if ((X + W) > (pp->X + pp->W))
		W = pp->X + pp->W - X;
	if ((Y + H) > (pp->Y + pp->H))
		H = pp->Y + pp->H - Y;
	if (((X + W) < pp->X) ||
		((Y + H) < pp->Y))
		return; // Out of the printed area
	if (X < pp->X) {
		W -= (pp->X - X);
		X = pp->X;
	}
	if (Y < pp->Y) {
		H -= (pp->Y - Y);
		Y = pp->Y;
	}

	// Convert to page coords
	if (file->mode == Z_TRIM || file->mode == Z_PGTRIM) {
		X = X - pp->X0 + (pp->zoom * file->cache[pp->page].left);
		Y = Y - pp->Y0 + (pp->zoom * file->cache[pp->page].top ) ;

		if (hasmargins(pp->page)) {
			X -= (pp->X - pp->X0);
			Y -= (pp->Y - pp->Y0);
		}
	}
	else {
		X -= pp->X0;
		Y -= pp->Y0;
	}

	// Convert to original page resolution
	X /= (pp->zoom * pp->ratio_x);
	Y /= (pp->zoom * pp->ratio_y);
	W /= (pp->zoom * pp->ratio_x);
	H /= (pp->zoom * pp->ratio_y);

	TextOutputDev * const dev = new TextOutputDev(NULL, true, 0, false, false);
	file->pdf->displayPage(dev, pp->page + 1, 144, 144, 0, true, false, false);
	GooString *str = dev->getText(X, Y, X + W, Y + H);
	const char * const cstr = str->getCString();

	// Put it to clipboard
	Fl::copy(cstr, strlen(cstr), 1);

	delete str;
	delete dev;
}

void pdfview::page_up() {
	if (floorf(yoff) == yoff)
		adjust_floor_yoff(-1.0f);
	else
		adjust_floor_yoff(0.0f);
	redraw();
}

void pdfview::page_down() {
	adjust_floor_yoff(1.0f);
	redraw();
}

void pdfview::page_top() {
	yoff = 0.0f;
	redraw();
}

void pdfview::page_bottom() {
	yoff = maxyoff();
	redraw();
}

int pdfview::handle(int e) {

	if (!file->cache)
		return Fl_Widget::handle(e);

	float zoom;
	u32 line_width, line_height;
	zoom = line_zoom_factor(yoff, line_width, line_height);

	static int lasty, lastx;
	const float move = 0.05f;

	switch (e) {
		case FL_RELEASE:
			// Was this a dragging text selection?
			if (selecting->value() && Fl::event_button() == FL_LEFT_MOUSE &&
				selx && sely && selx2 && sely2 && selx != selx2 &&
				sely != sely2)

				end_of_selection();

			break;
			
		case FL_PUSH:
			take_focus();
			lasty = Fl::event_y();
			lastx = Fl::event_x();

			reset_selection();
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
		
		case FL_DRAG:
			{
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

				// I don't understand this...
				if (file->maxh)
					adjust_yoff(-((movedy / zoom) / file->maxh));
				else
					adjust_yoff(-((movedy / (float) screen_height) / zoom));

				if (file->mode == Z_CUSTOM) {
					xoff += ((float) movedx / zoom) / line_width;
					float maxchg = (((((float)screen_width / zoom) + line_width) / 2) / line_width) - 0.1f;
					if (xoff < -maxchg)
						xoff = -maxchg;
					if (xoff > maxchg)
						xoff = maxchg;
				}

				lasty = my;
				lastx = mx;
			}
			redraw();
			break;
			
		case FL_MOUSEWHEEL:
			if (Fl::event_ctrl())
				if (Fl::event_dy() > 0)
					cb_Zoomout(NULL, NULL);
				else
					cb_Zoomin(NULL, NULL);
			else
				adjust_yoff(move * Fl::event_dy());

			reset_selection();
			redraw();
			break;
			
		case FL_KEYDOWN:
		case FL_SHORTCUT:
			reset_selection();
			switch (Fl::event_key()) {
				case 'k':
					if (Fl::event_ctrl())
						yoff = 0;
					else {
						const s32 page = yoff;
						s32 sh;
						s32 shp = sh = pxrel(page);
						if (page >= columns)
							shp = pxrel(page - columns);
						if (screen_height >= sh) {
							/* scroll up like Page_Up */
							if (floorf(yoff) >= yoff)
								adjust_floor_yoff(-1.0f);
							else
								adjust_floor_yoff(0.0f);
						} 
						else {
							/* scroll up a bit less than one screen height */
							float d = (screen_height - 2 * MARGIN) / (float) sh;
							if (((u32) yoff) != ((u32) (yoff - d))) {
							 	/* scrolling over page border */
							 	d -= (yoff - floorf(yoff));
							 	yoff = floorf(yoff);
							 	/* ratio of prev page can be different */
							 	d = d * (float) sh / (float) shp;
							}
							adjust_yoff(-d);
						}
					}
					redraw();
					break;
					
				case 'j':
					if (Fl::event_ctrl())
						yoff = maxyoff();
					else {
						const s32 page = yoff;
						s32 sh;
						s32 shn = sh = pxrel(page);
						if (page + columns <= (s32)(file->pages - 1))
							shn = pxrel(page + columns);
						if (screen_height >= sh)
							/* scroll down like Page_Down */
							adjust_floor_yoff(1.0f);
						else {
							/* scroll down a bit less than one screen height */
							float d = (screen_height - 2 * MARGIN) / (float) sh;
							if (((u32) yoff) != ((u32) (yoff + d))) {
							 	/* scrolling over page border */
							 	d -= (ceilf(yoff) - yoff);
							 	yoff = floorf(yoff) + columns;
							 	/* ratio of next page can be different */
							 	d = d * (float) sh / (float) shn;
							}
							adjust_yoff(d);
						}
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
					page_up();
					break;
					
				case FL_Page_Down:
					page_down();
					break;
					
				case FL_Home:
					if (Fl::event_ctrl())
						yoff = 0;
					else
						yoff = floorf(yoff);
					redraw();
					break;
					
				case FL_End:
					if (Fl::event_ctrl())
						yoff = maxyoff();
					else {
						u32 page = yoff;
						s32 sh = line_height * zoom;

						if (file->cache[page].ready) {
							const s32 hidden = sh - screen_height;
							float tmp = floorf(yoff) + hidden / (float) sh;
							if (tmp > yoff)
								yoff = tmp;
							adjust_yoff(0);
						}
						else
							yoff = ceilf(yoff) - 0.4f;
					}
					redraw();
					break;
					
				case FL_F + 8:
					cb_hide(NULL, NULL); // Hide toolbar
					break;
					
				default:
					return 0;
					
			} // switch (Fl::event_key())

			if (file->cache)
				update_visible(false);

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

	if (offset != 0.0f) {
		float y = floorf(yoff); 
		yoff += offset;
		float diff = floorf(yoff) - y;
		yoff = yoff + (diff * columns) - diff; 
	}

	if (yoff < 0.0f)
		yoff = 0.0f;
	else {
		float y = maxyoff();

		if (yoff > y)
			yoff = y;
	}
}

void pdfview::adjust_floor_yoff(float offset) {

	if (offset != 0.0f) {
		float y = floorf(yoff); 
		yoff = y + offset;
		float diff = floorf(yoff) - y;
		yoff = yoff + (diff * columns) - diff; 
	}
	else
		yoff = floorf(yoff);

	if (yoff < 0.0f)
		yoff = 0.0f;
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

void pdfview::content(const u32 page, const s32 X, const s32 Y, const u32 W, const u32 H) {

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
		} 
		else {
			x = selx2;
			w = selx - x;
		}

		if (sely < sely2) {
			y = sely;
			h = sely2 - y;
		} 
		else {
			y = sely2;
			h = sely - y;
		}

		XRenderFillRectangle(fl_display, PictOpOver, dst, &col, x, y, w, h);
	}

	XRenderFreePicture(fl_display, src);
	XRenderFreePicture(fl_display, dst);
}
