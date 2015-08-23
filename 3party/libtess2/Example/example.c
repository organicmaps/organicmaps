
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include "nanosvg.h"
#include "tesselator.h"


void* stdAlloc(void* userData, unsigned int size)
{
	int* allocated = ( int*)userData;
	TESS_NOTUSED(userData);
	*allocated += (int)size;
	return malloc(size);
}

void stdFree(void* userData, void* ptr)
{
	TESS_NOTUSED(userData);
	free(ptr);
}

struct MemPool
{
	unsigned char* buf;
	unsigned int cap;
	unsigned int size;
};

void* poolAlloc( void* userData, unsigned int size )
{
	struct MemPool* pool = (struct MemPool*)userData;
	size = (size+0x7) & ~0x7;
	if (pool->size + size < pool->cap)
	{
		unsigned char* ptr = pool->buf + pool->size;
		pool->size += size;
		return ptr;
	}
	printf("out of mem: %d < %d!\n", pool->size + size, pool->cap);
	return 0;
}

void poolFree( void* userData, void* ptr )
{
	// empty
	TESS_NOTUSED(userData);
	TESS_NOTUSED(ptr);
}


// Undefine this to see non-interactive heap allocator version.
#define USE_POOL 1


int run = 1;

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	TESS_NOTUSED(scancode);
	TESS_NOTUSED(mods);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		run = !run;
}

