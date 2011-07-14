//
// detail/timer_queue.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_TIMER_QUEUE_HPP
#define BOOST_ASIO_DETAIL_TIMER_QUEUE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <vector>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/detail/timer_op.hpp>
#include <boost/asio/detail/timer_queue_base.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/time_traits.hpp>

#include <boost/asio/detail/push_options.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Time_Traits>
class timer_queue
  : public timer_queue_base
{
public:
  // The time type.
  typedef typename Time_Traits::time_type time_type;

  // The duration type.
  typedef typename Time_Traits::duration_type duration_type;

  // Per-timer data.
  class per_timer_data
  {
  public:
    per_timer_data() : next_(0), prev_(0) {}

  private:
    friend class timer_queue;

    // The operations waiting on the timer.
    op_queue<timer_op> op_queue_;

    // The index of the timer in the heap.
    std::size_t heap_index_;

    // Pointers to adjacent timers in a linked list.
    per_timer_data* next_;
    per_timer_data* prev_;
  };

  // Constructor.
  timer_queue()
    : timers_(),
      heap_()
  {
  }

  // Add a new timer to the queue. Returns true if this is the timer that is
  // earliest in the queue, in which case the reactor's event demultiplexing
  // function call may need to be interrupted and restarted.
  bool enqueue_timer(const time_type& time, per_timer_data& timer, timer_op* op)
  {
    // Enqueue the timer object.
    if (timer.prev_ == 0 && &timer != timers_)
    {
      if (this->is_positive_infinity(time))
      {
        // No heap entry is required for timers that never expire.
        timer.heap_index_ = (std::numeric_limits<std::size_t>::max)();
      }
      else
      {
        // Put the new timer at the correct position in the heap. This is done
        // first since push_back() can throw due to allocation failure.
        timer.heap_index_ = heap_.size();
        heap_entry entry = { time, &timer };
        heap_.push_back(entry);
        up_heap(heap_.size() - 1);
      }

      // Insert the new timer into the linked list of active timers.
      timer.next_ = timers_;
      timer.prev_ = 0;
      if (timers_)
        timers_->prev_ = &timer;
      timers_ = &timer;
    }

    // Enqueue the individual timer operation.
    timer.op_queue_.push(op);

    // Interrupt reactor only if newly added timer is first to expire.
    return timer.heap_index_ == 0 && timer.op_queue_.front() == op;
  }

  // Whether there are no timers in the queue.
  virtual bool empty() const
  {
    return timers_ == 0;
  }

  // Get the time for the timer that is earliest in the queue.
  virtual long wait_duration_msec(long max_duration) const
  {
    if (heap_.empty())
      return max_duration;

    boost::posix_time::time_duration duration = Time_Traits::to_posix_duration(
        Time_Traits::subtract(heap_[0].time_, Time_Traits::now()));

    if (duration > boost::posix_time::milliseconds(max_duration))
      duration = boost::posix_time::milliseconds(max_duration);
    else if (duration <= boost::posix_time::milliseconds(0))
      duration = boost::posix_time::milliseconds(0);
    else if (duration < boost::posix_time::milliseconds(1))
      duration = boost::posix_time::milliseconds(1);

    return duration.total_milliseconds();
  }

  // Get the time for the timer that is earliest in the queue.
  virtual long wait_duration_usec(long max_duration) const
  {
    if (heap_.empty())
      return max_duration;

    boost::posix_time::time_duration duration = Time_Traits::to_posix_duration(
        Time_Traits::subtract(heap_[0].time_, Time_Traits::now()));

    if (duration > boost::posix_time::microseconds(max_duration))
      duration = boost::posix_time::microseconds(max_duration);
    else if (duration <= boost::posix_time::microseconds(0))
      duration = boost::posix_time::microseconds(0);
    else if (duration < boost::posix_time::microseconds(1))
      duration = boost::posix_time::microseconds(1);

    return duration.total_microseconds();
  }

  // Dequeue all timers not later than the current time.
  virtual void get_ready_timers(op_queue<operation>& ops)
  {
    const time_type now = Time_Traits::now();
    while (!heap_.empty() && !Time_Traits::less_than(now, heap_[0].time_))
    {
      per_timer_data* timer = heap_[0].timer_;
      ops.push(timer->op_queue_);
      remove_timer(*timer);
    }
  }

  // Dequeue all timers.
  virtual void get_all_timers(op_queue<operation>& ops)
  {
    while (timers_)
    {
      per_timer_data* timer = timers_;
      timers_ = timers_->next_;
      ops.push(timer->op_queue_);
      timer->next_ = 0;
      timer->prev_ = 0;
    }

    heap_.clear();
  }

  // Cancel and dequeue operations for the given timer.
  std::size_t cancel_timer(per_timer_data& timer, op_queue<operation>& ops,
      std::size_t max_cancelled = (std::numeric_limits<std::size_t>::max)())
  {
    std::size_t num_cancelled = 0;
    if (timer.prev_ != 0 || &timer == timers_)
    {
      while (timer_op* op = (num_cancelled != max_cancelled)
          ? timer.op_queue_.front() : 0)
      {
        op->ec_ = boost::asio::error::operation_aborted;
        timer.op_queue_.pop();
        ops.push(op);
        ++num_cancelled;
      }
      if (timer.op_queue_.empty())
        remove_timer(timer);
    }
    return num_cancelled;
  }

private:
  // Move the item at the given index up the heap to its correct position.
  void up_heap(std::size_t index)
  {
    std::size_t parent = (index - 1) / 2;
    while (index > 0
        && Time_Traits::less_than(heap_[index].time_, heap_[parent].time_))
    {
      swap_heap(index, parent);
      index = parent;
      parent = (index - 1) / 2;
    }
  }

  // Move the item at the given index down the heap to its correct position.
  void down_heap(std::size_t index)
  {
    std::size_t child = index * 2 + 1;
    while (child < heap_.size())
    {
      std::size_t min_child = (child + 1 == heap_.size()
          || Time_Traits::less_than(
            heap_[child].time_, heap_[child + 1].time_))
        ? child : child + 1;
      if (Time_Traits::less_than(heap_[index].time_, heap_[min_child].time_))
        break;
      swap_heap(index, min_child);
      index = min_child;
      child = index * 2 + 1;
    }
  }

  // Swap two entries in the heap.
  void swap_heap(std::size_t index1, std::size_t index2)
  {
    heap_entry tmp = heap_[index1];
    heap_[index1] = heap_[index2];
    heap_[index2] = tmp;
    heap_[index1].timer_->heap_index_ = index1;
    heap_[index2].timer_->heap_index_ = index2;
  }

  // Remove a timer from the heap and list of timers.
  void remove_timer(per_timer_data& timer)
  {
    // Remove the timer from the heap.
    std::size_t index = timer.heap_index_;
    if (!heap_.empty() && index < heap_.size())
    {
      if (index == heap_.size() - 1)
      {
        heap_.pop_back();
      }
      else
      {
        swap_heap(index, heap_.size() - 1);
        heap_.pop_back();
        std::size_t parent = (index - 1) / 2;
        if (index > 0 && Time_Traits::less_than(
              heap_[index].time_, heap_[parent].time_))
          up_heap(index);
        else
          down_heap(index);
      }
    }

    // Remove the timer from the linked list of active timers.
    if (timers_ == &timer)
      timers_ = timer.next_;
    if (timer.prev_)
      timer.prev_->next_ = timer.next_;
    if (timer.next_)
      timer.next_->prev_= timer.prev_;
    timer.next_ = 0;
    timer.prev_ = 0;
  }

  // Determine if the specified absolute time is positive infinity.
  template <typename Time_Type>
  static bool is_positive_infinity(const Time_Type&)
  {
    return false;
  }

  // Determine if the specified absolute time is positive infinity.
  static bool is_positive_infinity(const boost::posix_time::ptime& time)
  {
    return time == boost::posix_time::pos_infin;
  }

  // The head of a linked list of all active timers.
  per_timer_data* timers_;

  struct heap_entry
  {
    // The time when the timer should fire.
    time_type time_;

    // The associated timer with enqueued operations.
    per_timer_data* timer_;
  };

  // The heap of timers, with the earliest timer at the front.
  std::vector<heap_entry> heap_;
};

