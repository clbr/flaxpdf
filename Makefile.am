SUBDIRS = po src

ACLOCAL_AMFLAGS = -I m4

dist_pkgdata_DATA = icon64.png icon128.png icon256.png

EXTRA_DIST = config/config.rpath flaxpdf.desktop

install-data-hook: iconsdir = $(DESTDIR)$(datadir)/icons/hicolor
install-data-hook:
	install -D -m644 icon64.png $(iconsdir)/64x64/apps/flaxpdf.png
	install -D -m644 icon128.png $(iconsdir)/128x128/apps/flaxpdf.png
	install -D -m644 icon256.png $(iconsdir)/256x256/apps/flaxpdf.png
	install -D -m644 flaxpdf.desktop $(DESTDIR)$(datadir)/applications/flaxpdf.desktop
