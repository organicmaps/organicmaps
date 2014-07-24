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

package com.facebook;

import android.os.Handler;
import android.os.HandlerThread;

public class TestBlocker extends HandlerThread {
    private Exception exception;
    public int signals;
    private volatile Handler handler;

    private TestBlocker() {
        super("TestBlocker");
    }

    public synchronized static TestBlocker createTestBlocker() {
        TestBlocker blocker = new TestBlocker();
        blocker.start();

        // Wait until we have a Looper and Handler.
        synchronized (blocker) {
            while (blocker.handler == null) {
                try {
                    blocker.wait();
                } catch (InterruptedException e) {
                }
            }
        }

        return blocker;
    }

    @Override
    public void run() {
        try {
            super.run();
        } catch (Exception e) {
            setException(e);
        }
        synchronized (this) {
            notifyAll();
        }
    }

    public Handler getHandler() {
        return handler;
    }

    public void assertSuccess() throws Exception {
        Exception e = getException();
        if (e != null) {
            throw e;
        }
    }

    public synchronized void signal() {
        ++signals;
        notifyAll();
    }

    public void waitForSignals(int numSignals) throws Exception {
        // Make sure we aren't sitting on an unhandled exception before we even start, because that means our
        // thread isn't around anymore.
        assertSuccess();

        setException(null);

        synchronized (this) {
            while (getException() == null && signals < numSignals) {
                try {
                    wait();
                } catch (InterruptedException e) {
                }
            }
            signals = 0;
        }
    }

    public void waitForSignalsAndAssertSuccess(int numSignals) throws Exception {
        waitForSignals(numSignals);
        assertSuccess();
    }

    public synchronized Exception getException() {
        return exception;
    }

    public synchronized void setException(Exception e) {
        exception = e;
        notifyAll();
    }

    @Override
    protected void onLooperPrepared() {
        synchronized (this) {
            handler = new Handler(getLooper());
            notifyAll();
        }
    }
}
