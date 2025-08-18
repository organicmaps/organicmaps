This is only a very brief overview.  There is quite a bit of
additional documentation in the source code itself.


Goals of robust tesselation
---------------------------

The tesselation algorithm is fundamentally a 2D algorithm.  We
initially project all data into a plane; our goal is to robustly
tesselate the projected data.  The same topological tesselation is
then applied to the input data.

Topologically, the output should always be a tesselation.  If the
input is even slightly non-planar, then some triangles will
necessarily be back-facing when viewed from some angles, but the goal
is to minimize this effect.

The algorithm needs some capability of cleaning up the input data as
well as the numerical errors in its own calculations.  One way to do
this is to specify a tolerance as defined above, and clean up the
input and output during the line sweep process.  At the very least,
the algorithm must handle coincident vertices, vertices incident to an
edge, and coincident edges.


Phases of the algorithm
-----------------------

1. Find the polygon normal N.
2. Project the vertex data onto a plane.  It does not need to be
   perpendicular to the normal, eg. we can project onto the plane
   perpendicular to the coordinate axis whose dot product with N
   is largest.
3. Using a line-sweep algorithm, partition the plane into x-monotone
   regions.  Any vertical line intersects an x-monotone region in
   at most one interval.
4. Triangulate the x-monotone regions.
5. Group the triangles into strips and fans.


Finding the normal vector
-------------------------

A common way to find a polygon normal is to compute the signed area
when the polygon is projected along the three coordinate axes.  We
can't do this, since contours can have zero area without being
degenerate (eg. a bowtie).

We fit a plane to the vertex data, ignoring how they are connected
into contours.  Ideally this would be a least-squares fit; however for
our purpose the accuracy of the normal is not important.  Instead we
find three vertices which are widely separated, and compute the normal
to the triangle they form.  The vertices are chosen so that the
triangle has an area at least 1/sqrt(3) times the largest area of any
triangle formed using the input vertices.

The contours do affect the orientation of the normal; after computing
the normal, we check that the sum of the signed contour areas is
non-negative, and reverse the normal if necessary.


Projecting the vertices
-----------------------

We project the vertices onto a plane perpendicular to one of the three
coordinate axes.  This helps numerical accuracy by removing a
transformation step between the original input data and the data
processed by the algorithm.  The projection also compresses the input
data; the 2D distance between vertices after projection may be smaller
than the original 2D distance.  However by choosing the coordinate
axis whose dot product with the normal is greatest, the compression
factor is at most 1/sqrt(3).

Even though the *accuracy* of the normal is not that important (since
we are projecting perpendicular to a coordinate axis anyway), the
*robustness* of the computation is important.  For example, if there
are many vertices which lie almost along a line, and one vertex V
which is well-separated from the line, then our normal computation
should involve V otherwise the results will be garbage.

The advantage of projecting perpendicular to the polygon normal is
that computed intersection points will be as close as possible to
their ideal locations.  To get this behavior, define TRUE_PROJECT.


The Line Sweep
--------------

There are three data structures: the mesh, the event queue, and the
edge dictionary.

The mesh is a "quad-edge" data structure which records the topology of
the current decomposition; for details see the include file "mesh.h".

The event queue simply holds all vertices (both original and computed
ones), organized so that we can quickly extract the vertex with the
minimum x-coord (and among those, the one with the minimum y-coord).

The edge dictionary describes the current intersection of the sweep
line with the regions of the polygon.  This is just an ordering of the
edges which intersect the sweep line, sorted by their current order of
intersection.  For each pair of edges, we store some information about
the monotone region between them -- these are call "active regions"
(since they are crossed by the current sweep line).

The basic algorithm is to sweep from left to right, processing each
vertex.  The processed portion of the mesh (left of the sweep line) is
a planar decomposition.  As we cross each vertex, we update the mesh
and the edge dictionary, then we check any newly adjacent pairs of
edges to see if they intersect.

A vertex can have any number of edges.  Vertices with many edges can
be created as vertices are merged and intersection points are
computed.  For unprocessed vertices (right of the sweep line), these
edges are in no particular order around the vertex; for processed
vertices, the topological ordering should match the geometric ordering.

