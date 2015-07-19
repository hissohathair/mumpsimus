Mumpsimus Todo
==============

See also [Github Issues
Tracker](https://github.com/hissohathair/mumpsimus/issues).


Demonstrations
--------------

* Log all the information sent to Google and Facebook.
* Scramble / hack all the information sent to Google and Facebook.
* Write a "daily summary" of your web browsing.
* Listen in on HTTPS traffic.
* Determine if an iOS app is sending data to advertisers.
* "Hack" an iOS game by changing the information it receives.
* When browsing Google Scholar, extract h-index and top publications
  when looking at researcher profiles.



Existing Tools
--------------

* body
  * Chunked-encoding output option.
  * Filter by MIME type or URL.
* headers
  * Filter by MIME type or URL.



New Tools
---------

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


Experiments
-----------

* Port to Go or Rust?
* Python or Ruby prototypes?
