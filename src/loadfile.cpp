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


}