int main(int argc, char *argv[])
{
	GLFWwindow* window;
	const GLFWvidmode* mode;
	int width,height,i,j;
	struct SVGPath* bg;
	struct SVGPath* fg;
	struct SVGPath* it;
	float bounds[4],view[4],cx,cy,w,offx,offy;
	float t = 0.0f, pt = 0.0f;
	TESSalloc ma;
	TESStesselator* tess = 0;
	const int nvp = 6;
	unsigned char* vflags = 0;
	int nvflags = 0;
#ifdef USE_POOL
	struct MemPool pool;
	unsigned char mem[1024*1024];
#else
	int allocated = 0;
#endif
	TESS_NOTUSED(argc);
	TESS_NOTUSED(argv);

	if (!glfwInit()) {
		printf("Failed to init GLFW.");
		return -1;
	}

	printf("loading...\n");
	// Load assets
	bg = svgParseFromFile("../Bin/bg.svg");
	if (!bg) return -1;
	fg = svgParseFromFile("../Bin/fg.svg");
	if (!fg) return -1;

	printf("go...\n");

	// Flip y
	for (it = bg; it != NULL; it = it->next)
		for (i = 0; i < it->npts; ++i)
			it->pts[i*2+1] = -it->pts[i*2+1];
	for (it = fg; it != NULL; it = it->next)
		for (i = 0; i < it->npts; ++i)
			it->pts[i*2+1] = -it->pts[i*2+1];

	// Find FG bounds and center.
	bounds[0] = bounds[2] = fg->pts[0];
	bounds[1] = bounds[3] = fg->pts[1];
	for (it = fg; it != NULL; it = it->next)
	{
		for (i = 0; i < it->npts; ++i)
		{
			const float x = it->pts[i*2];
			const float y = it->pts[i*2+1];
			if (x < bounds[0]) bounds[0] = x;
			if (y < bounds[1]) bounds[1] = y;
			if (x > bounds[2]) bounds[2] = x;
			if (y > bounds[3]) bounds[3] = y;
		}
	}
	cx = (bounds[0]+bounds[2])/2;
	cy = (bounds[1]+bounds[3])/2;
	for (it = fg; it != NULL; it = it->next)
	{
		for (i = 0; i < it->npts; ++i)
		{
			it->pts[i*2] -= cx;
			it->pts[i*2+1] -= cy;
		}
	}

	// Find BG bounds.
	bounds[0] = bounds[2] = bg->pts[0];
	bounds[1] = bounds[3] = bg->pts[1];
	for (it = bg; it != NULL; it = it->next)
	{
		for (i = 0; i < it->npts; ++i)
		{
			const float x = it->pts[i*2];
			const float y = it->pts[i*2+1];
			if (x < bounds[0]) bounds[0] = x;
			if (y < bounds[1]) bounds[1] = y;
			if (x > bounds[2]) bounds[2] = x;
			if (y > bounds[3]) bounds[3] = y;
		}
	}

#ifdef USE_POOL

	pool.size = 0;
	pool.cap = sizeof(mem);
	pool.buf = mem;
	memset(&ma, 0, sizeof(ma));
	ma.memalloc = poolAlloc;
	ma.memfree = poolFree;
	ma.userData = (void*)&pool;
	ma.extraVertices = 256; // realloc not provided, allow 256 extra vertices.

#else

	memset(&ma, 0, sizeof(ma));
	ma.memalloc = stdAlloc;
	ma.memfree = stdFree;
	ma.userData = (void*)&allocated;
	ma.extraVertices = 256; // realloc not provided, allow 256 extra vertices.

	tess = tessNewTess(&ma);
	if (!tess)
		return -1;

	// Offset the foreground shape to center of the bg.
	offx = (bounds[2]+bounds[0])/2;
	offy = (bounds[3]+bounds[1])/2;
	for (it = fg; it != NULL; it = it->next)
	{
		for (i = 0; i < it->npts; ++i)
		{
			it->pts[i*2] += offx;
			it->pts[i*2+1] += offy;
		}
	}

	// Add contours.
	for (it = bg; it != NULL; it = it->next)
		tessAddContour(tess, 2, it->pts, sizeof(float)*2, it->npts);
	for (it = fg; it != NULL; it = it->next)
		tessAddContour(tess, 2, it->pts, sizeof(float)*2, it->npts);
	if (!tessTesselate(tess, TESS_WINDING_POSITIVE, TESS_POLYGONS, nvp, 2, 0))
		return -1;
	printf("Memory used: %.1f kB\n", allocated/1024.0f);

#endif

	mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	width = mode->width - 40;
	height = mode->height - 80;
    window = glfwCreateWindow(width, height, "Libtess2 Demo", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key);
	glfwMakeContextCurrent(window);

	// Adjust bounds so that we get nice view of the bg.
	cx = (bounds[0]+bounds[2])/2;
	cy = (bounds[3]+bounds[1])/2;
	w = (bounds[2]-bounds[0])/2;
	view[0] = cx - w*1.2f;
	view[2] = cx + w*1.2f;
	view[1] = cy - w*1.2f*(float)height/(float)width;
	view[3] = cy + w*1.2f*(float)height/(float)width;

	glfwSetTime(0);

	while (!glfwWindowShouldClose(window))
	{
		float ct = (float)glfwGetTime();
		if (run) t += ct - pt;
		pt = ct;

		// Update and render
		glViewport(0, 0, width, height);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(view[0],view[2],view[1],view[3],-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

#ifdef USE_POOL
		pool.size = 0; // reset pool
		tess = tessNewTess(&ma);
		if (tess)
		{
			offx = (view[2]+view[0])/2 + sinf(t) * (view[2]-view[0])/2;
			offy = (view[3]+view[1])/2 + cosf(t*3.13f) * (view[3]-view[1])/6;

			for (it = fg; it != NULL; it = it->next)
			{
				for (i = 0; i < it->npts; ++i)
				{
					it->pts[i*2] += offx;
					it->pts[i*2+1] += offy;
				}
			}

			for (it = bg; it != NULL; it = it->next)
				tessAddContour(tess, 2, it->pts, sizeof(float)*2, it->npts);
			for (it = fg; it != NULL; it = it->next)
				tessAddContour(tess, 2, it->pts, sizeof(float)*2, it->npts);

			for (it = fg; it != NULL; it = it->next)
			{
				for (i = 0; i < it->npts; ++i)
				{
					it->pts[i*2] -= offx;
					it->pts[i*2+1] -= offy;
				}
			}

			// First combine contours and then triangulate, this removes unnecessary inner vertices.
			if (tessTesselate(tess, TESS_WINDING_POSITIVE, TESS_BOUNDARY_CONTOURS, 0, 0, 0))
			{
				const float* verts = tessGetVertices(tess);
				const int* vinds = tessGetVertexIndices(tess);
				const int nverts = tessGetVertexCount(tess);
				const int* elems = tessGetElements(tess);
				const int nelems = tessGetElementCount(tess);

				if (nverts > nvflags)
				{
					if (vflags)
						free(vflags);
					nvflags = nverts;
					vflags = (unsigned char*)malloc(sizeof(unsigned char)*nvflags);
				}

				if (vflags)
				{
					// Vertex indices describe the order the indices were added and can be used
					// to map the tesselator output to input. Vertices marked as TESS_UNDEF
					// are the ones that were created at the intersection of segments.
					// That is, if vflags is set it means that the vertex comes from intersegment.
					for (i = 0; i < nverts; ++i)
						vflags[i] = vinds[i] == TESS_UNDEF ? 1 : 0;
				}

				for (i = 0; i < nelems; ++i)
				{
					int b = elems[i*2];
					int n = elems[i*2+1];
					tessAddContour(tess, 2, &verts[b*2], sizeof(float)*2, n);
				}
				if (!tessTesselate(tess, TESS_WINDING_POSITIVE, TESS_POLYGONS, nvp, 2, 0))
					tess = 0;
			}
			else
				tess = 0;
		}
#endif

		// Draw tesselated pieces.
		if (tess)
		{
			const float* verts = tessGetVertices(tess);
			const int* vinds = tessGetVertexIndices(tess);
			const int* elems = tessGetElements(tess);
			const int nverts = tessGetVertexCount(tess);
			const int nelems = tessGetElementCount(tess);

			// Draw polygons.
			glColor4ub(255,255,255,128);
			for (i = 0; i < nelems; ++i)
			{
				const int* p = &elems[i*nvp];
				glBegin(GL_TRIANGLE_FAN);
				for (j = 0; j < nvp && p[j] != TESS_UNDEF; ++j)
					glVertex2f(verts[p[j]*2], verts[p[j]*2+1]);
				glEnd();
			}

			glColor4ub(0,0,0,16);
			for (i = 0; i < nelems; ++i)
			{
				const int* p = &elems[i*nvp];
				glBegin(GL_LINE_LOOP);
				for (j = 0; j < nvp && p[j] != TESS_UNDEF; ++j)
					glVertex2f(verts[p[j]*2], verts[p[j]*2+1]);
				glEnd();
			}

			glColor4ub(0,0,0,128);
			glPointSize(3.0f);
			glBegin(GL_POINTS);
			for (i = 0; i < nverts; ++i)
			{
				if (vflags && vflags[vinds[i]])
					glColor4ub(255,0,0,192);
				else
					glColor4ub(0,0,0,128);
				glVertex2f(verts[i*2], verts[i*2+1]);
			}
			glEnd();
			glPointSize(1.0f);
		}

		glEnable(GL_DEPTH_TEST);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	if (tess) tessDeleteTess(tess);

	if (vflags)
		free(vflags);

	svgDelete(bg);
	svgDelete(fg);

	glfwTerminate();
	return 0;
}
