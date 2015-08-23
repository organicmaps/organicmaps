Libtess2
========
Version 1.0.1


This is refactored version of the original libtess which comes with the GLU reference implementation. The code is good quality polygon tesselator and triangulator. The original code comes with rather horrible interface and its' performance suffers from lots of small memory allocations. The main point of the refactoring has been the interface and memory allocation scheme.

A lot of the GLU tesselator documentation applies to Libtess2 too (apart from the API), check out http://www.glprogramming.com/red/chapter11.html

Simple bucketed memory allocator (see Graphics Gems III for reference) was added which speeds up the code by order of magnitude (tests showed 15 to 50 times improvement depending on data). The API allows the user to pass his own allocator to the library. It is possible to configure the library so that the library runs on predefined chunk of memory.

The API was changed to loosely resemble the OpenGL vertex array API. The processed data can be accessed via getter functions. The code is able to output contours, polygons and connected polygons. The output of the tesselator can be also used as input for new run. I.e. the user may first want to calculate an union all the input contours and the triangulate them.

The code is released under SGI FREE SOFTWARE LICENSE B Version 2.0.
http://oss.sgi.com/projects/FreeB/


Mikko Mononen
memon@inside.org
