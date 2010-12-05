/*
** Copyright (c) 2001-2007 Expat maintainers.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
** 
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <proto/exec.h>

struct Library* ExpatBase = 0;
struct ExpatIFace* IExpat = 0;


void setup() __attribute__((constructor));
void cleanup() __attribute__((destructor));


void setup()
{
	ExpatBase = OpenLibrary("expat.library", 4);
	IExpat = (struct ExpatIFace*)GetInterface(ExpatBase, "main", 1, NULL);
	if ( IExpat == 0 )  {
		DebugPrintF("Can't open expat.library\n");
	}
}


void cleanup()
{
	if ( IExpat != 0 )  {
		DropInterface((struct Interface*)IExpat);
		IExpat = 0;
	}

	if ( ExpatBase != 0 )  {
		CloseLibrary(ExpatBase);
		ExpatBase = 0;
	}
}
