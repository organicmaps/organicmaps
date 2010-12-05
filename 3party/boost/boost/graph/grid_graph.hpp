//=======================================================================
// Copyright 2009 Trustees of Indiana University.
// Authors: Michael Hansen, Andrew Lumsdaine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_GRAPH_GRID_GRAPH_HPP
#define BOOST_GRAPH_GRID_GRAPH_HPP

#include <cmath>
#include <functional>
#include <numeric>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/limits.hpp>
#include <boost/make_shared.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/property_map/property_map.hpp>

#define BOOST_GRID_GRAPH_TEMPLATE_PARAMS \
  std::size_t DimensionsT, typename VertexIndexT, \
    typename EdgeIndexT

#define BOOST_GRID_GRAPH_TYPE \
  grid_graph<DimensionsT, VertexIndexT, EdgeIndexT>

#define BOOST_GRID_GRAPH_TYPE_MEM typename BOOST_GRID_GRAPH_TYPE::

#define BOOST_GRID_GRAPH_TYPE_TD(mem) \
  typedef typename BOOST_GRID_GRAPH_TYPE::mem mem

#define BOOST_GRID_GRAPH_TRAITS_T \
  typename graph_traits<BOOST_GRID_GRAPH_TYPE >

namespace boost {

  // Class prototype for grid_graph
  template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
  class grid_graph;

  //===================
  // Index Property Map
  //===================

  template <typename Graph,
            typename Descriptor,
            typename Index>
  struct grid_graph_index_map {
  public:
    typedef Index value_type;
    typedef Index reference_type;
    typedef reference_type reference;
    typedef Descriptor key_type;
    typedef readable_property_map_tag category;

    grid_graph_index_map() { }

    grid_graph_index_map(const Graph& graph) :
      m_graph(make_shared<Graph>(graph)) { }

    value_type operator[](key_type key) const {
      return (m_graph->index_of(key));
    }

  protected:
    shared_ptr<Graph> m_graph;
  };

  template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
  struct property_map<BOOST_GRID_GRAPH_TYPE, vertex_index_t> {
    typedef grid_graph_index_map<BOOST_GRID_GRAPH_TYPE,
                                 BOOST_GRID_GRAPH_TRAITS_T::vertex_descriptor,
                                 BOOST_GRID_GRAPH_TRAITS_T::vertices_size_type> type;
    typedef type const_type;
  };

  template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
  struct property_map<BOOST_GRID_GRAPH_TYPE, edge_index_t> {
    typedef grid_graph_index_map<BOOST_GRID_GRAPH_TYPE,
                                 BOOST_GRID_GRAPH_TRAITS_T::edge_descriptor,
                                 BOOST_GRID_GRAPH_TRAITS_T::edges_size_type> type;
    typedef type const_type;
  };

  //=================
  // Function Objects
  //=================

  namespace detail {

    // vertex_at
    template <typename Graph>
    struct grid_graph_vertex_at {

      typedef typename graph_traits<Graph>::vertex_descriptor result_type;

      grid_graph_vertex_at() : m_graph(0) {}

      grid_graph_vertex_at(const Graph* graph) :
        m_graph(graph) { }

      result_type
      operator()
      (typename graph_traits<Graph>::vertices_size_type vertex_index) const {
        return (vertex(vertex_index, *m_graph));
      }

    private:
      const Graph* m_graph;
    };

    // out_edge_at
    template <typename Graph>
    struct grid_graph_out_edge_at {

    private:
      typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

    public:
      typedef typename graph_traits<Graph>::edge_descriptor result_type;

      grid_graph_out_edge_at() : m_vertex(), m_graph(0) {}

      grid_graph_out_edge_at(vertex_descriptor source_vertex,
                             const Graph* graph) :
        m_vertex(source_vertex),
        m_graph(graph) { }

      result_type
      operator()
      (typename graph_traits<Graph>::degree_size_type out_edge_index) const {
        return (out_edge_at(m_vertex, out_edge_index, *m_graph));
      }

