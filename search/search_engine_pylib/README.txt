This document describes how to use this module.

1. How to build?

   To build the module you need Python2.7 and Boost Python.  Also, you
   need Qt5.5 (or higher), but you need it in any case, if you're
   planning to build the project.  On MacOS, Python2.7 should be
   installed by default, to get Boost via Brew just type in the shell:

     brew update
     brew install boost --with-python
     brew install boost-python

   On Debian, type in the shell:

     sudo apt-get install libboost-*

   Note that on MacOS Boost is built by default with libc++, on Debian
   Boost is built by default with libstdc++. Therefore, you can use
   only macx-clang and linux-clang specs correspondingly. It's wrong
   to use linux-clang-libc++ because it's generally a bad idea to have
   two implementations of the C++ standard library in the same
   application.

   Then, invoke qmake from the shell, for example:

     qmake CONFIG+=search_engine_pylib path-to-omim.pro
     make -k -j8 all

2. How to use?

   As search_engine_pylib is a custom Python module, all that you need
   is to customize PYTHONPATH environment variable before running your
   scripts. For example:

     PYTHONPATH=path-to-the-directory-with-search_engine_pylib.so \
       ./search/search_engine_pylib/run_search_engine.py
