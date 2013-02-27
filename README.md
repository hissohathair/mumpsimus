Mumpsimus
=========

An experiment.


Requirements
============

The test suite at the moment is written in Perl, and we'll need a
fairly modern version of that (at least 5.12).


Acknowledgements
================

HTTP parser (src/http_parser.c) is based on Igor Sysoev's NGINX
parser. Copyright Joyent.

Platforms missing a strlcat(3) will use Todd C Miller's strlcat
implementation.

Command behaviour is tested using Daniel Boorstein's Test::Command.