    private:
      vertex_descriptor m_vertex;
      const Graph* m_graph;
    };

    // in_edge_at
    template <typename Graph>
    struct grid_graph_in_edge_at {

    private:
      typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

    public:
      typedef typename graph_traits<Graph>::edge_descriptor result_type;

      grid_graph_in_edge_at() : m_vertex(), m_graph(0) {}

      grid_graph_in_edge_at(vertex_descriptor target_vertex,
                            const Graph* graph) :
        m_vertex(target_vertex),
        m_graph(graph) { }

      result_type
      operator()
      (typename graph_traits<Graph>::degree_size_type in_edge_index) const {
        return (in_edge_at(m_vertex, in_edge_index, *m_graph));
      }

    private:
      vertex_descriptor m_vertex;
      const Graph* m_graph;
    };

    // edge_at
    template <typename Graph>
    struct grid_graph_edge_at {

      typedef typename graph_traits<Graph>::edge_descriptor result_type;

      grid_graph_edge_at() : m_graph(0) {}

      grid_graph_edge_at(const Graph* graph) :
        m_graph(graph) { }

      result_type
      operator()
      (typename graph_traits<Graph>::edges_size_type edge_index) const {
        return (edge_at(edge_index, *m_graph));
      }

    private:
      const Graph* m_graph;
    };

    // adjacent_vertex_at
    template <typename Graph>
    struct grid_graph_adjacent_vertex_at {

    public:
      typedef typename graph_traits<Graph>::vertex_descriptor result_type;

      grid_graph_adjacent_vertex_at(result_type source_vertex,
                                    const Graph* graph) :
        m_vertex(source_vertex),
        m_graph(graph) { }

      result_type
      operator()
      (typename graph_traits<Graph>::degree_size_type adjacent_index) const {
        return (target(out_edge_at(m_vertex, adjacent_index, *m_graph), *m_graph));
      }

    private:
      result_type m_vertex;
      const Graph* m_graph;
    };

  } // namespace detail

  //===========
  // Grid Graph
  //===========

  template <std::size_t Dimensions,
            typename VertexIndex = std::size_t,
            typename EdgeIndex = VertexIndex> 
  class grid_graph {

  private:
    typedef boost::array<bool, Dimensions> WrapDimensionArray;
    grid_graph() { };

  public:

    typedef grid_graph<Dimensions, VertexIndex, EdgeIndex> type;

    // sizes
    typedef VertexIndex vertices_size_type;
    typedef EdgeIndex edges_size_type;
    typedef EdgeIndex degree_size_type;

    // descriptors
    typedef boost::array<VertexIndex, Dimensions> vertex_descriptor;
    typedef std::pair<vertex_descriptor, vertex_descriptor> edge_descriptor;

    // vertex_iterator
    typedef counting_iterator<vertices_size_type> vertex_index_iterator;
    typedef detail::grid_graph_vertex_at<type> vertex_function;
    typedef transform_iterator<vertex_function, vertex_index_iterator> vertex_iterator;

    // edge_iterator
    typedef counting_iterator<edges_size_type> edge_index_iterator;
    typedef detail::grid_graph_edge_at<type> edge_function;
    typedef transform_iterator<edge_function, edge_index_iterator> edge_iterator;

    // out_edge_iterator
    typedef counting_iterator<degree_size_type> degree_iterator;
    typedef detail::grid_graph_out_edge_at<type> out_edge_function;
    typedef transform_iterator<out_edge_function, degree_iterator> out_edge_iterator;

    // in_edge_iterator
    typedef detail::grid_graph_in_edge_at<type> in_edge_function;
    typedef transform_iterator<in_edge_function, degree_iterator> in_edge_iterator;

    // adjacency_iterator
    typedef detail::grid_graph_adjacent_vertex_at<type> adjacent_vertex_function;
    typedef transform_iterator<adjacent_vertex_function, degree_iterator> adjacency_iterator;

