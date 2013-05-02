//----------------------------------------------------------------------------------
// File:            libs\jni\nv_event\nv_event_queue.h
// Samples Version: NVIDIA Android Lifecycle samples 1_0beta 
// Email:           tegradev@nvidia.com
// Web:             http://developer.nvidia.com/category/zone/mobile-development
//
// Copyright 2009-2011 NVIDIA® Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//----------------------------------------------------------------------------------

#ifndef NV_EVENT_QUEUE
#define NV_EVENT_QUEUE

#include "nv_event.hpp"
#include <pthread.h>

class NVEventSync
{
public:
  pthread_cond_t m_cond;
  bool           m_block;
};

class NVEventQueue {
public:
  void Init();
  void Shutdown();
  void Flush();
  void UnblockConsumer();
  void UnblockProducer();
  // Events are copied, so caller can reuse ev immediately
  void Insert(const NVEvent* ev);
  // Waits until the event is consumed.  Returns whether the
  // consumer indicates handling the event or ignoring it
  bool InsertBlocking(const NVEvent* ev);

  // Returned event is valid only until the next call to
  // RemoveOldest or until a call to DoneWithEvent
  // Calling RemoveOldest again without calling DoneWithEvent
  // indicates that the last event returned was NOT handled, and
  // thus InsertNewestAndWait for that even would return false
  const NVEvent* RemoveOldest(int waitMSecs);

  // Indicates that all processing of the last event returned
  // from RemoveOldest is complete.  Also allows the app to indicate
  // whether it handled the event or not.
  // Do not dereference the last event pointer after calling this function
  void DoneWithEvent(bool ret);

protected:
  bool insert(const NVEvent* ev);

  enum { QUEUE_ELEMS = 256 };
  enum { QUEUE_MASK = 0x000000ff };

  int32_t         m_nextInsertIndex;
  int32_t         m_headIndex;

  pthread_mutex_t m_accessLock;

  NVEventSync     m_blockerSync;
  NVEventSync     m_consumerSync;

  NVEvent m_events[QUEUE_ELEMS];
  const NVEvent* m_blocker;
  enum BlockerState
  {
    NO_BLOCKER,
    PENDING_BLOCKER,
    PROCESSING_BLOCKER,
    RETURNED_BLOCKER
  };
  BlockerState m_blockerState;
  bool m_blockerReturnVal;
};




#endif // #ifndef NV_EVENT_QUEUE