#if !defined(BOOST_ASIO_HEADER_ONLY)

struct forwarding_posix_time_traits : time_traits<boost::posix_time::ptime> {};

// Template specialisation for the commonly used instantation.
template <>
class timer_queue<time_traits<boost::posix_time::ptime> >
  : public timer_queue_base
{
public:
  // The time type.
  typedef boost::posix_time::ptime time_type;

  // The duration type.
  typedef boost::posix_time::time_duration duration_type;

  // Per-timer data.
  typedef timer_queue<forwarding_posix_time_traits>::per_timer_data
    per_timer_data;

  // Constructor.
  BOOST_ASIO_DECL timer_queue();

  // Destructor.
  BOOST_ASIO_DECL virtual ~timer_queue();

  // Add a new timer to the queue. Returns true if this is the timer that is
  // earliest in the queue, in which case the reactor's event demultiplexing
  // function call may need to be interrupted and restarted.
  BOOST_ASIO_DECL bool enqueue_timer(const time_type& time,
      per_timer_data& timer, timer_op* op);

  // Whether there are no timers in the queue.
  BOOST_ASIO_DECL virtual bool empty() const;

  // Get the time for the timer that is earliest in the queue.
  BOOST_ASIO_DECL virtual long wait_duration_msec(long max_duration) const;

  // Get the time for the timer that is earliest in the queue.
  BOOST_ASIO_DECL virtual long wait_duration_usec(long max_duration) const;

  // Dequeue all timers not later than the current time.
  BOOST_ASIO_DECL virtual void get_ready_timers(op_queue<operation>& ops);

  // Dequeue all timers.
  BOOST_ASIO_DECL virtual void get_all_timers(op_queue<operation>& ops);

  // Cancel and dequeue operations for the given timer.
  BOOST_ASIO_DECL std::size_t cancel_timer(
      per_timer_data& timer, op_queue<operation>& ops,
      std::size_t max_cancelled = (std::numeric_limits<std::size_t>::max)());

private:
  timer_queue<forwarding_posix_time_traits> impl_;
};

#endif // !defined(BOOST_ASIO_HEADER_ONLY)

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_TIMER_QUEUE_HPP
