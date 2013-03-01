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

