/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2009
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_ANY_NODE_HPP
#define BOOST_INTRUSIVE_ANY_NODE_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <iterator>
#include <boost/intrusive/detail/assert.hpp>
#include <boost/intrusive/detail/pointer_to_other.hpp>
#include <cstddef>
#include <boost/intrusive/detail/mpl.hpp> 
#include <boost/pointer_cast.hpp>

namespace boost {
namespace intrusive {

template<class VoidPointer>
struct any_node
{
   typedef typename boost::pointer_to_other
      <VoidPointer, any_node>::type   node_ptr;
   node_ptr    node_ptr_1;
   node_ptr    node_ptr_2;
   node_ptr    node_ptr_3;
   std::size_t size_t_1;
};

template<class VoidPointer>
struct any_list_node_traits
{
   typedef any_node<VoidPointer> node;
   typedef typename boost::pointer_to_other
      <VoidPointer, node>::type          node_ptr;
   typedef typename boost::pointer_to_other
      <VoidPointer, const node>::type    const_node_ptr;

   static node_ptr get_next(const_node_ptr n)
   {  return n->node_ptr_1;  }

   static void set_next(node_ptr n, node_ptr next)
   {  n->node_ptr_1 = next;  }

   static node_ptr get_previous(const_node_ptr n)
   {  return n->node_ptr_2;  }

   static void set_previous(node_ptr n, node_ptr prev)
   {  n->node_ptr_2 = prev;  }
};


template<class VoidPointer>
struct any_slist_node_traits
{
   typedef any_node<VoidPointer> node;
   typedef typename boost::pointer_to_other
      <VoidPointer, node>::type          node_ptr;
   typedef typename boost::pointer_to_other
      <VoidPointer, const node>::type    const_node_ptr;

   static node_ptr get_next(const_node_ptr n)
   {  return n->node_ptr_1;  }

   static void set_next(node_ptr n, node_ptr next)
   {  n->node_ptr_1 = next;  }
};


template<class VoidPointer>
struct any_unordered_node_traits
   :  public any_slist_node_traits<VoidPointer>
{
   typedef any_slist_node_traits<VoidPointer>                  reduced_slist_node_traits;
   typedef typename reduced_slist_node_traits::node            node;
   typedef typename reduced_slist_node_traits::node_ptr        node_ptr;
   typedef typename reduced_slist_node_traits::const_node_ptr  const_node_ptr;

   static const bool store_hash        = true;
   static const bool optimize_multikey = true;

   static node_ptr get_next(const_node_ptr n)
   {
      using ::boost::static_pointer_cast;
      return static_pointer_cast<node>(n->node_ptr_1);
   }

   static void set_next(node_ptr n, node_ptr next)
   {  n->node_ptr_1 = next;  }

   static node_ptr get_prev_in_group(const_node_ptr n)
   {  return n->node_ptr_2;  }

   static void set_prev_in_group(node_ptr n, node_ptr prev)
   {  n->node_ptr_2 = prev;  }

   static std::size_t get_hash(const_node_ptr n)
   {  return n->size_t_1;  }  

   static void set_hash(node_ptr n, std::size_t h)
   {  n->size_t_1 = h;  }  
};


template<class VoidPointer>
struct any_rbtree_node_traits
{
   typedef any_node<VoidPointer> node;

   typedef typename boost::pointer_to_other
      <VoidPointer, node>::type          node_ptr;
   typedef typename boost::pointer_to_other
      <VoidPointer, const node>::type    const_node_ptr;

   typedef std::size_t color;

   static node_ptr get_parent(const_node_ptr n)
   {  return n->node_ptr_1;  }

   static void set_parent(node_ptr n, node_ptr p)
   {  n->node_ptr_1 = p;  }

   static node_ptr get_left(const_node_ptr n)
   {  return n->node_ptr_2;  }

   static void set_left(node_ptr n, node_ptr l)
   {  n->node_ptr_2 = l;  }

   static node_ptr get_right(const_node_ptr n)
   {  return n->node_ptr_3;  }

   static void set_right(node_ptr n, node_ptr r)
   {  n->node_ptr_3 = r;  }

   static color get_color(const_node_ptr n)
   {  return n->size_t_1;  }

   static void set_color(node_ptr n, color c)
   {  n->size_t_1 = c;  }

