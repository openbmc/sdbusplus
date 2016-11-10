if WANT_SDBUSPP

all-local:
	$(AM_V_at)$(PYTHON) ${top_builddir}/tools/setup.py build

clean-local:
	rm -rfv ${top_builddir}/build

install-exec-hook:
	$(AM_V_at)$(PYTHON) ${top_builddir}/tools/setup.py install \
        --prefix=$(DESTDIR)${prefix} \
        --install-data=$(DESTDIR)${datadir} \
        --install-lib=$(DESTDIR)$(PYTHONDIR) \
        --install-scripts=$(DESTDIR)${bindir} \
        --record=${top_builddir}/python_install.log

uninstall-hook:
	cat ${top_builddir}/python_install.log \
        | awk '{print "$(DESTDIR)"$$1}' | xargs rm -fv

endif
