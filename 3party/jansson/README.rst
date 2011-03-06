Jansson README
==============

Jansson_ is a C library for encoding, decoding and manipulating JSON
data. Its main features and design principles are:

- Simple and intuitive API and data model

- Comprehensive documentation

- No dependencies on other libraries

- Full Unicode support (UTF-8)

- Extensive test suite

Jansson is licensed under the `MIT license`_; see LICENSE in the
source distribution for details.


Compilation and Installation
----------------------------

If you obtained a source tarball, just use the standard autotools
commands::

   $ ./configure && make && make install

If the source has been checked out from a Git repository, the
./configure script has to be generated fist. The easiest way is to use
autoreconf::

   $ autoreconf -i

To run the test suite, invoke::

   $ make check


Documentation
-------------

Documentation is in the ``doc/`` subdirectory. It's written in
reStructuredText_ with Sphinx_ annotations, so reading it in plain may
be inconvenient. For this reason, prebuilt HTML documentation is
available at http://www.digip.org/jansson/doc/.

To generate HTML documentation yourself, invoke::

   make html

and point your browser to ``doc/_build/html/index.html``. Sphinx_ is
required to generate the documentation.


.. _Jansson: http://www.digip.org/jansson/
.. _`MIT license`: http://www.opensource.org/licenses/mit-license.php
.. _reStructuredText: http://docutils.sourceforge.net/rst.html
.. _Sphinx: http://sphinx.pocoo.org/
