confdir = $(sysconfdir)/hpssix

conf_DATA = hpssix.conf extfilter.conf

EXTRA_DIST = hpssix.conf.in extfilter.conf

edit = sed -e 's|@localstatedir[@]|$(localstatedir)|g'

hpssix.conf: hpssix.conf.in
	rm -f $@ $@.tmp
	$(edit) '$(srcdir)/$@.in' > $@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

extfilter.conf: extfilter.conf.in
	rm -f $@ $@.tmp
	$(edit) '$(srcdir)/$@.in' > $@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = hpssix.conf extfilter.conf
