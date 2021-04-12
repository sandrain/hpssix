pkglibexec_SCRIPTS = hpssix-db2-scan.sh

EXTRA_DIST = hpssix-db2-scan.sh.in

edit = sed -e 's|@sysconfdir[@]|$(sysconfdir)|g'

hpssix-db2-scan.sh: hpssix-db2-scan.sh.in
	rm -f $@ $@.tmp
	$(edit) '$(srcdir)/$@.in' > $@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = $(pkglibexec_SCRIPTS)

