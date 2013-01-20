SUBDIRS = src t

EXTRA_DIST = exp/perl/App-TTT-1.tar.gz
dist-hook:
	(cd t && make testclean)
	(cd exp/perl && perl Build.PL && ./Build dist)

test:
	(cd src && make test)

