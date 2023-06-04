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

// The SVG parser is based on Anti-Graim Geometry SVG example
// Copyright (C) 2002-2004 Maxim Shemanarev (McSeem)

#include "nanosvg.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#ifndef M_PI
	#define M_PI 3.14159265358979323846264338327
#endif

#ifdef _MSC_VER
	#pragma warning (disable: 4996) // Switch off security warnings
#endif

// Simple XML parser

#define TAG 1
#define CONTENT 2
#define MAX_ATTRIBS 256

static void parseContent(char* s,
						 void (*contentCb)(void* ud, const char* s),
						 void* ud)
{
	// Trim start white spaces
	while (*s && isspace(*s)) s++;
	if (!*s) return;

	if (contentCb)
		(*contentCb)(ud, s);
}

static void parseElement(char* s,
						 void (*startelCb)(void* ud, const char* el, const char** attr),
						 void (*endelCb)(void* ud, const char* el),
						 void* ud)
{
	const char* attr[MAX_ATTRIBS];
	int nattr = 0;
	char* name;
	int start = 0;
	int end = 0;

	// Skip white space after the '<'
	while (*s && isspace(*s)) s++;

	// Check if the tag is end tag
	if (*s == '/')
	{
		s++;
		end = 1;
	}
	else
	{
		start = 1;
	}

	// Skip comments, data and preprocessor stuff.
	if (!*s || *s == '?' || *s == '!')
		return;

	// Get tag name
	name = s;
	while (*s && !isspace(*s)) s++;
	if (*s) { *s++ = '\0'; }

	// Get attribs
	while (!end && *s && nattr < MAX_ATTRIBS-1)
	{
		// Skip white space before the attrib name
		while (*s && isspace(*s)) s++;
		if (!*s) break;
		if (*s == '/')
		{
			end = 1;
			break;
		}
		attr[nattr++] = s;
		// Find end of the attrib name.
		while (*s && !isspace(*s) && *s != '=') s++;
		if (*s) { *s++ = '\0'; }
		// Skip until the beginning of the value.
		while (*s && *s != '\"') s++;
		if (!*s) break;
		s++;
		// Store value and find the end of it.
		attr[nattr++] = s;
		while (*s && *s != '\"') s++;
		if (*s) { *s++ = '\0'; }
	}

	// List terminator
	attr[nattr++] = 0;
	attr[nattr++] = 0;

	// Call callbacks.
	if (start && startelCb)
		(*startelCb)(ud, name, attr);
	if (end && endelCb)
		(*endelCb)(ud, name);
}

int parsexml(char* input,
			 void (*startelCb)(void* ud, const char* el, const char** attr),
			 void (*endelCb)(void* ud, const char* el),
			 void (*contentCb)(void* ud, const char* s),
			 void* ud)
{
	char* s = input;
	char* mark = s;
	int state = CONTENT;
	while (*s)
	{
		if (*s == '<' && state == CONTENT)
		{
			// Start of a tag
			*s++ = '\0';
			parseContent(mark, contentCb, ud);
			mark = s;
			state = TAG;
		}
		else if (*s == '>' && state == TAG)
		{
			// Start of a content or new tag.
			*s++ = '\0';
			parseElement(mark, startelCb, endelCb, ud);
			mark = s;
			state = CONTENT;
		}
		else
			s++;
	}

	return 1;
}


/* Simple SVG parser. */

#define SVG_MAX_ATTR 128

struct SVGAttrib
{
	float xform[6];
	unsigned int fillColor;
	unsigned int strokeColor;
	float fillOpacity;
	float strokeOpacity;
	float strokeWidth;
	char hasFill;
	char hasStroke;
	char visible;
};

struct SVGParser
{
	struct SVGAttrib attr[SVG_MAX_ATTR];
	int attrHead;
	float* buf;
	int nbuf;
	int cbuf;
	struct SVGPath* plist;
	char pathFlag;
	char defsFlag;
	float tol;
};

static void xformSetIdentity(float* t)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void xformSetTranslation(float* t, float tx, float ty)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = tx; t[5] = ty;
}

