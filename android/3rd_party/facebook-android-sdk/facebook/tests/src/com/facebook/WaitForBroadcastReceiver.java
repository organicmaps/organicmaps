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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.ConditionVariable;
import android.os.Looper;
import junit.framework.Assert;

import java.util.ArrayList;
import java.util.List;

class WaitForBroadcastReceiver extends BroadcastReceiver {
    static int idGenerator = 0;
    final int id = idGenerator++;

    ConditionVariable condition = new ConditionVariable(true);
    int expectCount;
    int actualCount;
    List<Intent> receivedIntents = new ArrayList<Intent>();

    public void incrementExpectCount() {
        incrementExpectCount(1);
    }

    public void incrementExpectCount(int n) {
        expectCount += n;
        if (actualCount < expectCount) {
            condition.close();
        }
    }

    public void waitForExpectedCalls() {
        this.waitForExpectedCalls(SessionTestsBase.DEFAULT_TIMEOUT_MILLISECONDS);
    }

    public void waitForExpectedCalls(long timeoutMillis) {
        if (!condition.block(timeoutMillis)) {
            Assert.assertTrue(false);
        }
    }

    public List<Intent> getReceivedIntents() {
        return receivedIntents;
    }

    public static void incrementExpectCounts(WaitForBroadcastReceiver... receivers) {
        for (WaitForBroadcastReceiver receiver : receivers) {
            receiver.incrementExpectCount();
        }
    }

    public static void waitForExpectedCalls(WaitForBroadcastReceiver... receivers) {
        for (WaitForBroadcastReceiver receiver : receivers) {
            receiver.waitForExpectedCalls();
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (++actualCount == expectCount) {
            condition.open();
        }
        receivedIntents.add(intent);
        Assert.assertTrue("expecting " + expectCount + "broadcasts, but received " + actualCount,                actualCount <= expectCount);
        Assert.assertEquals("BroadcastReceiver should receive on main UI thread",
                Thread.currentThread(), Looper.getMainLooper().getThread());
    }
}
