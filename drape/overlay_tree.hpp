#pragma once

#include "overlay_handle.hpp"

#include "../base/buffer_vector.hpp"
#include "../geometry/screenbase.hpp"

#include "../std/kdtree.hpp"

class OverlayTree
{
public:
  void StartOverlayPlacing(ScreenBase const & screen, bool canOverlap = false);
  void Add(RefPointer<OverlayHandle> handle);
  void EndOverlayPlacing();

private:
  struct Node
  {
    typedef double value_type;

    Node(RefPointer<OverlayHandle> handle, m2::RectD const & pixelRect);

    RefPointer<OverlayHandle> m_nodeValue;
    double m_pts[4];

    double operator[] (size_t i) const { return m_pts[i]; }
  };

  ScreenBase m_modelView;
  typedef KDTree::KDTree<4, Node> tree_t;
  tree_t m_tree;
  bool m_canOverlap;

private:
  typedef buffer_vector<Node const *, 8> find_result_t;

  class BaseFindFunctor
  {
  public:
    BaseFindFunctor(m2::RectD const & r);

    bool ScanLeft(size_t plane, Node const & v) const;
    bool ScanRight(size_t plane, Node const & v) const;

  protected:
    m2::RectD const & m_rect;
  };

  class FindIntersectedFunctor : public BaseFindFunctor
  {
    typedef BaseFindFunctor base_t;
  public:
    FindIntersectedFunctor(m2::RectD const & r, find_result_t & intersections);

    void operator()(Node const & node);

  private:
    find_result_t & m_intersections;
  };
};
