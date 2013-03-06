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
> [Oxford](http://oxforddictionaries.com/definition/english/mumpsimus)

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
Proxy. It's very handy. Mumpsimus is still a toy.


How it Works
============

The toolset is designed to work as a set of UNIX command-line programs
that will co-operate using *UNIX pipes*. The basic idea is to feed in
HTTP messages (requests or responses) via *stdin*, and have the
(potentially) modified output printed on *stdout*. This in turn can be
fed to another commnad and so on.

Here's a simple example:

    $ mkfifo backpipe
    $ nc -l 8080  < backpipe | log | nc proxy 3128 | log -v > backpipe

Or, if you have [Mmap.org's Ncat tool](http://nmap.org/ncat/):

    $ ncat -l -k localhost 8080 -c "log | ncat proxy 3128 | log -v"

*Log* is part of Mumpsimus. It prints information about HTTP messages
it sees on stderr. So these commands (the two blocks above are
functionally equivalent):

  - Listens on port 8080 for browser requests (therefore set your
    proxy to localhost:8080)

  - Pipes the browser request through the *log* command, which will
    print one line on stderr per request

  - Pipes the request through another netcat instance which will
    forward it to a real proxy running on port 3128 (assumes the
    proxy's hostname is "proxy")

  - Pipes the response through another *log* command, this time also
    printing HTTP headers (-v turns this on)

  - The resulting output will be sent back to the browser.

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
"Cache-Control" will be re-written, not just headers. The *pipeif*
tool can help, since it knows HTTP:

    $ nc -l 8080  < backpipe | nc proxy 3128 \
        | pipeif -h -c "sed -e 's/^Cache-Control: .*/Cache-Control: private/'" \
        | log -v > backpipe

Now that _sed_ command will only apply to HTTP headers, because that's
what *pipeif* does.


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
* pipeif -- pipe HTTP header or body through another command

For testing only:

* noop -- do nothing. Echo stdin to stdout.
* null -- do nothing. Consume all stdin and print nothing.


TODO
----

Not yet written, although often have a Perl prototype in the "exp"
directory:

* cache -- save a local copy to disk, and use that copy for subsequent
  requests
* connect -- fetch a resource
* get -- generate a sample HTTP request
* insert_header -- insert a new HTTP header
* no_cache -- disable cacheing
* proxy -- listen for browser requests
* remove_header -- remove a header line
* spoof -- request a resource from another server


Requirements
============

The toolset is written in C, tested with the Clang & GCC compilers. So
far I've only tested UNIX-like environments (OS X, Ubuntu Linux &
OpenIndiana).

The test suite at the moment is written in Perl, and we'll need a
fairly modern version of that (at least 5.12) because of the test
harness being used.


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