/*
Copyright (C) 2016 Guy Turcotte

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

//#define TEST 1

#ifdef TEST
  #include "config.h"
#else
  #include "main.h"
#endif

#ifdef TEST
  #define fl_alert printf
  #define fl_message printf
  #define xmalloc malloc
  #define _(a) a
#endif

std::string CONFIG_VERSION("1.0");
std::string version("ERROR");

recent_file_struct *recent_files = NULL;

void save_to_config(
		char * filename,
		int columns,
		float xoff,
		float yoff,
		float zoom,
		int zoommode,
		int x,
		int y,
		int w,
		int h,
		bool full) {

	recent_file_struct *prev = NULL;
	recent_file_struct *rf   = recent_files;

	while ((rf != NULL) && (rf->filename.compare(filename) != 0)) {

		prev = rf;
		rf   = rf->next;
	}

	if (rf) {
		rf->columns    = columns;
		rf->xoff       = xoff;
		rf->yoff       = yoff;
		rf->zoom       = zoom;
		rf->zoom_mode  = zoommode;
		rf->x          = x;
		rf->y          = y;
		rf->width      = w;
		rf->height     = h;
		rf->fullscreen = full;

		// prev is NULL if it's already the first entry in the list
		if (prev) {
			prev->next   = rf->next;
			rf->next     = recent_files;
			recent_files = rf;
		}
	}
	else {
		rf = new recent_file_struct;

		rf->filename   = filename;
		rf->columns    = columns;
		rf->xoff       = xoff;
		rf->yoff       = yoff;
		rf->zoom       = zoom;
		rf->zoom_mode  = zoommode;
		rf->x          = x;
		rf->y          = y;
		rf->width      = w;
		rf->height     = h;
		rf->fullscreen = full;

		rf->next       = recent_files;
		recent_files   = rf;
	}
}

bool file_exists (const char * name) {
  struct stat buffer;   
  return (stat (name, &buffer) == 0); 
}

void load_config() {

	Config cfg;

	static char config_filename[301];
	char * homedir;

	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}

	strncpy(config_filename, homedir, 300);
	strncat(config_filename, "/.flaxpdf", 300 - strlen(config_filename) - 1);

	try {
		cfg.readFile(config_filename);

		Setting &root = cfg.getRoot();

		if (root.exists("version")){

			version = root["version"].c_str();

			if (version.compare(CONFIG_VERSION) != 0) {
				fl_alert(_("Error: Config file version is expected to be %s."), CONFIG_VERSION.c_str());
				return;
			}
		}
		else {
			fl_alert(_("Error: Config file structure error."));
			return;
		}

		if (!root.exists("recent_files"))
			root.add("recent_files", Setting::TypeList);

		Setting &rf_setting = root["recent_files"];
		recent_file_struct *prev = NULL;

		for (int i = 0; i < rf_setting.getLength(); i++) {

			Setting &s = rf_setting[i];
			std::string filename = s["filename"];
			
			if (file_exists(filename.c_str())) {
				recent_file_struct *rf = new recent_file_struct;

				if (!(s.lookupValue("filename"  , rf->filename  ) &&
				      s.lookupValue("columns"   , rf->columns   ) &&
				      s.lookupValue("xoff"      , rf->xoff      ) &&
				      s.lookupValue("yoff"      , rf->yoff      ) &&
				      s.lookupValue("zoom"      , rf->zoom      ) &&
				      s.lookupValue("zoom_mode" , rf->zoom_mode ) &&
				      s.lookupValue("x"         , rf->x         ) &&
				      s.lookupValue("y"         , rf->y         ) &&
				      s.lookupValue("width"     , rf->width     ) &&
				      s.lookupValue("height"    , rf->height    ) &&
				      s.lookupValue("fullscreen", rf->fullscreen))) {
					fl_alert(_("Configuration file content error: %s."), config_filename);
					return;
				}

				if (prev == NULL) {
					prev = rf;
					recent_files = rf;
				}
				else {
					prev->next = rf;
					prev = rf;
				}
				rf->next = NULL;
			}
		}
	}
	catch (const FileIOException &e) {
	}
	catch (const ParseException &e) {
		fl_alert(_("Parse Exception (config file %s error): %s"), config_filename, e.getError());
	}
	catch (const ConfigException &e) {
		fl_alert(_("Load Config Exception: %s"), e.what());
	}
}

void save_config() {

	Config cfg;

	char   config_filename[301];
	char * homedir;

	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}

	strncpy(config_filename, homedir, 300);
	strncat(config_filename, "/.flaxpdf", 300 - strlen(config_filename) - 1);

	try {
		Setting &root = cfg.getRoot();
		if (root.exists("recent_files"))
			root.remove("recent_files");

		root.add("version", Setting::TypeString) = CONFIG_VERSION;

		Setting &rf_setting = root.add("recent_files", Setting::TypeList);

		recent_file_struct *rf = recent_files;

		int cnt = 0;
		while ((rf != NULL) && (cnt < 10)) {
			Setting &gr = rf_setting.add(Setting::TypeGroup);

			gr.add("filename",   Setting::TypeString ) = rf->filename;
			gr.add("columns",    Setting::TypeInt    ) = rf->columns;
			gr.add("xoff",       Setting::TypeFloat  ) = rf->xoff;
			gr.add("yoff",       Setting::TypeFloat  ) = rf->yoff;
			gr.add("zoom",       Setting::TypeFloat  ) = rf->zoom;
			gr.add("zoom_mode",  Setting::TypeInt    ) = (int)rf->zoom_mode;
			gr.add("x",          Setting::TypeInt    ) = rf->x;
			gr.add("y",          Setting::TypeInt    ) = rf->y;
			gr.add("width",      Setting::TypeInt    ) = rf->width;
			gr.add("height",     Setting::TypeInt    ) = rf->height;
			gr.add("fullscreen", Setting::TypeBoolean) = rf->fullscreen;

			rf = rf->next;
			cnt += 1;
		}

		cfg.writeFile(config_filename);
	}
	catch (const FileIOException &e) {
		fl_alert(_("File IO Error: Unable to create or write config file %s."), config_filename);
	}
	catch (const ConfigException &e) {
		fl_alert(_("Save Config Exception: %s."), e.what());
	}
}

#ifdef TEST

main()
{
	printf("Begin. Config version: %s\n", CONFIG_VERSION.c_str());

	load_config();

	printf("Config file:\nVersion: %s\n\n", version.c_str());

	printf("recent file descriptors:\n\n");
	
	recent_file_struct *rf = recent_files;
	while (rf != NULL) {
		printf("filename: %s, columns: %d, xoff: %f, yoff: %f, zoom: %f, mode: %d\n",
			rf->filename.c_str(),
			rf->columns,
			rf->xoff,
			rf->yoff,
			rf->zoom,
			rf->zoom_mode);
		rf = rf->next;
	}
	printf("End\n");
}

#endif

