liboauthcpp
-----------

liboauthcpp is a pure C++ library for performing OAuth requests. It
doesn't contain any networking code -- you provide for performing HTTP
requests yourself, however you like -- instead focusing on performing
OAuth-specific functionality and providing a nice interface for it.
If you already have infrastructure for making HTTP requests and are
looking to add OAuth support, liboauthcpp is for you.

liboauthcpp currently implements OAuth 1.0a (see
http://tools.ietf.org/html/rfc5849).

Buildbot
--------
[![Build Status](https://secure.travis-ci.org/sirikata/liboauthcpp.png)](http://travis-ci.org/sirikata/liboauthcpp)

Requirements
------------

You should only need:

 * CMake
 * A C++ compiler for your platform (e.g. g++, Microsoft Visual C++)

Compiling
---------

The build process is simple:

    cd liboauthcpp
    cd build
    cmake .
    make            # or open Visual Studio and build the solution

If your own project uses CMake you can also include
build/CMakeLists.txt directly into your project and reference the
target "oauthcpp", a static library, in your project.

Percent (URL) Encoding
----------------------

To get correct results, you need to pass your URL properly encoded to
liboauthcpp. If you are not at all familiar, you should probably start
by reading the [URI Spec](http://tools.ietf.org/html/rfc3986), especially
Section 2. Alternatively,
[this article](http://blog.lunatech.com/2009/02/03/what-every-web-developer-must-know-about-url-encoding)
gives a more readable overview.

The basic idea is that there are 3 classes of characters: reserved,
unreserved, and other. Reserved characters are special characters that
are used in the URI syntax itself, e.g. ':' (after the scheme), '/'
(the hierarchical path separator), and '?'  (prefixing the query
string). Unreserved characters are characters that are always safe to
include unencoded, e.g. the alphanumerics. Other characters must
always be encoded, mainly covering special characters like ' ', '<' or
'>', and '{' or '}'.

The basic rule is that reserved characters must be encoded if they
appear in any part of the URI when not being used as a
separator. Unreserved characters are always safe. And the other
characters they didn't know if they would be safe or not so they must
always be encoded.

Unfortunately, the reserved set is a bit more complicated. They are
broken down into 'general delimiters' and 'sub delimiters'. The ones
already mentioned, like ':', can appear in many forms of URIs (say,
http, ftp, about, gopher, mailto, etc. Those are called general
delimiters. Others (e.g. '(', ')', '!', '$', '+', ',', '=', and more)
are called subdelimiters because their use depends on the URI
scheme. Worse, their use depends on the *part of the URI*. Depending
on the particular URI scheme, these may or may not have to be encoded,
and it might also depend on where they appear. (As an example, an '&'
in an http URI isn't an issue if it appears in the path -- before the
query string -- i.e. before a '?' appears. Worse, '=' can appear unencoded in
the path, or in a query parameter value, but not in a query parameter key since
it would be interpreted as the end of the key.)

*Additionally*, in many cases it is permitted to encode a character
unnecessarily and the result is supposed to be the same. This means
that it's possible to percent encode some URLs in multiple ways
(e.g. encoding the unreserved set unnecessarily). It is possible, but not
guaranteed, that if you pass *exactly* the same URI to liboauthcpp and the
OAuth server, it will handle it regardless of the variant of encoding, so long
as it is a valid encoding.

The short version: percent encoding a URL properly is non-trivial and
you can even encode the same URL multiple ways, but has to be done
correctly so that the OAuth signature can be computed. Sadly,
"correctly" in this case really means "in whatever way the server your
interacting with wants it encoded".

Internally, liboauthcpp needs to do another step of percent encoding,
but the OAuth spec is very precise about how that works (none of these
scheme-dependent issues). liboauth applies this percent encoding, but
assumes that you have encoded your URLs properly. This assumption
makes sense since the actual request is made separately, and the URI
has to be specified in it, so you should already have a form which the
server will accept.

However, in order to aid you, a very simple percent encoding API is exposed. It
should help you encode URLs minimally and in a way that many services accept. In
most cases you should use `HttpPercentEncodePath()`,
`HttpPercentEncodeQueryKey()`, and `HttpPercentEncodeQueryValue()` to encode
those parts of your http URL, then combine them and pass them to liboauthcpp for
signing.


Thread Safety
-------------

liboauthcpp doesn't provide any thread safety guarantees. That said, there is
very little shared state, and some classes (e.g. Consumer) are naturally
immutable and therefore thread safe. Similarly, nearly the entire library uses
no static/shared state, so as long as you create separate objects for separate
threads, you should be safe.

The one exception is nonces: the Client class needs to generate a nonce for
authorization. To do so, the random number generator needs to be seeded. We do
this with the current time, but fast, repeated use of the Client class from
different threads could result in the same nonce. To avoid requiring an entire
thread library just for this one case, you can call Client::initialize()
explicitly before using the Client from multiple threads. For single-threaded
use, you are not required to call it.

Demos
-----
There are two demos included in the demos/ directory, and they are built by
default with the instructions above. In both, you enter key/secret information
and it generates URLs for you to visit (in a browser) and copy data back into
the program.

simple_auth should be executed first. It starts with only a consumer key and
secret and performs 3-legged auth: you enter in consumer keys, it generates URLs
to authenticate the user and generate access tokens. It requires 3 steps:
request_token, authorize, and access_token (which correspond the URLs
accessed). At the end of this process, you'll be provided an access key/secret
pair which you can use to access actual resources.

simple_request actually does something useful now that your application is
authorized. Enter your consumer key/secret and the access key/secret from
simple_auth (or which you've generated elsewhere) and it will generate a URL you
can use to access your home timeline in JSON format. It adds a parameter to ask
for only 5 entries (demonstrating that signing works properly over additional
query parameters). This is a one-step process -- it just gives you the URL and
you get the results in your browser.

In both, the URLs accessed are specified at the top of the demo
files. simple_auth requires URLs for request_token, authorize_url, and
access_token. Some providers require additional parameters (notably an
oauth_callback for Twitter, even if its out of band, or oob), which you can also
specify in that location. simple_request only needs the URL of the resource
being accessed (i.e. the URL for the home_timeline JSON data used by default in
the demo), with optional parameters stored as a query string.

Both demos only use GET requests with query strings, but all HTTP methods
(e.g. POST, PUT, DELETE) and approaches to sending parameters (e.g. HTTP
headers, url-encoded body) should be supported in the API.

License
-------

liboauthcpp is MIT licensed. See the LICENSE file for more details.

liboauthcpp is mostly taken from libtwitcurl
(http://code.google.com/p/twitcurl/), which is similarly licensed. It
mostly serves to isolate the OAuth code from libtwitcurl's Twitter and
cURL specific code.

libtwitcurl also borrowed code from other projects:
twitcurl uses HMAC_SHA1 from http://www.codeproject.com/KB/recipes/HMACSHA1class.aspx
twitcurl uses base64 from http://www.adp-gmbh.ch/cpp/common/base64.html
