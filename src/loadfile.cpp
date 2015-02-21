#include "main.h"
#include <FL/Fl_File_Chooser.H>
#include <ErrorCodes.h>
#include <GlobalParams.h>
#include <SplashOutputDev.h>
#include <splash/SplashBitmap.h>

static bool nonwhite(const u8 * const pixel) {

	return pixel[0] != 255 ||
		pixel[1] != 255 ||
		pixel[2] != 255;
}

static void getmargins(const u8 * const src, const u32 w, const u32 h,
			const u32 rowsize, u32 *minx, u32 *maxx,
			u32 *miny, u32 *maxy) {

	int i, j;

	bool found = false;
	for (i = 0; i < (int) w && !found; i++) {
		for (j = 0; j < (int) h && !found; j++) {
			const u8 * const pixel = src + j * rowsize + i * 3;
			if (nonwhite(pixel)) {
				found = true;
				*minx = i;
			}
		}
	}

	found = false;
	for (j = 0; j < (int) h && !found; j++) {
		for (i = *minx; i < (int) w && !found; i++) {
			const u8 * const pixel = src + j * rowsize + i * 3;
			if (nonwhite(pixel)) {
				found = true;
				*miny = j;
			}
		}
	}

	const int startx = *minx, starty = *miny;

	found = false;
	for (i = w - 1; i >= startx && !found; i--) {
		for (j = h - 1; j >= starty && !found; j--) {
			const u8 * const pixel = src + j * rowsize + i * 3;
			if (nonwhite(pixel)) {
				found = true;
				*maxx = i;
			}
		}
	}

	found = false;
	for (j = h - 1; j >= starty && !found; j--) {
		for (i = *maxx; i >= startx && !found; i--) {
			const u8 * const pixel = src + j * rowsize + i * 3;
			if (nonwhite(pixel)) {
				found = true;
				*maxy = j;
			}
		}
	}
}

static void store(SplashBitmap * const bm, const u32 page) {

	const u32 w = bm->getWidth();
	const u32 h = bm->getHeight();
	const u32 rowsize = bm->getRowSize();

	const u8 * const src = bm->getDataPtr();
	u32 minx = 0, miny = 0, maxx = w - 1, maxy = h - 1;

	// Trim margins
	getmargins(src, w, h, rowsize, &minx, &maxx, &miny, &maxy);

	const u32 trimw = maxx - minx + 1;
	const u32 trimh = maxy - miny + 1;

	u8 * const trimmed = (u8 *) xcalloc(trimw * trimh * 3, 1);
	u32 j;
	for (j = miny; j <= maxy; j++) {
		const u32 destj = j - miny;
		memcpy(trimmed + destj * trimw * 3, src + j * rowsize, trimw * 3);
	}

	// Trimmed copy done, compress it
	u8 * const tmp = (u8 *) xcalloc(trimw * trimh * 3 * 1.08f, 1);
	u8 workmem[LZO1X_1_MEM_COMPRESS]; // 64kb, we can afford it
	lzo_uint outlen;
	int ret = lzo1x_1_compress(trimmed, trimw * trimh * 3, tmp, &outlen, workmem);
	if (ret != LZO_E_OK)
		die(_("Compression failed\n"));

	u8 * const dst = (u8 *) xcalloc(outlen, 1);
	memcpy(dst, tmp, outlen);
	free(tmp);

	// Store
	file->cache[page].uncompressed = trimw * trimh * 3;
	file->cache[page].w = trimw;
	file->cache[page].h = trimh;
	file->cache[page].left = minx;
	file->cache[page].right = w - maxx;
	file->cache[page].top = miny;
	file->cache[page].bottom = h -maxy;

	file->cache[page].size = outlen;
	file->cache[page].data = dst;

	free(trimmed);
}

static void dopage(const u32 page) {

	SplashColor white = { 255, 255, 255 };
	SplashOutputDev *splash = new SplashOutputDev(splashModeBGR8, 4, false, white);
	splash->startDoc(file->pdf);

	file->pdf->displayPage(splash, page + 1, 144, 144, 0, true, false, false);

	SplashBitmap * const bm = splash->takeBitmap();

	store(bm, page);

	delete bm;
	delete splash;

	__sync_bool_compare_and_swap(&file->cache[page].ready, 0, 1);
}

static void *renderer(void *) {

	// Optional timing
	struct timeval start, end;
	gettimeofday(&start, NULL);

	#pragma omp parallel for schedule(guided)
	for (u32 i = 1; i < file->pages; i++) {
		dopage(i);
	}

	// Print stats
	if (details) {
		u32 total = 0, totalcomp = 0;
		for (u32 i = 0; i < file->pages; i++) {
			total += file->cache[i].uncompressed;
			totalcomp += file->cache[i].size;
		}

		printf(_("Compressed mem usage %.2fmb, compressed to %.2f%%\n"),
			totalcomp / 1024 / 1024.0f, 100 * totalcomp / (float) total);

		gettimeofday(&end, NULL);
		const u32 us = usecs(start, end);

		printf(_("Processing the file took %u us\n"), us);
	}

	return NULL;
}

void loadfile(const char *file) {

	while (!file) {
		file = fl_file_chooser(_("Open PDF"), "*.pdf", NULL, 0);

		if (!file) {
			if (fl_choice(_("Quit?"), _("No"), _("Yes"), NULL))
				exit(0);
		}
	}

	// Parse info
	GooString gooname(file);
	PDFDoc *pdf = new PDFDoc(&gooname);
	if (!pdf->isOk()) {
		const int err = pdf->getErrorCode();
		const char *msg = _("Unknown");

		switch (err) {
			case errOpenFile:
			case errFileIO:
				msg = _("Couldn't open file");
			break;
			case errBadCatalog:
			case errDamaged:
			case errPermission:
				msg = _("Damaged PDF file");
			break;
		}

		fl_alert(_("Error %d, %s"), err, msg);

		return;
	}

	if (::file->cache) {
		// Free the old one
		pthread_cancel(::file->tid);
		pthread_join(::file->tid, NULL);

		u32 i;
		const u32 max = ::file->pages;
		for (i = 0; i < max; i++) {
			if (::file->cache[i].ready)
				free(::file->cache[i].data);
		}
		free(::file->cache);
		::file->cache = NULL;
	}

	::file->pdf = pdf;
	::file->pages = pdf->getNumPages();

	// Start threaded magic
	if (::file->pages < 1) {
		fl_alert(_("Couldn't open %s, perhaps it's corrupted?"), file);
		return;
	}

	::file->cache = (cachedpage *) xcalloc(::file->pages, sizeof(cachedpage));

	if (!globalParams)
		globalParams = new GlobalParams;

	dopage(0);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	const struct sched_param nice = { 15 };
	pthread_attr_setschedparam(&attr, &nice);

	pthread_create(&::file->tid, &attr, renderer, NULL);

	// Update title
	char relative[80];
	const char * const slash = strrchr(file, '/');
	if (!slash) {
		strncpy(relative, file, 80);
	} else {
		strncpy(relative, slash + 1, 80);
	}
	relative[79] = '\0';

	char tmp[160];
	snprintf(tmp, 160, "%s - FlaxPDF", relative);
	win->copy_label(tmp);

	// Update page count
	sprintf(tmp, "/ %u", ::file->pages);
	pagectr->copy_label(tmp);
}
