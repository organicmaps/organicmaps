/**
 * Copyright 2010-present Facebook.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.facebook.internal;

import com.facebook.Settings;

import java.util.concurrent.Executor;

class WorkQueue {
    public static final int DEFAULT_MAX_CONCURRENT = 8;

    private final Object workLock = new Object();
    private WorkNode pendingJobs;

    private final int maxConcurrent;
    private final Executor executor;

    private WorkNode runningJobs = null;
    private int runningCount = 0;

    WorkQueue() {
        this(DEFAULT_MAX_CONCURRENT);
    }

    WorkQueue(int maxConcurrent) {
        this(maxConcurrent, Settings.getExecutor());
    }

    WorkQueue(int maxConcurrent, Executor executor) {
        this.maxConcurrent = maxConcurrent;
        this.executor = executor;
    }

    WorkItem addActiveWorkItem(Runnable callback) {
        return addActiveWorkItem(callback, true);
    }

    WorkItem addActiveWorkItem(Runnable callback, boolean addToFront) {
        WorkNode node = new WorkNode(callback);
        synchronized (workLock) {
            pendingJobs = node.addToList(pendingJobs, addToFront);
        }

        startItem();
        return node;
    }

    void validate() {
        synchronized (workLock) {
            // Verify that all running items know they are running, and counts match
            int count = 0;

            if (runningJobs != null) {
                WorkNode walk = runningJobs;
                do {
                    walk.verify(true);
                    count++;
                    walk = walk.getNext();
                } while (walk != runningJobs);
            }

            assert runningCount == count;
        }
    }

    private void startItem() {
        finishItemAndStartNew(null);
    }

    private void finishItemAndStartNew(WorkNode finished) {
        WorkNode ready = null;

        synchronized (workLock) {
            if (finished != null) {
                runningJobs = finished.removeFromList(runningJobs);
                runningCount--;
            }

            if (runningCount < maxConcurrent) {
                ready = pendingJobs; // Head of the pendingJobs queue
                if (ready != null) {
                    // The Queue reassignments are necessary since 'ready' might have been
                    // added / removed from the front of either queue, which changes its
                    // respective head.
                    pendingJobs = ready.removeFromList(pendingJobs);
                    runningJobs = ready.addToList(runningJobs, false);
                    runningCount++;

                    ready.setIsRunning(true);
                }
            }
        }

        if (ready != null) {
            execute(ready);
        }
    }

    private void execute(final WorkNode node) {
        executor.execute(new Runnable() {
            @Override
            public void run() {
                try {
                    node.getCallback().run();
                } finally {
                    finishItemAndStartNew(node);
                }
            }
        });
    }

    private class WorkNode implements WorkItem {
        private final Runnable callback;
        private WorkNode next;
        private WorkNode prev;
        private boolean isRunning;

        WorkNode(Runnable callback) {
            this.callback = callback;
        }

        @Override
        public boolean cancel() {
            synchronized (workLock) {
                if (!isRunning()) {
                    pendingJobs = removeFromList(pendingJobs);
                    return true;
                }
            }

            return false;
        }

        @Override
        public void moveToFront() {
            synchronized (workLock) {
                if (!isRunning()) {
                    pendingJobs = removeFromList(pendingJobs);
                    pendingJobs = addToList(pendingJobs, true);
                }
            }
        }

        @Override
        public boolean isRunning() {
            return isRunning;
        }

        Runnable getCallback() {
            return callback;
        }

        WorkNode getNext() {
            return next;
        }

        void setIsRunning(boolean isRunning) {
            this.isRunning = isRunning;
        }

        WorkNode addToList(WorkNode list, boolean addToFront) {
            assert next == null;
            assert prev == null;

            if (list == null) {
                list = next = prev = this;
            } else {
                next = list;
                prev = list.prev;
                next.prev = prev.next = this;
            }

            return addToFront ? this : list;
        }

        WorkNode removeFromList(WorkNode list) {
            assert next != null;
            assert prev != null;

            if (list == this) {
                if (next == this) {
                    list = null;
                } else {
                    list = next;
                }
            }

            next.prev = prev;
            prev.next = next;
            next = prev = null;

            return list;
        }

        void verify(boolean shouldBeRunning) {
            assert prev.next == this;
            assert next.prev == this;
            assert isRunning() == shouldBeRunning;
        }
    }

    interface WorkItem {
        boolean cancel();
        boolean isRunning();
        void moveToFront();
    }
}