    // categories
    typedef undirected_tag directed_category;
    typedef disallow_parallel_edge_tag edge_parallel_category;    
    struct traversal_category : virtual public incidence_graph_tag,
                                virtual public adjacency_graph_tag,
                                virtual public vertex_list_graph_tag,
                                virtual public edge_list_graph_tag,
                                virtual public bidirectional_graph_tag,
                                virtual public adjacency_matrix_tag { };

    static inline vertex_descriptor null_vertex()
    {
      vertex_descriptor maxed_out_vertex;
      std::fill(maxed_out_vertex.begin(), maxed_out_vertex.end(),
                (std::numeric_limits<vertices_size_type>::max)());

      return (maxed_out_vertex);
    }

    // Constructor that defaults to no wrapping for all dimensions.
    grid_graph(vertex_descriptor dimension_lengths) :
      m_dimension_lengths(dimension_lengths) {

      std::fill(m_wrap_dimension.begin(),
                m_wrap_dimension.end(), false);

      precalculate();
    }

    // Constructor that allows for wrapping to be specified for all
    // dimensions at once.
    grid_graph(vertex_descriptor dimension_lengths,
               bool wrap_all_dimensions) :
      m_dimension_lengths(dimension_lengths) {
      
      std::fill(m_wrap_dimension.begin(),
                m_wrap_dimension.end(),
                wrap_all_dimensions);

      precalculate();
    }

    // Constructor that allows for individual dimension wrapping to be
    // specified.
    grid_graph(vertex_descriptor dimension_lengths,
               WrapDimensionArray wrap_dimension) :
      m_dimension_lengths(dimension_lengths),
      m_wrap_dimension(wrap_dimension) {

      precalculate();
    }

    // Returns the number of dimensions in the graph
    inline std::size_t dimensions() const {
      return (Dimensions);
    }

    // Returns the length of dimension [dimension_index]
    inline vertices_size_type length(std::size_t dimension) const {
      return (m_dimension_lengths[dimension]);
    }

    // Returns a value indicating if dimension [dimension_index] wraps
    inline bool wrapped(std::size_t dimension) const {
      return (m_wrap_dimension[dimension]);
    }

    // Gets the vertex that is [distance] units ahead of [vertex] in
    // dimension [dimension_index].
    vertex_descriptor next
    (vertex_descriptor vertex,
     std::size_t dimension_index,
     vertices_size_type distance = 1) const {

      vertices_size_type new_position =
        vertex[dimension_index] + distance;

      if (wrapped(dimension_index)) {
        new_position %= length(dimension_index);
      }
      else {
        // Stop at the end of this dimension if necessary.
        new_position =
          (std::min)(new_position,
                     length(dimension_index) - 1);
      }

      vertex[dimension_index] = new_position;

      return (vertex);    
    }

    // Gets the vertex that is [distance] units behind [vertex] in
    // dimension [dimension_index].
    vertex_descriptor previous
    (vertex_descriptor vertex,
     std::size_t dimension_index,
     vertices_size_type distance = 1) const {
    
      // We're assuming that vertices_size_type is unsigned, so we
      // need to be careful about the math.
      vertex[dimension_index] =
        (distance > vertex[dimension_index]) ?
        (wrapped(dimension_index) ?
         (length(dimension_index) - (distance % length(dimension_index))) : 0) :
        vertex[dimension_index] - distance;

      return (vertex);    
    }

  protected:

    // Returns the number of vertices in the graph
    inline vertices_size_type num_vertices() const {
      return (m_num_vertices);
    }
    
    // Returns the number of edges in the graph
    inline edges_size_type num_edges() const {
      return (m_num_edges);
    }

    // Returns the number of edges in dimension [dimension_index]
    inline edges_size_type num_edges
    (std::size_t dimension_index) const {
      return (m_edge_count[dimension_index]);
    }