static void xformSetScale(float* t, float sx, float sy)
{
	t[0] = sx; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = sy;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void xformMultiply(float* t, float* s)
{
	float t0 = t[0] * s[0] + t[1] * s[2];
	float t2 = t[2] * s[0] + t[3] * s[2];
	float t4 = t[4] * s[0] + t[5] * s[2] + s[4];
	t[1] = t[0] * s[1] + t[1] * s[3];
	t[3] = t[2] * s[1] + t[3] * s[3];
	t[5] = t[4] * s[1] + t[5] * s[3] + s[5];
	t[0] = t0;
	t[2] = t2;
	t[4] = t4;
}

static void xformPremultiply(float* t, float* s)
{
	float s2[6];
	memcpy(s2, s, sizeof(float)*6);
	xformMultiply(s2, t);
	memcpy(t, s2, sizeof(float)*6);
}

static struct SVGParser* svgCreateParser()
{
	struct SVGParser* p;
	p = (struct SVGParser*)malloc(sizeof(struct SVGParser));
	if (!p)
		return NULL;
	memset(p, 0, sizeof(struct SVGParser));

	// Init style
	xformSetIdentity(p->attr[0].xform);
	p->attr[0].fillColor = 0;
	p->attr[0].strokeColor = 0;
	p->attr[0].fillOpacity = 1;
	p->attr[0].strokeOpacity = 1;
	p->attr[0].strokeWidth = 1;
	p->attr[0].hasFill = 0;
	p->attr[0].hasStroke = 0;
	p->attr[0].visible = 1;

	return p;
}

static void svgDeleteParser(struct SVGParser* p)
{
	struct SVGPath* path;
	struct SVGPath* next;
	path = p->plist;
	while (path)
	{
		next = path->next;
		if (path->pts)
			free(path->pts);
		free(path);
		path = next;
	}
	if (p->buf)
		free(p->buf);
	free(p);
}

static void svgResetPath(struct SVGParser* p)
{
	p->nbuf = 0;
}

static void svgPathPoint(struct SVGParser* p, float x, float y)
{
	int cap;
	float* buf;
	if (p->nbuf+1 > p->cbuf)
	{
		cap = p->cbuf ? p->cbuf*2 : 8;
		buf = (float*)malloc(cap*2*sizeof(float));
		if (!buf) return;
		if (p->nbuf)
			memcpy(buf, p->buf, p->nbuf*2*sizeof(float));
		if (p->buf)
			free(p->buf);
		p->buf = buf;
		p->cbuf = cap;
	}
	p->buf[p->nbuf*2+0] = x;
	p->buf[p->nbuf*2+1] = y;
	p->nbuf++;
}

static struct SVGAttrib* svgGetAttr(struct SVGParser* p)
{
	return &p->attr[p->attrHead];
}

static void svgPushAttr(struct SVGParser* p)
{
	if (p->attrHead < SVG_MAX_ATTR-1)
	{
		p->attrHead++;
		memcpy(&p->attr[p->attrHead], &p->attr[p->attrHead-1], sizeof(struct SVGAttrib));
	}
}

static void svgPopAttr(struct SVGParser* p)
{
	if (p->attrHead > 0)
		p->attrHead--;
}

static void svgCreatePath(struct SVGParser* p, char closed)
{
	float* t;
	float* pt;
	struct SVGAttrib* attr;
	struct SVGPath* path;
	int i;

	if (!p)
		return;

	if (!p->nbuf)
	{
		return;
	}

	attr = svgGetAttr(p);

	path = (struct SVGPath*)malloc(sizeof(struct SVGPath));
	if (!path)
		return;
	memset(path, 0, sizeof(struct SVGPath));
	path->pts = (float*)malloc(p->nbuf*2*sizeof(float));
	if (!path->pts)
	{
		free(path);
		return;
	}
	path->closed = closed;
	path->npts = p->nbuf;

	path->next = p->plist;
	p->plist = path;

	// Transform path.
	t = attr->xform;
	for (i = 0; i < p->nbuf; ++i)
	{
		pt = &p->buf[i*2];
		path->pts[i*2+0] = pt[0]*t[0] + pt[1]*t[2] + t[4];
		path->pts[i*2+1] = pt[0]*t[1] + pt[1]*t[3] + t[5];
	}

	path->hasFill = attr->hasFill;
	path->hasStroke = attr->hasStroke;
	path->strokeWidth = attr->strokeWidth * t[0];

	path->fillColor = attr->fillColor;
	if (path->hasFill)
		path->fillColor |= (unsigned int)(attr->fillOpacity*255) << 24;

	path->strokeColor = attr->strokeColor;
	if (path->hasStroke)
		path->strokeColor |= (unsigned int)(attr->strokeOpacity*255) << 24;
}

static int isnum(char c)
{
	return strchr("0123456789+-.eE", c) != 0;
}

/*static const char* parsePathFloats(const char* s, float* arg, int n)
{
	char num[64];
	const char* start;
	int nnum;
	int i = 0;
	while (*s && i < n)
	{
		// Skip white spaces and commas
		while (*s && (isspace(*s) || *s == ',')) s++;
		if (!*s) break;
		start = s;
		nnum = 0;
		while (*s && isnum(*s))
		{
			if (nnum < 63) num[nnum++] = *s;
			s++;
		}
		num[nnum] = '\0';
		arg[i++] = (float)atof(num);
	}
	return s;
}*/


static const char* getNextPathItem(const char* s, char* it)
{
	int i = 0;
	it[0] = '\0';
	// Skip white spaces and commas
	while (*s && (isspace(*s) || *s == ',')) s++;
	if (!*s) return s;
	if (*s == '-' || *s == '+' || isnum(*s))
	{
		while (*s == '-' || *s == '+')
		{
			if (i < 63) it[i++] = *s;
			s++;
		}
		while (*s && *s != '-' && *s != '+' && isnum(*s))
		{
			if (i < 63) it[i++] = *s;
			s++;
		}
		it[i] = '\0';
	}
	else
	{
		it[0] = *s++;
		it[1] = '\0';
		return s;
	}
	return s;
}


static unsigned int parseColor(const char* str)
{
	unsigned c = 0;
	while(*str == ' ') ++str;
	if (*str == '#')
		sscanf(str + 1, "%x", &c);
	return c;
}

static float parseFloat(const char* str)
{
	while (*str == ' ') ++str;
	return (float)atof(str);
}

static int parseTransformArgs(const char* str, float* args, int maxNa, int* na)
{
	const char* end;
	const char* ptr;

	*na = 0;
	ptr = str;
	while (*ptr && *ptr != '(') ++ptr;
	if (*ptr == 0)
		return 1;
	end = ptr;
	while (*end && *end != ')') ++end;
	if (*end == 0)
		return 1;

	while (ptr < end)
	{
		if (isnum(*ptr))
		{
			if (*na >= maxNa) return 0;
			args[(*na)++] = (float)atof(ptr);
			while (ptr < end && isnum(*ptr)) ++ptr;
		}
		else
		{
			++ptr;
		}
	}
	return (int)(end - str);
}

static int svgParseMatrix(struct SVGParser* p, const char* str)
{
	float t[6];
	int na = 0;
	int len = parseTransformArgs(str, t, 6, &na);
	if (na != 6) return len;
	xformPremultiply(svgGetAttr(p)->xform, t);
	return len;
}

static int svgParseTranslate(struct SVGParser* p, const char* str)
{
	float args[2];
	float t[6];
	int na = 0;
	int len = parseTransformArgs(str, args, 2, &na);
	if (na == 1) args[1] = 0.0;
	xformSetTranslation(t, args[0], args[1]);
	xformPremultiply(svgGetAttr(p)->xform, t);
	return len;
}

static int svgParseScale(struct SVGParser* p, const char* str)
{
	float args[2];
	int na = 0;
	float t[6];
	int len = parseTransformArgs(str, args, 2, &na);
	if (na == 1) args[1] = args[0];
	xformSetScale(t, args[0], args[1]);
	xformPremultiply(svgGetAttr(p)->xform, t);
	return len;
}

static void svgParseTransform(struct SVGParser* p, const char* str)
{
	while (*str)
	{
		if (strncmp(str, "matrix", 6) == 0)
			str += svgParseMatrix(p, str);
		else if (strncmp(str, "translate", 9) == 0)
			str += svgParseTranslate(p, str);
		else if (strncmp(str, "scale", 5) == 0)
			str += svgParseScale(p, str);
		else
			++str;
	}
}

static void svgParseStyle(struct SVGParser* p, const char* str);

static int svgParseAttr(struct SVGParser* p, const char* name, const char* value)
{
	struct SVGAttrib* attr = svgGetAttr(p);
	if (!attr) return 0;

	if (strcmp(name, "style") == 0)
	{
		svgParseStyle(p, value);
	}
	else if (strcmp(name, "display") == 0)
	{
		if (strcmp(value, "none") == 0)
			attr->visible = 0;
		else
			attr->visible = 1;
	}
	else if (strcmp(name, "fill") == 0)
	{
		if (strcmp(value, "none") == 0)
		{
			attr->hasFill = 0;
		}
		else
		{
			attr->hasFill = 1;
			attr->fillColor = parseColor(value);
		}
	}
	else if (strcmp(name, "fill-opacity") == 0)
	{
		attr->fillOpacity = parseFloat(value);
	}
	else if (strcmp(name, "stroke") == 0)
	{
		if (strcmp(value, "none") == 0)
		{
			attr->hasStroke = 0;
		}
		else
		{
			attr->hasStroke = 1;
			attr->strokeColor = parseColor(value);
		}
	}
	else if (strcmp(name, "stroke-width") == 0)
	{
		attr->strokeWidth = parseFloat(value);
	}
	else if (strcmp(name, "stroke-opacity") == 0)
	{
		attr->strokeOpacity = parseFloat(value);
	}
	else if (strcmp(name, "transform") == 0)
	{
		svgParseTransform(p, value);
	}
	else
	{
		return 0;
	}
	return 1;
}

static int svgParseNameValue(struct SVGParser* p, const char* start, const char* end)
{
	const char* str;
	const char* val;
	char name[512];
	char value[512];
	int n;

	str = start;
	while (str < end && *str != ':') ++str;

	val = str;

	// Right Trim
	while (str > start &&  (*str == ':' || isspace(*str))) --str;
	++str;

	n = (int)(str - start);
	if (n > 511) n = 511;
	if (n) memcpy(name, start, n);
	name[n] = 0;

	while (val < end && (*val == ':' || isspace(*val))) ++val;

	n = (int)(end - val);
	if (n > 511) n = 511;
	if (n) memcpy(value, val, n);
	value[n] = 0;

	return svgParseAttr(p, name, value);
}

static void svgParseStyle(struct SVGParser* p, const char* str)
{
	const char* start;
	const char* end;

	while (*str)
	{
		// Left Trim
		while(*str && isspace(*str)) ++str;
		start = str;
		while(*str && *str != ';') ++str;
		end = str;

		// Right Trim
		while (end > start &&  (*end == ';' || isspace(*end))) --end;
		++end;

		svgParseNameValue(p, start, end);
		if (*str) ++str;
	}
}

static void svgParseAttribs(struct SVGParser* p, const char** attr)
{
	int i;
	for (i = 0; attr[i]; i += 2)
	{
		if (strcmp(attr[i], "style") == 0)
			svgParseStyle(p, attr[i + 1]);
		else
			svgParseAttr(p, attr[i], attr[i + 1]);
	}
}

static int getArgsPerElement(char cmd)
{
	switch (tolower(cmd))
	{
		case 'v':
		case 'h':
			return 1;
		case 'm':
		case 'l':
		case 't':
			return 2;
		case 'q':
		case 's':
			return 4;
		case 'c':
			return 6;
		case 'a':
			return 7;
	}
	return 0;
}

static float distPtSeg(float x, float y, float px, float py, float qx, float qy)
{
	float pqx, pqy, dx, dy, d, t;
	pqx = qx-px;
	pqy = qy-py;
	dx = x-px;
	dy = y-py;
	d = pqx*pqx + pqy*pqy;
	t = pqx*dx + pqy*dy;
	if (d > 0) t /= d;
	if (t < 0) t = 0;
	else if (t > 1) t = 1;
	dx = px + t*pqx - x;
	dy = py + t*pqy - y;
	return dx*dx + dy*dy;
}

static void cubicBezRec(struct SVGParser* p,
						float x1, float y1, float x2, float y2,
						float x3, float y3, float x4, float y4,
						int level)
{
	float x12,y12,x23,y23,x34,y34,x123,y123,x234,y234,x1234,y1234;
	float d;

	if (level > 12) return;

	x12 = (x1+x2)*0.5f;
	y12 = (y1+y2)*0.5f;
	x23 = (x2+x3)*0.5f;
	y23 = (y2+y3)*0.5f;
	x34 = (x3+x4)*0.5f;
	y34 = (y3+y4)*0.5f;
	x123 = (x12+x23)*0.5f;
	y123 = (y12+y23)*0.5f;
	x234 = (x23+x34)*0.5f;
	y234 = (y23+y34)*0.5f;
	x1234 = (x123+x234)*0.5f;
	y1234 = (y123+y234)*0.5f;

	d = distPtSeg(x1234, y1234, x1,y1, x4,y4);
	if (level > 0 && d < p->tol*p->tol)
	{
		svgPathPoint(p, x1234, y1234);
		return;
	}

	cubicBezRec(p, x1,y1, x12,y12, x123,y123, x1234,y1234, level+1);
	cubicBezRec(p, x1234,y1234, x234,y234, x34,y34, x4,y4, level+1);
}

static void cubicBez(struct SVGParser* p,
					 float x1, float y1, float cx1, float cy1,
					 float cx2, float cy2, float x2, float y2)
{
	cubicBezRec(p, x1,y1, cx1,cy1, cx2,cy2, x2,y2, 0);
	svgPathPoint(p, x2, y2);
}

static void quadBezRec(struct SVGParser* p,
					   float x1, float y1, float x2, float y2, float x3, float y3,
					   int level)
{
	float x12,y12,x23,y23,x123,y123,d;

	if (level > 12) return;

	x12 = (x1+x2)*0.5f;
	y12 = (y1+y2)*0.5f;
	x23 = (x2+x3)*0.5f;
	y23 = (y2+y3)*0.5f;
	x123 = (x12+x23)*0.5f;
	y123 = (y12+y23)*0.5f;

	d = distPtSeg(x123, y123, x1,y1, x3,y3);
	if (level > 0 && d < p->tol*p->tol)
	{
		svgPathPoint(p, x123, y123);
		return;
	}

	quadBezRec(p, x1,y1, x12,y12, x123,y123, level+1);
	quadBezRec(p, x123,y123, x23,y23, x3,y3, level+1);
}

static void quadBez(struct SVGParser* p,
					float x1, float y1, float cx, float cy, float x2, float y2)
{
	quadBezRec(p, x1,y1, cx,cy, x2,y2, 0);
	svgPathPoint(p, x2, y2);
}

static void pathLineTo(struct SVGParser* p, float* cpx, float* cpy, float* args, int rel)
{
	if (rel)
	{
		*cpx += args[0];
		*cpy += args[1];
	}
	else
	{
		*cpx = args[0];
		*cpy = args[1];
	}
	svgPathPoint(p, *cpx, *cpy);
}

static void pathHLineTo(struct SVGParser* p, float* cpx, float* cpy, float* args, int rel)
{
	if (rel)
		*cpx += args[0];
	else
		*cpx = args[0];
	svgPathPoint(p, *cpx, *cpy);
}

static void pathVLineTo(struct SVGParser* p, float* cpx, float* cpy, float* args, int rel)
{
	if (rel)
		*cpy += args[0];
	else
		*cpy = args[0];
	svgPathPoint(p, *cpx, *cpy);
}

static void pathCubicBezTo(struct SVGParser* p, float* cpx, float* cpy,
						   float* cpx2, float* cpy2, float* args, int rel)
{
	float x1, y1, x2, y2, cx1, cy1, cx2, cy2;

	x1 = *cpx;
	y1 = *cpy;
	if (rel)
	{
		cx1 = *cpx + args[0];
		cy1 = *cpy + args[1];
		cx2 = *cpx + args[2];
		cy2 = *cpy + args[3];
		x2 = *cpx + args[4];
		y2 = *cpy + args[5];
	}
	else
	{
		cx1 = args[0];
		cy1 = args[1];
		cx2 = args[2];
		cy2 = args[3];
		x2 = args[4];
		y2 = args[5];
	}

	cubicBez(p, x1,y1, cx1,cy1, cx2,cy2, x2,y2);

	*cpx2 = cx2;
	*cpy2 = cy2;
	*cpx = x2;
	*cpy = y2;
}

static void pathCubicBezShortTo(struct SVGParser* p, float* cpx, float* cpy,
								float* cpx2, float* cpy2, float* args, int rel)
{
	float x1, y1, x2, y2, cx1, cy1, cx2, cy2;

	x1 = *cpx;
	y1 = *cpy;
	if (rel)
	{
		cx2 = *cpx + args[0];
		cy2 = *cpy + args[1];
		x2 = *cpx + args[2];
		y2 = *cpy + args[3];
	}
	else
	{
		cx2 = args[0];
		cy2 = args[1];
		x2 = args[2];
		y2 = args[3];
	}

	cx1 = 2*x1 - *cpx2;
	cy1 = 2*y1 - *cpy2;

	cubicBez(p, x1,y1, cx1,cy1, cx2,cy2, x2,y2);

	*cpx2 = cx2;
	*cpy2 = cy2;
	*cpx = x2;
	*cpy = y2;
}

static void pathQuadBezTo(struct SVGParser* p, float* cpx, float* cpy,
						  float* cpx2, float* cpy2, float* args, int rel)
{
	float x1, y1, x2, y2, cx, cy;

	x1 = *cpx;
	y1 = *cpy;
	if (rel)
	{
		cx = *cpx + args[0];
		cy = *cpy + args[1];
		x2 = *cpx + args[2];
		y2 = *cpy + args[3];
	}
	else
	{
		cx = args[0];
		cy = args[1];
		x2 = args[2];
		y2 = args[3];
	}

	quadBez(p, x1,y1, cx,cy, x2,y2);

	*cpx2 = cx;
	*cpy2 = cy;
	*cpx = x2;
	*cpy = y2;
}

static void pathQuadBezShortTo(struct SVGParser* p, float* cpx, float* cpy,
							   float* cpx2, float* cpy2, float* args, int rel)
{
	float x1, y1, x2, y2, cx, cy;

	x1 = *cpx;
	y1 = *cpy;
	if (rel)
	{
		x2 = *cpx + args[0];
		y2 = *cpy + args[1];
	}
	else
	{
		x2 = args[0];
		y2 = args[1];
	}

	cx = 2*x1 - *cpx2;
	cy = 2*y1 - *cpy2;

	quadBez(p, x1,y1, cx,cy, x2,y2);

	*cpx2 = cx;
	*cpy2 = cy;
	*cpx = x2;
	*cpy = y2;
}

static void svgParsePath(struct SVGParser* p, const char** attr)
{
	const char* s;
	char cmd;
	float args[10];
	int nargs;
	int rargs;
	float cpx, cpy, cpx2, cpy2;
	const char* tmp[4];
	char closedFlag;
	int i;
	char item[64];

	for (i = 0; attr[i]; i += 2)
	{
		if (strcmp(attr[i], "d") == 0)
		{
			s = attr[i + 1];

			svgResetPath(p);
			closedFlag = 0;
			nargs = 0;

			while (*s)
			{
				s = getNextPathItem(s, item);
				if (!*item) break;

				if (isnum(item[0]))
				{
					if (nargs < 10)
						args[nargs++] = (float)atof(item);
					if (nargs >= rargs)
					{
						switch (cmd)
						{
							case 'm':
							case 'M':
							case 'l':
							case 'L':
								pathLineTo(p, &cpx, &cpy, args, (cmd == 'm' || cmd == 'l') ? 1 : 0);
								break;
							case 'H':
							case 'h':
								pathHLineTo(p, &cpx, &cpy, args, cmd == 'h' ? 1 : 0);
								break;
							case 'V':
							case 'v':
								pathVLineTo(p, &cpx, &cpy, args, cmd == 'v' ? 1 : 0);
								break;
							case 'C':
							case 'c':
								pathCubicBezTo(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 'c' ? 1 : 0);
								break;
							case 'S':
							case 's':
								pathCubicBezShortTo(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 's' ? 1 : 0);
								break;
							case 'Q':
							case 'q':
								pathQuadBezTo(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 'q' ? 1 : 0);
								break;
							case 'T':
							case 't':
								pathQuadBezShortTo(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 's' ? 1 : 0);
								break;
							default:
								if (nargs >= 2)
								{
									cpx = args[nargs-2];
									cpy = args[nargs-1];
								}
								break;
						}
						nargs = 0;
					}
				}
				else
				{
					cmd = item[0];
					rargs = getArgsPerElement(cmd);
					if (cmd == 'M' || cmd == 'm')
					{
						// Commit path.
						if (p->nbuf)
							svgCreatePath(p, closedFlag);
						// Start new subpath.
						svgResetPath(p);
						closedFlag = 0;
						nargs = 0;
						cpx = 0; cpy = 0;
					}
					else if (cmd == 'Z' || cmd == 'z')
					{
						closedFlag = 1;
						// Commit path.
						if (p->nbuf)
							svgCreatePath(p, closedFlag);
						// Start new subpath.
						svgResetPath(p);
						closedFlag = 0;
						nargs = 0;
					}
				}
			}

			// Commit path.
			if (p->nbuf)
				svgCreatePath(p, closedFlag);

		}
		else
		{
			tmp[0] = attr[i];
			tmp[1] = attr[i + 1];
			tmp[2] = 0;
			tmp[3] = 0;
			svgParseAttribs(p, tmp);
		}
	}
}

static void svgParseRect(struct SVGParser* p, const char** attr)
{
	float x = 0.0f;
	float y = 0.0f;
	float w = 0.0f;
	float h = 0.0f;
	int i;

	for (i = 0; attr[i]; i += 2)
	{
		if (!svgParseAttr(p, attr[i], attr[i + 1]))
		{
			if (strcmp(attr[i], "x") == 0) x = parseFloat(attr[i+1]);
			if (strcmp(attr[i], "y") == 0) y = parseFloat(attr[i+1]);
			if (strcmp(attr[i], "width") == 0) w = parseFloat(attr[i+1]);
			if (strcmp(attr[i], "height") == 0) h = parseFloat(attr[i+1]);
		}
	}

	if (w != 0.0f && h != 0.0f)
	{
		svgResetPath(p);

		svgPathPoint(p, x, y);
		svgPathPoint(p, x+w, y);
		svgPathPoint(p, x+w, y+h);
		svgPathPoint(p, x, y+h);

		svgCreatePath(p, 1);
	}
}

static void svgParseCircle(struct SVGParser* p, const char** attr)
{
	float cx = 0.0f;
	float cy = 0.0f;
	float r = 0.0f;
	float da;
	int i,n;
	float x,y,u;

	for (i = 0; attr[i]; i += 2)
	{
		if (!svgParseAttr(p, attr[i], attr[i + 1]))
		{
			if (strcmp(attr[i], "cx") == 0) cx = parseFloat(attr[i+1]);
			if (strcmp(attr[i], "cy") == 0) cy = parseFloat(attr[i+1]);
			if (strcmp(attr[i], "r") == 0) r = fabsf(parseFloat(attr[i+1]));
		}
	}

	if (r != 0.0f)
	{
		svgResetPath(p);

		da = acosf(r/(r+p->tol))*2;
		n = (int)ceilf(M_PI*2/da);

		da = (float)(M_PI*2)/n;
		for (i = 0; i < n; ++i)
		{
			u = i*da;
			x = cx + cosf(u)*r;
			y = cy + sinf(u)*r;
			svgPathPoint(p, x, y);
		}

		svgCreatePath(p, 1);
	}
}

static void svgParseLine(struct SVGParser* p, const char** attr)
{
	float x1 = 0.0;
	float y1 = 0.0;
	float x2 = 0.0;
	float y2 = 0.0;
	int i;

	for (i = 0; attr[i]; i += 2)
	{
		if (!svgParseAttr(p, attr[i], attr[i + 1]))
		{
			if (strcmp(attr[i], "x1") == 0) x1 = parseFloat(attr[i + 1]);
			if (strcmp(attr[i], "y1") == 0) y1 = parseFloat(attr[i + 1]);
			if (strcmp(attr[i], "x2") == 0) x2 = parseFloat(attr[i + 1]);
			if (strcmp(attr[i], "y2") == 0) y2 = parseFloat(attr[i + 1]);
		}
	}

	svgResetPath(p);

	svgPathPoint(p, x1, y1);
	svgPathPoint(p, x2, y2);

	svgCreatePath(p, 0);
}

static void svgParsePoly(struct SVGParser* p, const char** attr, int closeFlag)
{
	int i;
	const char* s;
	float args[2];
	int nargs;
	char item[64];

	svgResetPath(p);

	for (i = 0; attr[i]; i += 2)
	{
		if (!svgParseAttr(p, attr[i], attr[i + 1]))
		{
			if (strcmp(attr[i], "points") == 0)
			{
				s = attr[i + 1];
				nargs = 0;
				while (*s)
				{
					s = getNextPathItem(s, item);
					args[nargs++] = (float)atof(item);
					if (nargs >= 2)
					{
						svgPathPoint(p, args[0], args[1]);
						nargs = 0;
					}
				}
			}
		}
	}

	svgCreatePath(p, closeFlag);
}

static void svgStartElement(void* ud, const char* el, const char** attr)
{
	struct SVGParser* p = (struct SVGParser*)ud;

	// Skip everything in defs
	if (p->defsFlag)
		return;

	if (strcmp(el, "g") == 0)
	{
		svgPushAttr(p);
		svgParseAttribs(p, attr);
	}
	else if (strcmp(el, "path") == 0)
	{
		if (p->pathFlag)	// Do not allow nested paths.
			return;
		svgPushAttr(p);
		svgParsePath(p, attr);
		p->pathFlag = 1;
		svgPopAttr(p);
	}
	else if (strcmp(el, "rect") == 0)
	{
		svgPushAttr(p);
		svgParseRect(p, attr);
		svgPopAttr(p);
	}
	else if (strcmp(el, "circle") == 0)
	{
		svgPushAttr(p);
		svgParseCircle(p, attr);
		svgPopAttr(p);
	}
	else if (strcmp(el, "line") == 0)
	{
		svgPushAttr(p);
		svgParseLine(p, attr);
		svgPopAttr(p);
	}
	else if (strcmp(el, "polyline") == 0)
	{
		svgPushAttr(p);
		svgParsePoly(p, attr, 0);
		svgPopAttr(p);
	}
	else if (strcmp(el, "polygon") == 0)
	{
		svgPushAttr(p);
		svgParsePoly(p, attr, 1);
		svgPopAttr(p);
	}
	else if (strcmp(el, "defs") == 0)
	{
		p->defsFlag = 1;
	}
}

static void svgEndElement(void* ud, const char* el)
{
	struct SVGParser* p = (struct SVGParser*)ud;

	if (strcmp(el, "g") == 0)
	{
		svgPopAttr(p);
	}
	else if (strcmp(el, "path") == 0)
	{
		p->pathFlag = 0;
	}
	else if (strcmp(el, "defs") == 0)
	{
		p->defsFlag = 0;
	}
}

static void svgContent(void* ud, const char* s)
{
	// empty
}

struct SVGPath* svgParse(char* input)
{
	struct SVGParser* p;
	struct SVGPath* ret = 0;

	p = svgCreateParser();
	if (!p)
		return 0;

	p->tol = 1.0f;

	parsexml(input, svgStartElement, svgEndElement, svgContent, p);

	if (p->buf)
	{
		free(p->buf);
		p->buf = NULL;
		p->nbuf = 0;
		p->cbuf = 0;
	}

	ret = p->plist;
	p->plist = 0;

	svgDeleteParser(p);

	return ret;
}

struct SVGPath* svgParseFromFile(const char* filename)
{
	FILE* fp;
	int size;
	char* data;
	struct SVGPath* plist;

	fp = fopen(filename, "rb");
	if (!fp) return 0;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	data = (char*)malloc(size+1);
	fread(data, size, 1, fp);
	data[size] = '\0';	// Must be null terminated.
	fclose(fp);
	plist = svgParse(data);
	free(data);
	return plist;
}

void svgDelete(struct SVGPath* plist)
{
	struct SVGPath* path;
	struct SVGPath* next;
	if (!plist)
		return;
	path = plist;
	while (path)
	{
		next = path->next;
		if (path->pts)
			free(path->pts);
		free(path);
		path = next;
	}
}
