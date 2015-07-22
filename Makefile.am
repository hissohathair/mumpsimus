ACLOCAL_AMFLAGS = -I ./m4

SUBDIRS = src unit-tests system-tests

# for "make test"
test: check

# for "make dist"
EXTRA_DIST = exp/perl/App-TTT.tar.gz $(TESTS) 
dist-hook:
	chmod u+w $(distdir)/t
	rm -rf `find . -type f -name \*~ -o -name \*.trs -o -name \*.log`

exp/perl/App-TTT.tar.gz:
	(cd exp/perl && perl Build.PL && ./Build dist distversion=$(VERSION))
	cp exp/perl/App-TTT-$(VERSION).tar.gz exp/perl/App-TTT.tar.gz

realclean: clean
	 rm -rf Makefile Makefile.in aclocal.m4 autom4te.cache build-aux/compile build-aux/depcomp build-aux/install-sh build-aux/missing build-aux/tap-driver.sh build-aux/test-driver config.log config.status configure src/.deps src/.dirstamp src/Makefile src/Makefile.in src/config.h src/config.h.in src/stamp-h1 system-tests/Makefile system-tests/Makefile.in system-tests/*.log system-tests/*.trs unit-tests/\$\(* unit-tests/.deps/ unit-tests/Makefile unit-tests/Makefile.in unit-tests/*.log unit-tests/*.trs
