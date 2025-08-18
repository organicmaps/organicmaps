//
// Copyright (c) 2009 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

// Version 1.0 - Initial version
// Version 1.1 - Fixed path parsing, implemented curves, implemented circle.

#ifndef NANOSVG_H
#define NANOSVG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Example Usage:
	// Load
	struct SVGPath* plist;
	plist = svgParseFromFile("test.svg.");
	// Use...
	for (SVGPath* it = plist; it; it = it->next)
		...
	// Delete
	svgDelete(plist);
*/

struct SVGPath
{
	float* pts;
	int npts;
	unsigned int fillColor;
	unsigned int strokeColor;
	float strokeWidth;
	char hasFill;
	char hasStroke;
	char closed;
	struct SVGPath* next;
};

// Parses SVG file from a file, returns linked list of paths.
struct SVGPath* svgParseFromFile(const char* filename);

// Parses SVG file from a null terminated string, returns linked list of paths.
struct SVGPath* svgParse(char* input);

// Deletes list of paths.
void svgDelete(struct SVGPath* plist);

#ifdef __cplusplus
};
#endif

#endif // NANOSVG_H
