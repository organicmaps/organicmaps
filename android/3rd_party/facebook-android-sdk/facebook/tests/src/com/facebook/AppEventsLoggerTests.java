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

import android.content.Intent;
import android.content.IntentFilter;
import android.support.v4.content.LocalBroadcastManager;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.util.HashMap;
import java.util.List;

public class AppEventsLoggerTests extends FacebookTestCase {
    public void testSimpleCall() throws InterruptedException {
        AppEventsLogger.setFlushBehavior(AppEventsLogger.FlushBehavior.EXPLICIT_ONLY);

        TestSession session1 = TestSession.createSessionWithSharedUser(getActivity(), null);
        TestSession session2 = TestSession.createSessionWithSharedUser(getActivity(), null, SECOND_TEST_USER_TAG);

        AppEventsLogger logger1 = AppEventsLogger.newLogger(getActivity(), session1);
        AppEventsLogger logger2 = AppEventsLogger.newLogger(getActivity(), session2);

        final WaitForBroadcastReceiver waitForBroadcastReceiver = new WaitForBroadcastReceiver();
        waitForBroadcastReceiver.incrementExpectCount();

        final LocalBroadcastManager broadcastManager = LocalBroadcastManager.getInstance(getActivity());

        // Need to get notifications on another thread so we can wait for them.
        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                broadcastManager.registerReceiver(waitForBroadcastReceiver,
                        new IntentFilter(AppEventsLogger.ACTION_APP_EVENTS_FLUSHED));
            }
        }, true);

        logger1.logEvent("an_event");
        logger2.logEvent("another_event");

        logger1.flush();

        waitForBroadcastReceiver.waitForExpectedCalls();

        closeBlockerAndAssertSuccess();

        broadcastManager.unregisterReceiver(waitForBroadcastReceiver);
    }

    public void testPersistedEvents() throws IOException, ClassNotFoundException {
        AppEventsLogger.setFlushBehavior(AppEventsLogger.FlushBehavior.EXPLICIT_ONLY);

        final WaitForBroadcastReceiver waitForBroadcastReceiver = new WaitForBroadcastReceiver();
        final LocalBroadcastManager broadcastManager = LocalBroadcastManager.getInstance(getActivity());

        // Need to get notifications on another thread so we can wait for them.
        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                broadcastManager.registerReceiver(waitForBroadcastReceiver,
                        new IntentFilter(AppEventsLogger.ACTION_APP_EVENTS_FLUSHED));
            }
        }, true);

        getActivity().getFileStreamPath(AppEventsLogger.PersistedEvents.PERSISTED_EVENTS_FILENAME).delete();

        TestSession session1 = TestSession.createSessionWithSharedUser(getActivity(), null);
        AppEventsLogger logger1 = AppEventsLogger.newLogger(getActivity(), session1);

        logger1.logEvent("an_event");

        AppEventsLogger.onContextStop();

        FileInputStream fis = getActivity().openFileInput(AppEventsLogger.PersistedEvents.PERSISTED_EVENTS_FILENAME);
        assertNotNull(fis);

        ObjectInputStream ois = new ObjectInputStream(fis);
        Object obj = ois.readObject();
        ois.close();

        assertTrue(obj instanceof HashMap);

        logger1.flush();

        logger1.logEvent("another_event");

        waitForBroadcastReceiver.incrementExpectCount();
        logger1.flush();

        waitForBroadcastReceiver.waitForExpectedCalls();
        List<Intent> receivedIntents = waitForBroadcastReceiver.getReceivedIntents();
        assertEquals(1, receivedIntents.size());

        Intent intent = receivedIntents.get(0);
        assertNotNull(intent);

        assertEquals(2, intent.getIntExtra(AppEventsLogger.APP_EVENTS_EXTRA_NUM_EVENTS_FLUSHED, 0));
        broadcastManager.unregisterReceiver(waitForBroadcastReceiver);
    }

    @SuppressWarnings("deprecation")
    public void testInsightsLoggerCompatibility() throws InterruptedException {
        AppEventsLogger.setFlushBehavior(AppEventsLogger.FlushBehavior.AUTO);

        TestSession session1 = TestSession.createSessionWithSharedUser(getActivity(), null);

        InsightsLogger logger1 = InsightsLogger.newLogger(getActivity(), null, null, session1);

        final WaitForBroadcastReceiver waitForBroadcastReceiver = new WaitForBroadcastReceiver();
        waitForBroadcastReceiver.incrementExpectCount();

        final LocalBroadcastManager broadcastManager = LocalBroadcastManager.getInstance(getActivity());

        // Need to get notifications on another thread so we can wait for them.
        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                broadcastManager.registerReceiver(waitForBroadcastReceiver,
                        new IntentFilter(AppEventsLogger.ACTION_APP_EVENTS_FLUSHED));
            }
        }, true);

        logger1.logConversionPixel("foo", 1.0);

        // For some reason the flush can take an extraordinary amount of time, so increasing
        // the timeout here to prevent failures.
        waitForBroadcastReceiver.waitForExpectedCalls(600*1000);

        closeBlockerAndAssertSuccess();

        broadcastManager.unregisterReceiver(waitForBroadcastReceiver);
    }
}
