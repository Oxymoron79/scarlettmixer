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

if HAVE_DOXYGEN
doxyfile.stamp:
	@-echo "Generate developer documentation."; \
	$(DOXYGEN) Doxyfile; \
	touch doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp

clean-local:
	@-echo "Clean developer documentation."; \
	rm -rf ${top_builddir}/html

install-data-local: doxyfile.stamp
	@-installfiles=`echo $(builddir)/html/*`; \
	if test "$$installfiles" = '$(builddir)/html/*'; then \
		echo 1>&2 "No documentation to install!"; \
	else \
		installdir="$(docdir)/html"; \
		echo "Install developer documentation to $${installdir}"; \
		$(mkinstalldirs) $${installdir}; \
		for i in $$installfiles; do \
			$(INSTALL_DATA) $$i $${installdir}; \
		done; \
	fi

uninstall-local:
	@-echo "Uninstall developer documentation from $(docdir)."; \
	rm -rf $(docdir)

endif
