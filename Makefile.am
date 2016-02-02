SUBDIRS=src

iconsdir = $(datadir)/icons/hicolor
desktopdir = $(datadir)/applications

nobase_dist_icons_DATA = scalable/apps/ScarlettMixer.svg
dist_desktop_DATA = ScarlettMixer.desktop

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updateing GTK icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After (un)install run:"; \
		echo "*** $(gtk_update_icon_cache)"; \
	fi
