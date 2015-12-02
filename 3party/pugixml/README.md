pugixml [![Build Status](https://travis-ci.org/zeux/pugixml.svg?branch=master)](https://travis-ci.org/zeux/pugixml) [![Build status](https://ci.appveyor.com/api/projects/status/9hdks1doqvq8pwe7/branch/master?svg=true)](https://ci.appveyor.com/project/zeux/pugixml) [![codecov.io](http://codecov.io/github/zeux/pugixml/coverage.svg?branch=master)](http://codecov.io/github/zeux/pugixml?branch=master)
=======

pugixml is a C++ XML processing library, which consists of a DOM-like interface with rich traversal/modification
capabilities, an extremely fast XML parser which constructs the DOM tree from an XML file/buffer, and an XPath 1.0
implementation for complex data-driven tree queries. Full Unicode support is also available, with Unicode interface
variants and conversions between different Unicode encodings (which happen automatically during parsing/saving).

pugixml is used by a lot of projects, both open-source and proprietary, for performance and easy-to-use interface.

## Documentation

Documentation for the current release of pugixml is available on-line as two separate documents:

* [Quick-start guide](http://pugixml.org/docs/quickstart.html), that aims to provide enough information to start using the library;
* [Complete reference manual](http://pugixml.org/docs/manual.html), that describes all features of the library in detail.

You’re advised to start with the quick-start guide; however, many important library features are either not described in it at all or only mentioned briefly; if you require more information you should read the complete manual.

## License
This library is available to anybody free of charge, under the terms of MIT License:

Copyright (c) 2006-2015 Arseny Kapoulkine

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