    // Returns the index of [vertex] (See also vertex_at)
    vertices_size_type index_of(vertex_descriptor vertex) const {

      vertices_size_type vertex_index = 0;
      vertices_size_type index_multiplier = 1;

      for (std::size_t dimension_index = 0;
           dimension_index < Dimensions;
           ++dimension_index) {

        vertex_index += (vertex[dimension_index] * index_multiplier);
        index_multiplier *= length(dimension_index);
      }

      return (vertex_index);
    }

    // Returns the vertex whose index is [vertex_index] (See also
    // index_of(vertex_descriptor))
    vertex_descriptor vertex_at
    (vertices_size_type vertex_index) const {
    
      array<vertices_size_type, Dimensions> vertex;
      vertices_size_type index_divider = 1;

      for (std::size_t dimension_index = 0;
           dimension_index < Dimensions;
           ++dimension_index) {

        vertex[dimension_index] = (vertex_index / index_divider) %
          length(dimension_index);

        index_divider *= length(dimension_index);
      }

      return (vertex);
    }    

    // Returns the edge whose index is [edge_index] (See also
    // index_of(edge_descriptor)).  NOTE: The index mapping is
    // dependent upon dimension wrapping.
    edge_descriptor edge_at(edges_size_type edge_index) const {

      // Edge indices are sorted into bins by dimension
      std::size_t dimension_index = 0;
      edges_size_type dimension_edges = num_edges(0);

      while (edge_index >= dimension_edges) {
        edge_index -= dimension_edges;
        ++dimension_index;
        dimension_edges = num_edges(dimension_index);
      }

      vertex_descriptor vertex_source, vertex_target;
      bool is_forward = ((edge_index / (num_edges(dimension_index) / 2)) == 0);

      if (wrapped(dimension_index)) {
        vertex_source = vertex_at(edge_index % num_vertices());
        vertex_target = is_forward ?
          next(vertex_source, dimension_index) :
          previous(vertex_source, dimension_index);
      }
      else {

        // Dimensions can wrap arbitrarily, so an index needs to be
        // computed in a more complex manner.  This is done by
        // grouping the edges for each dimension together into "bins"
        // and considering [edge_index] as an offset into the bin.
        // Each bin consists of two parts: the "forward" looking edges
        // and the "backward" looking edges for the dimension.

        edges_size_type vertex_offset = edge_index % num_edges(dimension_index);

        // Consider vertex_offset an index into the graph's vertex
        // space but with the dimension [dimension_index] reduced in
        // size by one.
        vertices_size_type index_divider = 1;

        for (std::size_t dimension_index_iter = 0;
             dimension_index_iter < Dimensions;
             ++dimension_index_iter) {

          std::size_t dimension_length = (dimension_index_iter == dimension_index) ?
            length(dimension_index_iter) - 1 :
            length(dimension_index_iter);

          vertex_source[dimension_index_iter] = (vertex_offset / index_divider) %
            dimension_length;

          index_divider *= dimension_length;
        }

        if (is_forward) {
          vertex_target = next(vertex_source, dimension_index);
        }
        else {
          // Shift forward one more unit in the dimension for backward
          // edges since the algorithm above will leave us one behind.
          vertex_target = vertex_source;
          ++vertex_source[dimension_index];
        }

      } // if (wrapped(dimension_index))
      
      return (std::make_pair(vertex_source, vertex_target));
    }
    
