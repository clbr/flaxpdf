#include "main.h"
#include <FL/Fl_File_Chooser.H>

static void dopage(const u32 page) {

	#pragma omp atomic
	file->cache[page].ready = true;
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

	// Parse info

	// Start threaded magic
	dopage(0);

	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	const struct sched_param nice = { 15 };
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setschedparam(&attr, &nice);

	pthread_create(&tid, &attr, renderer, NULL);
}
