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

#ifndef VIEW_H
#define VIEW_H

#include "main.h"

#define CACHE_MAX 15
#define PAGES_ON_SCREEN_MAX 50

// Used to keep drawing postion of displayed pages to
// help in the identification of the selection zone.
// Used by the end_of_selection method.
struct page_pos_struct {
	float zoom, ratio_x, ratio_y;
	u32 page;
	int X0, Y0, X, Y, W, H;
};

class pdfview: public Fl_Widget {
public:
	pdfview(int x, int y, int w, int h);
	void draw();
	int handle(int e);

	void go(const u32 page);
	void reset();
	void reset_selection();
	void set_columns(int count);
	inline float get_xoff() { return xoff; };
	inline float get_yoff() { return yoff; };
	inline int   get_columns() { return columns; };
	void set_params(int columns_count, float x, float y);
	void page_up();
	void page_down();
	void page_top();
	void page_bottom();
private:
	void compute_screen_size();
	float line_zoom_factor(u32 first_page, u32 &width,u32 &height) const;
	void update_visible(const bool fromdraw) const;
	u8 iscached(const u32 page) const;
	void docache(const u32 page);
	float maxyoff() const;
	u32 pxrel(u32 page) const;
	void content(const u32 page, const s32 X, const s32 y,
			const u32 w, const u32 h);
	void adjust_yoff(float offset);
	void adjust_floor_yoff(float offset);
	void end_of_selection();
	u32 pageh(u32 page) const;
	u32 pagew(u32 page) const;
	u32 fullh(u32 page) const;
	u32 fullw(u32 page) const;

	float yoff, xoff;
	u32 cachedsize;
	u8 *cache[CACHE_MAX];
	u16 cachedpage[CACHE_MAX];
	Pixmap pix[CACHE_MAX];

	page_pos_struct page_pos_on_screen[PAGES_ON_SCREEN_MAX];
	u32 page_pos_count;

	// Text selection coords
	u16 selx, sely, selx2, sely2;
	s32 columns;

	s32 screen_x, screen_y, screen_width, screen_height;
};

extern pdfview *view;

#endif
