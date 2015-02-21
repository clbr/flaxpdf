#include "main.h"
#include <FL/Fl_File_Chooser.H>

void loadfile(const char *file) {

	while (!file) {
		file = fl_file_chooser(_("Open PDF"), "*.pdf", NULL, 0);

		if (!file) {
			if (fl_ask(_("Quit?")))
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
}