The vertex processing happens in two phases: first we process are the
left-going edges (all these edges are currently in the edge
dictionary).  This involves:

 - deleting the left-going edges from the dictionary;
 - relinking the mesh if necessary, so that the order of these edges around
   the event vertex matches the order in the dictionary;
 - marking any terminated regions (regions which lie between two left-going
   edges) as either "inside" or "outside" according to their winding number.

When there are no left-going edges, and the event vertex is in an
"interior" region, we need to add an edge (to split the region into
monotone pieces).  To do this we simply join the event vertex to the
rightmost left endpoint of the upper or lower edge of the containing
region.

Then we process the right-going edges.  This involves:

 - inserting the edges in the edge dictionary;
 - computing the winding number of any newly created active regions.
   We can compute this incrementally using the winding of each edge
   that we cross as we walk through the dictionary.
 - relinking the mesh if necessary, so that the order of these edges around
   the event vertex matches the order in the dictionary;
 - checking any newly adjacent edges for intersection and/or merging.

If there are no right-going edges, again we need to add one to split
the containing region into monotone pieces.  In our case it is most
convenient to add an edge to the leftmost right endpoint of either
containing edge; however we may need to change this later (see the
code for details).


Invariants
----------

These are the most important invariants maintained during the sweep.
We define a function VertLeq(v1,v2) which defines the order in which
vertices cross the sweep line, and a function EdgeLeq(e1,e2; loc)
which says whether e1 is below e2 at the sweep event location "loc".
This function is defined only at sweep event locations which lie
between the rightmost left endpoint of {e1,e2}, and the leftmost right
endpoint of {e1,e2}.

Invariants for the Edge Dictionary.

 - Each pair of adjacent edges e2=Succ(e1) satisfies EdgeLeq(e1,e2)
   at any valid location of the sweep event.
 - If EdgeLeq(e2,e1) as well (at any valid sweep event), then e1 and e2
   share a common endpoint.
 - For each e in the dictionary, e->Dst has been processed but not e->Org.
 - Each edge e satisfies VertLeq(e->Dst,event) && VertLeq(event,e->Org)
   where "event" is the current sweep line event.
 - No edge e has zero length.
 - No two edges have identical left and right endpoints.

Invariants for the Mesh (the processed portion).

 - The portion of the mesh left of the sweep line is a planar graph,
   ie. there is *some* way to embed it in the plane.
 - No processed edge has zero length.
 - No two processed vertices have identical coordinates.
 - Each "inside" region is monotone, ie. can be broken into two chains
   of monotonically increasing vertices according to VertLeq(v1,v2)
   - a non-invariant: these chains may intersect (slightly) due to
     numerical errors, but this does not affect the algorithm's operation.

Invariants for the Sweep.

 - If a vertex has any left-going edges, then these must be in the edge
   dictionary at the time the vertex is processed.
 - If an edge is marked "fixUpperEdge" (it is a temporary edge introduced
   by ConnectRightVertex), then it is the only right-going edge from
   its associated vertex.  (This says that these edges exist only
   when it is necessary.)


Robustness
----------

The key to the robustness of the algorithm is maintaining the
invariants above, especially the correct ordering of the edge
dictionary.  We achieve this by:

  1. Writing the numerical computations for maximum precision rather
     than maximum speed.

  2. Making no assumptions at all about the results of the edge
     intersection calculations -- for sufficiently degenerate inputs,
     the computed location is not much better than a random number.

  3. When numerical errors violate the invariants, restore them
     by making *topological* changes when necessary (ie. relinking
     the mesh structure).


Triangulation and Grouping
--------------------------

We finish the line sweep before doing any triangulation.  This is
because even after a monotone region is complete, there can be further
changes to its vertex data because of further vertex merging.

After triangulating all monotone regions, we want to group the
triangles into fans and strips.  We do this using a greedy approach.
The triangulation itself is not optimized to reduce the number of
primitives; we just try to get a reasonable decomposition of the
computed triangulation.

Optionally, it's possible to output a Constrained Delaunay Triangulation.
This is done by doing a delaunay refinement with the normal triangulation as
a basis. The Edge Flip algorithm is used, which is guaranteed to terminate in O(n^2).

Note: We don't use robust predicates to check if edges are locally
delaunay, but currently us a naive epsilon of 0.01 radians to ensure
termination.
