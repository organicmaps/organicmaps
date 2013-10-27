//----------------------------------------------------------------------------------
// File:            libs\jni\nv_event\nv_event_queue.cpp
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
#define MODULE "NVEventQueue"
#include "../nv_debug/nv_debug.hpp"

#include "nv_event_queue.hpp"
#include <assert.h>
#include <stdlib.h>
#include <jni.h>
#include <pthread.h>
#include <android/log.h>

#define NVNextWrapped(index) (((index) + 1) & QUEUE_MASK)
#define NVPrevWrapped(index) (((index) - 1) & QUEUE_MASK)

/* you must be inside a m_accessLock lock to invoke this! */
static void unlockAll(NVEventSync* sem)
{
  sem->m_block = false;
  pthread_cond_broadcast(&(sem->m_cond));
}

/* you must be inside mutex lock to invoke this! */
static int32_t wait(NVEventSync* sem, pthread_mutex_t* mutex, int waitMS)
{
  // TBD - spec is dodgy; do we definitely release the mutex even if
  // wait fails?
  if(sem->m_block)
  {
    if( waitMS < 0 )
      return pthread_cond_wait(&sem->m_cond, mutex);
    else
      return pthread_cond_timeout_np(&sem->m_cond, mutex, (unsigned)waitMS);
  }
  else
  {
    // must release this, as failure assumes no lock
    pthread_mutex_unlock(mutex);
    return 1; // do not return 0 - we do not own the lock!
  }
}

static void signal(NVEventSync* sem)
{
  pthread_cond_signal(&sem->m_cond);
}

static void broadcast(NVEventSync* sem)
{
  pthread_cond_broadcast(&sem->m_cond);
}

static void syncInit( NVEventSync* sync )
{
  pthread_cond_init(&(sync->m_cond), NULL);
  sync->m_block = true;
}

static void syncDestroy( NVEventSync* sync )
{
  pthread_cond_destroy( &sync->m_cond );
}

/* you must be inside a m_accessLock lock to invoke this! */
bool NVEventQueue::insert(const NVEvent* ev)
{
  // Is the queue full?
  int32_t nextNext = NVNextWrapped(m_nextInsertIndex);
  if (nextNext == m_headIndex)
  {
    // TBD - what to do when we cannot insert (full queue)
    return false;
  }

  NVEvent* evDest = m_events + m_nextInsertIndex;
  memcpy(evDest, ev, sizeof(NVEvent));

  m_nextInsertIndex = nextNext;
  return true;
}

void NVEventQueue::Init()
{
  m_nextInsertIndex = 0;
  m_headIndex = 0;
  pthread_mutex_init(&(m_accessLock), NULL);
  syncInit(&m_consumerSync);
  syncInit(&m_blockerSync);

  m_blocker = NULL;
  m_blockerState = NO_BLOCKER;
  m_blockerReturnVal = false;
}

void NVEventQueue::Shutdown()
{
  pthread_mutex_destroy(&(m_accessLock));

  // free everyone...
  unlockAll(&m_consumerSync);
  unlockAll(&m_blockerSync);
  syncDestroy(&(m_consumerSync));
  syncDestroy(&(m_blockerSync));
}

void NVEventQueue::Flush()
{
  // TBD: Lock the mutex????
  m_headIndex = m_nextInsertIndex;
}

void NVEventQueue::UnblockConsumer()
{
  unlockAll(&(m_consumerSync));
}

void NVEventQueue::UnblockProducer()
{
  unlockAll(&(m_blockerSync));
}

void NVEventQueue::Insert(const NVEvent* ev)
{
  pthread_mutex_lock(&(m_accessLock));

  // insert the event and unblock a waiter
  insert(ev);
  signal(&m_consumerSync);
  pthread_mutex_unlock(&(m_accessLock));
}

bool NVEventQueue::InsertBlocking(const NVEvent* ev)
{
  // TBD - how to handle the destruction of these mutexes

  pthread_mutex_lock(&(m_accessLock));
  while (m_blocker)
  {
    if (wait(&(m_blockerSync), &(m_accessLock), NV_EVENT_WAIT_FOREVER))
      return false;
  }
    
  assert(!m_blocker);
  assert(m_blockerState == NO_BLOCKER);

  // we have the mutex _and_ the blocking event is NULL
  // So now we can push a new one
  m_blocker = ev;
  m_blockerState = PENDING_BLOCKER;

  // Release the consumer, as we just posted a new event
  signal(&(m_consumerSync));

  // Loop on the condition variable until we find out that
  // there is a return value waiting for us.  Since only we
  // will null the blocker pointer, we will not let anyone
  // else start to post a blocking event
  while (m_blockerState != RETURNED_BLOCKER)
  {
    if (wait(&(m_blockerSync), &(m_accessLock), NV_EVENT_WAIT_FOREVER))
      return false;
  }

  bool handled = m_blockerReturnVal;
  m_blocker = NULL;
  m_blockerState = NO_BLOCKER;

  NVDEBUG("producer unblocking from consumer handling blocking event (%s)", handled ? "true" : "false");
    	
  // We've handled the event, so the producer can release the
  // next thread to potentially post a blocking event
  signal(&(m_blockerSync));
  pthread_mutex_unlock(&(m_accessLock));

  return handled;
}


const NVEvent* NVEventQueue::RemoveOldest(int waitMSecs)
{
  pthread_mutex_lock(&(m_accessLock));

  // Hmm - the last event we got from RemoveOldest was a
  // blocker, and DoneWithEvent not called.
  // Default to "false" as a return value
  if (m_blockerState == PROCESSING_BLOCKER)
  {
    m_blockerReturnVal = false;
    m_blockerState = RETURNED_BLOCKER;
    broadcast(&(m_blockerSync));
  }

  // Blocker is waiting - return it
  // And push the blocker pipeline forward
  if(m_blockerState == PENDING_BLOCKER)
  {
    m_blockerState = PROCESSING_BLOCKER;
    const NVEvent* ev = m_blocker;
    pthread_mutex_unlock(&(m_accessLock));

    return ev;
  }
  else if (m_nextInsertIndex == m_headIndex)
  {
    // We're empty - so what do we do?
    if (waitMSecs == 0)
    {
      goto no_event;
    }
    else
    {
      // wait for the specified time
      wait(&(m_consumerSync), &(m_accessLock), (unsigned)waitMSecs);
    }

    // check again after exiting cond waits, either we had a timeout
    if(m_blockerState == PENDING_BLOCKER)
    {
      m_blockerState = PROCESSING_BLOCKER;
      const NVEvent* ev = m_blocker;
      pthread_mutex_unlock(&(m_accessLock));

      return ev;
    }
    else if (m_nextInsertIndex == m_headIndex)
    {
      goto no_event;
    }
  }

  {
    // One way or another, we have an event...
    const NVEvent* ev = m_events + m_headIndex;
    m_headIndex = NVNextWrapped(m_headIndex);

    pthread_mutex_unlock(&(m_accessLock));
    return ev;
  }
	
no_event:
  pthread_mutex_unlock(&(m_accessLock));
  return NULL;
}

void NVEventQueue::DoneWithEvent(bool ret)
{
  // We only care about blockers for now.
  // All other events just NOP
  pthread_mutex_lock(&(m_accessLock));
  if (m_blockerState == PROCESSING_BLOCKER)
  {
    m_blockerReturnVal = ret;
    m_blockerState = RETURNED_BLOCKER;
    broadcast(&(m_blockerSync));
  }
  pthread_mutex_unlock(&(m_accessLock));
}
