Mumpsimus
=========

An experiment.

A mumpsimus is a person who obstinately adheres to a custom or notion,
even after it's been shown to be unreasonable (alternatively, the word
may be used to describe the custom or notion itself).

It is said to have come from an illiterate 16th century priest, who
was mispronouncing the Latin word "sumpsimus". When told of the error,
he replied:

> I will not change my old mumpsimus for your new sumpsimus
> - [Oxford Dictionaries](http://oxforddictionaries.com/definition/english/mumpsimus)

This Mumpsimus is a tool set for re-writing HTTP messages before they
reach your browser or device. It will stubbornly mis-state both
requests and responses -- even when someone thinks that's a bad idea.

For example, we might use Mumpsimus to stubbornly insist that all
assets are cacheable. Or that none are. Or we might serve a local file
when a remote one is requested. Or we might randomly scramble the
Google Analytics tracking code, or strip out our Facebook cookie when
visiting third party sites. Oh the places we'll go!

The original idea was a tool for debugging during web development. If
that's your thing then check out the Charles Web Development
Proxy. It's very handy. Mumpsimus is a hacking toy.


How it Works
============

The toolset is designed to work as a set of UNIX command-line programs
that will co-operate using *UNIX pipes*. The basic idea is to feed in
HTTP messages (requests or responses) via *stdin*, and have the
(potentially) modified output printed on *stdout*. This in turn can be
fed to another command and so on.

Here's a simple example:

    $ mkfifo backpipe
    $ nc -l 8080  < backpipe | log | nc proxy 3128 | log -v > backpipe

*Log* is part of Mumpsimus. It prints information about HTTP messages
it sees on stderr. So the commands above do this:

  - Created a FIFO pipe called *backpipe* in the current directory.

  - Listens on port 8080 for browser requests (therefore set your
    proxy to localhost:8080)

  - Pipes the browser request through the *log* command, which will
    print one line on stderr per HTTP message

  - Pipes the request through another netcat instance which will
    forward it to a real proxy running on port 3128 (assumes the
    proxy's hostname is "proxy")

  - Pipes the response through another *log* command, this time also
    printing HTTP headers on stderr (-v turns this on)

  - The resulting output will be sent back to the browser (unchanged).

If you have [Mmap.org's Ncat tool](http://nmap.org/ncat/) (you *do*
have Ncat don't you?) this same chain is very easy:

    $ ncat -l -k localhost 8080 -c "log | ncat proxy 3128 | log -v"

Again, this assumes that "proxy" resolves to an actual proxy server
that listens on port 3128.

Let's play with the Cache-Control header!

    $ mkfifo backpipe
    $ nc -l 8080  < backpipe | nc proxy 3128 \
        | sed -e 's/^Cache-Control: .*/Cache-Control: private/' \
        | log -v > backpipe

This will mean that all requests going back to the browser will have
their Cache-Control header re-written to say "private", allowing our
browser to cache locally.

Actually, all we're doing above we could do already without
Mumpsimus. The problem with it is that any line beginning
"Cache-Control" will be re-written, not just headers. The *headers*
tool can help, since it knows HTTP:

    $ nc -l 8080  < backpipe | nc proxy 3128 \
        | headers -c "sed -e 's/^Cache-Control: .*/Cache-Control: private/'" \
        | log -v > backpipe

Now that _sed_ command will only apply to HTTP headers, because that's
what *headers* does.


Installation
============

    $ ./configure
    $ make
    $ make check     # optional but a good idea
    $ make install   # optional


Using the Tools
===============

Available
---------

* dup -- echo anything read on stdin to both stdout & stderr
* log -- print log messages on stderr
* headers -- pipe HTTP header through another command before passing
  it along
* body -- pipe HTTP message bodies through another command before
  passing it along. Re-calculates Content-Length header. Generally
  speaking, chunked encoding is decoded.

For testing only:

* noop -- do nothing. Echo stdin to stdout.
* null -- do nothing. Consume all stdin and print nothing.


TODO
----

See the file [TODO.md](TODO.md). See also [Github Issues
Tracker](https://github.com/hissohathair/mumpsimus/issues).


Requirements
============

The toolset is written in C, tested with the Clang & GCC compilers. So
far I've only tested UNIX-like environments (OS X, Ubuntu Linux &
OpenIndiana).

The test suite at the moment is written in Perl, and we'll need a
fairly modern version of that (at least 5.12) because of the test
harness being used.

Unit tests are done using the [Check Unit Testing Framework](http://check.sourceforge.net/).


Acknowledgements
================

HTTP parser (src/http_parser.c) is based on Igor Sysoev's NGINX
parser. Copyright Joyent.

Platforms missing a strlcat(3) will use Todd C Miller's strlcat
implementation.

Command behaviour is tested using Daniel Boorstein's Test::Command.

Source is hosted by [Github](http://github.com/hissohathair/mumpsimus)

CI provided by [Travis](https://travis-ci.org/hissohathair/mumpsimus/builds)
[![Build Status](https://travis-ci.org/hissohathair/mumpsimus.png)](https://travis-ci.org/hissohathir/mumpsimus)