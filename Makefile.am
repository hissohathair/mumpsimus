SUBDIRS = src t

# for "make check"
TESTS = t/01noop.t t/02log.t
TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
		  $(top_srcdir)/build-aux/tap-driver.sh

# for "make test"
test:
	(cd src && make test)

# for "make dist"
EXTRA_DIST = exp/perl/App-TTT.tar.gz $(TESTS) 
dist-hook:
	chmod u+w $(distdir)/t
	rm -rf `find . -type f -name \*~ -o -name \*.trs -o -name \*.log`

exp/perl/App-TTT.tar.gz:
	(cd exp/perl && perl Build.PL && ./Build dist distversion=$(VERSION))
	cp exp/perl/App-TTT-$(VERSION).tar.gz exp/perl/App-TTT.tar.gz

