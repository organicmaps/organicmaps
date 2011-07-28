***************
Getting Started
***************

.. highlight:: c

Compiling and Installing Jansson
================================

The Jansson source is available at
http://www.digip.org/jansson/releases/.

Unix-like systems
-----------------

Unpack the source tarball and change to the source directory:

.. parsed-literal::

    bunzip2 -c jansson-|release|.tar.bz2 | tar xf -
    cd jansson-|release|

The source uses GNU Autotools (autoconf_, automake_, libtool_), so
compiling and installing is extremely simple::

    ./configure
    make
    make check
    make install

To change the destination directory (``/usr/local`` by default), use
the ``--prefix=DIR`` argument to ``./configure``. See ``./configure
--help`` for the list of all possible installation options. (There are
no options to customize the resulting Jansson binary.)

The command ``make check`` runs the test suite distributed with
Jansson. This step is not strictly necessary, but it may find possible
problems that Jansson has on your platform. If any problems are found,
please report them.

If you obtained the source from a Git repository (or any other source
control system), there's no ``./configure`` script as it's not kept in
version control. To create the script, the build system needs to be
bootstrapped. There are many ways to do this, but the easiest one is
to use ``autoreconf``::

    autoreconf -vi

This command creates the ``./configure`` script, which can then be
used as described above.

.. _autoconf: http://www.gnu.org/software/autoconf/
.. _automake: http://www.gnu.org/software/automake/
.. _libtool: http://www.gnu.org/software/libtool/


Other Systems
-------------

On Windows and other non Unix-like systems, you may be unable to run
the ``./configure`` script. In this case, follow these steps. All the
files mentioned can be found in the ``src/`` directory.

1. Create ``jansson_config.h``. This file has some platform-specific
   parameters that are normally filled in by the ``./configure``
   script:

   - On Windows, rename ``jansson_config.h.win32`` to ``jansson_config.h``.

   - On other systems, edit ``jansson_config.h.in``, replacing all
     ``@variable@`` placeholders, and rename the file to
     ``jansson_config.h``.

2. Make ``jansson.h`` and ``jansson_config.h`` available to the
   compiler, so that they can be found when compiling programs that
   use Jansson.

3. Compile all the ``.c`` files (in the ``src/`` directory) into a
   library file. Make the library available to the compiler, as in
   step 2.


Building the Documentation
--------------------------

(This subsection describes how to build the HTML documentation you are
currently reading, so it can be safely skipped.)

Documentation is in the ``doc/`` subdirectory. It's written in
reStructuredText_ with Sphinx_ annotations. To generate the HTML
documentation, invoke::

   make html

and point your browser to ``doc/_build/html/index.html``. Sphinx_ 1.0
or newer is required to generate the documentation.

.. _reStructuredText: http://docutils.sourceforge.net/rst.html
.. _Sphinx: http://sphinx.pocoo.org/


Compiling Programs that Use Jansson
===================================

Jansson involves one C header file, :file:`jansson.h`, so it's enough
to put the line

::

    #include <jansson.h>

in the beginning of every source file that uses Jansson.

There's also just one library to link with, ``libjansson``. Compile and
link the program as follows::

    cc -o prog prog.c -ljansson

Starting from version 1.2, there's also support for pkg-config_::

    cc -o prog prog.c `pkg-config --cflags --libs jansson`

.. _pkg-config: http://pkg-config.freedesktop.org/
