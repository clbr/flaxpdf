#include "main.h"
#include <FL/Fl_File_Chooser.H>
#include <ErrorCodes.h>

static void dopage(const u32 page) {

	__sync_bool_compare_and_swap(&file->cache[page].ready, 0, 1);
}

static void *renderer(void *) {

	#pragma omp parallel for schedule(guided)
	for (u32 i = 1; i < file->pages; i++) {
		dopage(i);
	}
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