   static color black()
   {  return 0u;  }

   static color red()
   {  return 1u;  }
};


template<class VoidPointer>
struct any_avltree_node_traits
{
   typedef any_node<VoidPointer> node;

   typedef typename boost::pointer_to_other
      <VoidPointer, node>::type          node_ptr;
   typedef typename boost::pointer_to_other
      <VoidPointer, const node>::type    const_node_ptr;
   typedef std::size_t balance;

   static node_ptr get_parent(const_node_ptr n)
   {  return n->node_ptr_1;  }

   static void set_parent(node_ptr n, node_ptr p)
   {  n->node_ptr_1 = p;  }

   static node_ptr get_left(const_node_ptr n)
   {  return n->node_ptr_2;  }

   static void set_left(node_ptr n, node_ptr l)
   {  n->node_ptr_2 = l;  }

   static node_ptr get_right(const_node_ptr n)
   {  return n->node_ptr_3;  }

   static void set_right(node_ptr n, node_ptr r)
   {  n->node_ptr_3 = r;  }

   static balance get_balance(const_node_ptr n)
   {  return n->size_t_1;  }

   static void set_balance(node_ptr n, balance b)
   {  n->size_t_1 = b;  }

   static balance negative()
   {  return 0u;  }

   static balance zero()
   {  return 1u;  }

   static balance positive()
   {  return 2u;  }
};


template<class VoidPointer>
struct any_tree_node_traits
{
   typedef any_node<VoidPointer> node;

   typedef typename boost::pointer_to_other
      <VoidPointer, node>::type              node_ptr;
   typedef typename boost::pointer_to_other
      <VoidPointer, const node>::type        const_node_ptr;

   static node_ptr get_parent(const_node_ptr n)
   {  return n->node_ptr_1;  }

   static void set_parent(node_ptr n, node_ptr p)
   {  n->node_ptr_1 = p;  }

   static node_ptr get_left(const_node_ptr n)
   {  return n->node_ptr_2;  }

   static void set_left(node_ptr n, node_ptr l)
   {  n->node_ptr_2 = l;  }

   static node_ptr get_right(const_node_ptr n)
   {  return n->node_ptr_3;  }

   static void set_right(node_ptr n, node_ptr r)
   {  n->node_ptr_3 = r;  }
};

template<class VoidPointer>
class any_node_traits
{
   public:
   typedef any_node<VoidPointer>          node;
   typedef typename boost::pointer_to_other
      <VoidPointer, node>::type          node_ptr;
   typedef typename boost::pointer_to_other
      <VoidPointer, const node>::type    const_node_ptr;
};

template<class VoidPointer>
class any_algorithms
{
   template <class T>
   static void function_not_available_for_any_hooks(typename detail::enable_if<detail::is_same<T, bool> >::type)
   {}

   public:
   typedef any_node<VoidPointer>             node;
   typedef typename boost::pointer_to_other
      <VoidPointer, node>::type              node_ptr;
   typedef typename boost::pointer_to_other
      <VoidPointer, const node>::type        const_node_ptr;
   typedef any_node_traits<VoidPointer>      node_traits;

   //! <b>Requires</b>: node must not be part of any tree.
   //!
   //! <b>Effects</b>: After the function unique(node) == true.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Nodes</b>: If node is inserted in a tree, this function corrupts the tree.
   static void init(node_ptr node)
   {  node->node_ptr_1 = 0;   };

   //! <b>Effects</b>: Returns true if node is in the same state as if called init(node)
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   static bool inited(const_node_ptr node)
   {  return !node->node_ptr_1;  };

   static bool unique(const_node_ptr node)
   {  return 0 == node->node_ptr_1; }

   static void unlink(node_ptr)
   {
      //Auto-unlink hooks and unlink() are not available for any hooks
      any_algorithms<VoidPointer>::template function_not_available_for_any_hooks<node_ptr>();
   }

   static void swap_nodes(node_ptr l, node_ptr r)
   {
      //Any nodes have no swap_nodes capability because they don't know
      //what algorithm they must use to unlink the node from the container
      any_algorithms<VoidPointer>::template function_not_available_for_any_hooks<node_ptr>();
   }
};

} //namespace intrusive 
} //namespace boost 

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_ANY_NODE_HPP