    // Returns the index for [edge] (See also edge_at)
    edges_size_type index_of(edge_descriptor edge) const {
      vertex_descriptor source_vertex = source(edge, *this);
      vertex_descriptor target_vertex = target(edge, *this);

      // Determine the dimension where the source and target vertices
      // differ (should only be one if this is a valid edge).
      std::size_t different_dimension_index = 0;

      while (source_vertex[different_dimension_index] ==
             target_vertex[different_dimension_index]) {

        ++different_dimension_index; 
      }

      edges_size_type edge_index = 0;
      
      // Offset the edge index into the appropriate "bin" (see edge_at
      // for a more in-depth description).
      for (std::size_t dimension_index = 0;
           dimension_index < different_dimension_index;
           ++dimension_index) {

        edge_index += num_edges(dimension_index);      
      }

      // Get the position of both vertices in the differing dimension.
      vertices_size_type source_position = source_vertex[different_dimension_index];
      vertices_size_type target_position = target_vertex[different_dimension_index];

      // Determine if edge is forward or backward
      bool is_forward = true;
        
      if (wrapped(different_dimension_index)) {

        // If the dimension is wrapped, an edge is going backward if
        // either A: its target precedes the source in the differing
        // dimension and the vertices are adjacent or B: its source
        // precedes the target and they're not adjacent.
        if (((target_position < source_position) &&
             ((source_position - target_position) == 1)) ||
            ((source_position < target_position) &&
             ((target_position - source_position) > 1))) {

          is_forward = false;
        }
      }
      else if (target_position < source_position) {
        is_forward = false;
      }

      // "Backward" edges are in the second half of the bin.
      if (!is_forward) {
        edge_index += (num_edges(different_dimension_index) / 2);
      }

      // Finally, apply the vertex offset
      if (wrapped(different_dimension_index)) {
        edge_index += index_of(source_vertex);
      }
      else {
        vertices_size_type index_multiplier = 1;

        if (!is_forward) {
          --source_vertex[different_dimension_index];
        }

        for (std::size_t dimension_index = 0;
             dimension_index < Dimensions;
             ++dimension_index) {

          edge_index += (source_vertex[dimension_index] * index_multiplier);
          index_multiplier *= (dimension_index == different_dimension_index) ?
            length(dimension_index) - 1 :
            length(dimension_index);
        }
      }

      return (edge_index);
    }

    // Returns the number of out-edges for [vertex]
    degree_size_type out_degree(vertex_descriptor vertex) const {

      degree_size_type out_edge_count = 0;

      for (std::size_t dimension_index = 0;
           dimension_index < Dimensions;
           ++dimension_index) {

        // If the vertex is on the edge of this dimension, then its
        // number of out edges is dependent upon whether the dimension
        // wraps or not.
        if ((vertex[dimension_index] == 0) ||
            (vertex[dimension_index] == (length(dimension_index) - 1))) {
          out_edge_count += (wrapped(dimension_index) ? 2 : 1);
        }
        else {
          // Next and previous edges, regardless or wrapping
          out_edge_count += 2;
        }
      }

      return (out_edge_count);
    }

    // Returns an out-edge for [vertex] by index. Indices are in the
    // range [0, out_degree(vertex)).
    edge_descriptor out_edge_at
    (vertex_descriptor vertex,
     degree_size_type out_edge_index) const {

      edges_size_type edges_left = out_edge_index + 1;
      std::size_t dimension_index = 0;
      bool is_forward = false;

      // Walks the out edges of [vertex] and accommodates for dimension
      // wrapping.
      while (edges_left > 0) {

        if (!wrapped(dimension_index)) {
          if (!is_forward && (vertex[dimension_index] == 0)) {
            is_forward = true;
            continue;
          }
          else if (is_forward &&
                   (vertex[dimension_index] == (length(dimension_index) - 1))) {
            is_forward = false;
            ++dimension_index;
            continue;
          }
        }

        --edges_left;

        if (edges_left > 0) {
          is_forward = !is_forward;
        
          if (!is_forward) {
            ++dimension_index;
          }
        }
      }

      return (std::make_pair(vertex, is_forward ?
                             next(vertex, dimension_index) :
                             previous(vertex, dimension_index)));
    }

    // Returns the number of in-edges for [vertex]
    inline degree_size_type in_degree(vertex_descriptor vertex) const {
      return (out_degree(vertex));
    }

    // Returns an in-edge for [vertex] by index. Indices are in the
    // range [0, in_degree(vertex)).
    edge_descriptor in_edge_at
    (vertex_descriptor vertex,
     edges_size_type in_edge_index) const {

      edge_descriptor out_edge = out_edge_at(vertex, in_edge_index);
      return (std::make_pair(target(out_edge, *this), source(out_edge, *this)));

    }

