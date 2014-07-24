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

import android.os.Bundle;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import org.json.JSONArray;

// Because TestSession is the component under test here, be careful in calling methods on FacebookTestCase that
// assume TestSession works correctly.
public class TestSessionTests extends FacebookTestCase {
    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanCreateWithPrivateUser() {
        TestSession session = TestSession.createSessionWithPrivateUser(getActivity(), null);
        assertTrue(session != null);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanCreateWithSharedUser() {
        TestSession session = TestSession.createSessionWithSharedUser(getActivity(), null);
        assertTrue(session != null);
    }

    @MediumTest
    @LargeTest
    public void testCanOpenWithSharedUser() throws Throwable {
        final TestBlocker blocker = getTestBlocker();
        TestSession session = getTestSessionWithSharedUser();

        Session.OpenRequest openRequest = new Session.OpenRequest(getActivity()).
                setCallback(
                        new Session.StatusCallback() {
                            @Override
                            public void call(Session session, SessionState state, Exception exception) {
                                assertTrue(exception == null);
                                blocker.signal();
                            }
                        });
        session.openForRead(openRequest);

        waitAndAssertSuccess(blocker, 1);

        assertTrue(session.getState().isOpened());
    }

    @MediumTest
    @LargeTest
    public void testSharedUserDoesntCreateUnnecessaryUsers() throws Throwable {
        TestSession session = getTestSessionWithSharedUser();
        openSession(getActivity(), session);

        // Note that this test is somewhat brittle in that the count of test users could change for
        // external reasons while the test is running. For that reason it may not be appropriate for an
        // automated test suite, and could be run only when testing changes to TestSession.
        int startingUserCount = countTestUsers();

        session = getTestSessionWithSharedUser();
        openSession(getActivity(), session);

        int endingUserCount = countTestUsers();

        assertEquals(startingUserCount, endingUserCount);
    }

    // This test is currently unreliable, I believe due to timing/replication issues that cause the
    // counts to occasionally be off. Taking out of test runs for now until a more robust test can be added.
    @LargeTest
    public void failing_testPrivateUserIsDeletedOnSessionClose() throws Throwable {
        final TestBlocker blocker = getTestBlocker();

        // See comment above regarding test user count.
        int startingUserCount = countTestUsers();

        TestSession session = getTestSessionWithPrivateUser(blocker);
        openSession(getActivity(), session);

        int sessionOpenUserCount = countTestUsers();

        assertEquals(startingUserCount + 1, sessionOpenUserCount);

        session.close();

        int endingUserCount = countTestUsers();

        assertEquals(startingUserCount, endingUserCount);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCannotChangeTestApplicationIdOnceSet() {
        try {
            TestSession.setTestApplicationId("hello");
            TestSession.setTestApplicationId("world");
            fail("expected exception");
        } catch (FacebookException e) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCannotChangeTestApplicationSecretOnceSet() {
        try {
            TestSession.setTestApplicationSecret("hello");
            TestSession.setTestApplicationSecret("world");
            fail("expected exception");
        } catch (FacebookException e) {
        }
    }

    private int countTestUsers() {
        TestSession session = getTestSessionWithSharedUser(null);

        String appAccessToken = TestSession.getAppAccessToken();
        assertNotNull(appAccessToken);
        String applicationId = session.getApplicationId();
        assertNotNull(applicationId);

        String fqlQuery = String.format("SELECT id FROM test_account WHERE app_id = %s", applicationId);
        Bundle parameters = new Bundle();
        parameters.putString("q", fqlQuery);
        parameters.putString("access_token", appAccessToken);

        Request request = new Request(null, "fql", parameters, null);
        Response response = request.executeAndWait();

        JSONArray data = (JSONArray) response.getGraphObject().getProperty("data");
        return data.length();
    }
}