    // Pre-computes the number of vertices and edges
    void precalculate() {
      m_num_vertices =
        std::accumulate(m_dimension_lengths.begin(),
                        m_dimension_lengths.end(), 1,
                        std::multiplies<vertices_size_type>());

      // Calculate number of edges in each dimension
      m_num_edges = 0;

      for (std::size_t dimension_index = 0;
           dimension_index < Dimensions;
           ++dimension_index) {

        if (wrapped(dimension_index)) {
          m_edge_count[dimension_index] = num_vertices() * 2;
        }
        else {
          m_edge_count[dimension_index] =
            (num_vertices() - (num_vertices() / length(dimension_index))) * 2;
        }

        m_num_edges += num_edges(dimension_index);
      }
    }

    const vertex_descriptor m_dimension_lengths;
    WrapDimensionArray m_wrap_dimension;
    vertices_size_type m_num_vertices;

    boost::array<edges_size_type, Dimensions> m_edge_count;
    edges_size_type m_num_edges;

  public:

    //================
    // VertexListGraph
    //================

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline std::pair<BOOST_GRID_GRAPH_TYPE_MEM vertex_iterator,
                            BOOST_GRID_GRAPH_TYPE_MEM vertex_iterator> 
    vertices(const BOOST_GRID_GRAPH_TYPE& graph) {
      BOOST_GRID_GRAPH_TYPE_TD(vertex_iterator);
      BOOST_GRID_GRAPH_TYPE_TD(vertex_function);
      BOOST_GRID_GRAPH_TYPE_TD(vertex_index_iterator);

      return (std::make_pair
              (vertex_iterator(vertex_index_iterator(0),
                               vertex_function(&graph)),
               vertex_iterator(vertex_index_iterator(graph.num_vertices()),
                               vertex_function(&graph))));
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline  BOOST_GRID_GRAPH_TYPE_MEM vertices_size_type
    num_vertices(const BOOST_GRID_GRAPH_TYPE& graph) {
      return (graph.num_vertices());
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor
    vertex(BOOST_GRID_GRAPH_TYPE_MEM vertices_size_type vertex_index,
           const BOOST_GRID_GRAPH_TYPE& graph) {

      return (graph.vertex_at(vertex_index));
    }

    //===============
    // IncidenceGraph
    //===============

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline std::pair<BOOST_GRID_GRAPH_TYPE_MEM out_edge_iterator,
                            BOOST_GRID_GRAPH_TYPE_MEM out_edge_iterator>
    out_edges(BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex,
              const BOOST_GRID_GRAPH_TYPE& graph) {
      BOOST_GRID_GRAPH_TYPE_TD(degree_iterator);
      BOOST_GRID_GRAPH_TYPE_TD(out_edge_function);
      BOOST_GRID_GRAPH_TYPE_TD(out_edge_iterator);

      return (std::make_pair
              (out_edge_iterator(degree_iterator(0),
                                 out_edge_function(vertex, &graph)),
               out_edge_iterator(degree_iterator(graph.out_degree(vertex)),
                                 out_edge_function(vertex, &graph))));
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM degree_size_type
    out_degree
    (BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex,
     const BOOST_GRID_GRAPH_TYPE& graph) {
      return (graph.out_degree(vertex));
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM edge_descriptor
    out_edge_at(BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex,
                BOOST_GRID_GRAPH_TYPE_MEM degree_size_type out_edge_index,
                const BOOST_GRID_GRAPH_TYPE& graph) {
      return (graph.out_edge_at(vertex, out_edge_index));
    }

    //===============
    // AdjacencyGraph
    //===============

    template <BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend typename std::pair<BOOST_GRID_GRAPH_TYPE_MEM adjacency_iterator,
                              BOOST_GRID_GRAPH_TYPE_MEM adjacency_iterator>
    adjacent_vertices (BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex,
                       const BOOST_GRID_GRAPH_TYPE& graph) {
      BOOST_GRID_GRAPH_TYPE_TD(degree_iterator);
      BOOST_GRID_GRAPH_TYPE_TD(adjacent_vertex_function);
      BOOST_GRID_GRAPH_TYPE_TD(adjacency_iterator);

      return (std::make_pair
              (adjacency_iterator(degree_iterator(0),
                                 adjacent_vertex_function(vertex, &graph)),
               adjacency_iterator(degree_iterator(graph.out_degree(vertex)),
                                 adjacent_vertex_function(vertex, &graph))));
    }

    //==============
    // EdgeListGraph
    //==============

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM edges_size_type
    num_edges(const BOOST_GRID_GRAPH_TYPE& graph) {
      return (graph.num_edges());
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM edge_descriptor
    edge_at(BOOST_GRID_GRAPH_TYPE_MEM edges_size_type edge_index,
            const BOOST_GRID_GRAPH_TYPE& graph) {
      return (graph.edge_at(edge_index));
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline std::pair<BOOST_GRID_GRAPH_TYPE_MEM edge_iterator,
                            BOOST_GRID_GRAPH_TYPE_MEM edge_iterator>
    edges(const BOOST_GRID_GRAPH_TYPE& graph) {
      BOOST_GRID_GRAPH_TYPE_TD(edge_index_iterator);
      BOOST_GRID_GRAPH_TYPE_TD(edge_function);
      BOOST_GRID_GRAPH_TYPE_TD(edge_iterator);

      return (std::make_pair
              (edge_iterator(edge_index_iterator(0),
                             edge_function(&graph)),
               edge_iterator(edge_index_iterator(graph.num_edges()),
                             edge_function(&graph))));
    }

    //===================
    // BiDirectionalGraph
    //===================

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline std::pair<BOOST_GRID_GRAPH_TYPE_MEM in_edge_iterator,
                            BOOST_GRID_GRAPH_TYPE_MEM in_edge_iterator>
    in_edges(BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex,
             const BOOST_GRID_GRAPH_TYPE& graph) {
      BOOST_GRID_GRAPH_TYPE_TD(in_edge_function);
      BOOST_GRID_GRAPH_TYPE_TD(degree_iterator);
      BOOST_GRID_GRAPH_TYPE_TD(in_edge_iterator);

      return (std::make_pair
              (in_edge_iterator(degree_iterator(0),
                                in_edge_function(vertex, &graph)),
               in_edge_iterator(degree_iterator(graph.in_degree(vertex)),
                                in_edge_function(vertex, &graph))));
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM degree_size_type
    in_degree (BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex,
               const BOOST_GRID_GRAPH_TYPE& graph) {
      return (graph.in_degree(vertex));
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM degree_size_type
    degree (BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex,
            const BOOST_GRID_GRAPH_TYPE& graph) {
      return (graph.out_degree(vertex) * 2);
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM edge_descriptor
    in_edge_at(BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex,
               BOOST_GRID_GRAPH_TYPE_MEM degree_size_type in_edge_index,
               const BOOST_GRID_GRAPH_TYPE& graph) {
      return (graph.in_edge_at(vertex, in_edge_index));
    }


    //==================
    // Adjacency Matrix
    //==================

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend std::pair<BOOST_GRID_GRAPH_TYPE_MEM edge_descriptor, bool>
    edge (BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor source_vertex,
          BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor destination_vertex,
          const BOOST_GRID_GRAPH_TYPE& graph) {

      std::pair<BOOST_GRID_GRAPH_TYPE_MEM edge_descriptor, bool> edge_exists =
        std::make_pair(std::make_pair(source_vertex, destination_vertex), false);

      for (std::size_t dimension_index = 0;
           dimension_index < Dimensions;
           ++dimension_index) {

        BOOST_GRID_GRAPH_TYPE_MEM vertices_size_type dim_difference = 0;
        BOOST_GRID_GRAPH_TYPE_MEM vertices_size_type
          source_dim = source_vertex[dimension_index],
          dest_dim = destination_vertex[dimension_index];

        dim_difference = (source_dim > dest_dim) ?
          (source_dim - dest_dim) : (dest_dim - source_dim);

        if (dim_difference > 0) {

          // If we've already found a valid edge, this would mean that
          // the vertices are really diagonal across dimensions and
          // therefore not connected.
          if (edge_exists.second) {
            edge_exists.second = false;
            break;
          }

          // If the difference is one, the vertices are right next to
          // each other and the edge is valid.  The edge is still
          // valid, though, if the dimension wraps and the vertices
          // are on opposite ends.
          if ((dim_difference == 1) ||
              (graph.wrapped(dimension_index) &&
               (((source_dim == 0) && (dest_dim == (graph.length(dimension_index) - 1))) ||
                ((dest_dim == 0) && (source_dim == (graph.length(dimension_index) - 1)))))) {

            edge_exists.second = true;
            // Stay in the loop to check for diagonal vertices.
          }
          else {

            // Stop checking - the vertices are too far apart.
            edge_exists.second = false;
            break;
          }
        }

      } // for dimension_index

      return (edge_exists);
    }


    //=============================
    // Index Property Map Functions
    //=============================

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM vertices_size_type
    get(vertex_index_t,
        const BOOST_GRID_GRAPH_TYPE& graph,
        BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor vertex) {
      return (graph.index_of(vertex));
    }

    template<BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline BOOST_GRID_GRAPH_TYPE_MEM edges_size_type
    get(edge_index_t,
        const BOOST_GRID_GRAPH_TYPE& graph,
        BOOST_GRID_GRAPH_TYPE_MEM edge_descriptor edge) {
      return (graph.index_of(edge));
    }

    template <BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline grid_graph_index_map<
                    BOOST_GRID_GRAPH_TYPE,
                    BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor,
                    BOOST_GRID_GRAPH_TYPE_MEM vertices_size_type>
    get(vertex_index_t, const BOOST_GRID_GRAPH_TYPE& graph) {
      return (grid_graph_index_map<
                BOOST_GRID_GRAPH_TYPE,
                BOOST_GRID_GRAPH_TYPE_MEM vertex_descriptor,
                BOOST_GRID_GRAPH_TYPE_MEM vertices_size_type>(graph));
    }

    template <BOOST_GRID_GRAPH_TEMPLATE_PARAMS>
    friend inline grid_graph_index_map<
                    BOOST_GRID_GRAPH_TYPE,
                    BOOST_GRID_GRAPH_TYPE_MEM edge_descriptor,
                    BOOST_GRID_GRAPH_TYPE_MEM edges_size_type>
    get(edge_index_t, const BOOST_GRID_GRAPH_TYPE& graph) {
      return (grid_graph_index_map<
                BOOST_GRID_GRAPH_TYPE,
                BOOST_GRID_GRAPH_TYPE_MEM edge_descriptor,
                BOOST_GRID_GRAPH_TYPE_MEM edges_size_type>(graph));
    }                                       

    template<typename Graph,
             typename Descriptor,
             typename Index>
    friend inline Index
    get(const grid_graph_index_map<Graph, Descriptor, Index>& index_map,
        const typename grid_graph_index_map<Graph, Descriptor, Index>::key_type& key)
    {
      return (index_map[key]);
    }

    template<typename Graph,
             typename Descriptor,
             typename Index>
    friend struct grid_graph_index_map;

  }; // grid_graph

} // namespace boost

#undef BOOST_GRID_GRAPH_TYPE
#undef BOOST_GRID_GRAPH_TYPE_TD
#undef BOOST_GRID_GRAPH_TYPE_MEM
#undef BOOST_GRID_GRAPH_TEMPLATE_PARAMS
#undef BOOST_GRID_GRAPH_TRAITS_T

#endif // BOOST_GRAPH_GRID_GRAPH_HPP
