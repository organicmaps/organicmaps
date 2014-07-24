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

import android.net.Uri;
import android.os.Handler;
import android.test.AndroidTestCase;
import bolts.AppLink;
import bolts.Continuation;
import bolts.Task;

import java.util.ArrayList;
import java.util.List;

public class FacebookAppLinkResolverTests extends FacebookTestCase {
    private Task resolveTask;

    public void testSingleUrl() {
        String testUrlString = "https://fb.me/732873156764191";
        Uri testUrl = Uri.parse(testUrlString);
        Uri testWebUri = Uri.parse("http://www.facebook.com/");
        ArrayList<AppLink.Target> testTargets = new ArrayList<AppLink.Target>();
        testTargets.add(new AppLink.Target(
                "com.myapp",
                null,
                Uri.parse("myapp://3"),
                "my app"));
        testTargets.add(new AppLink.Target(
                "com.myapp-test",
                null,
                Uri.parse("myapp-test://4"),
                "my test app"));
        try {
            executeResolverOnBlockerThread(new FacebookAppLinkResolver(), testUrl);

            getTestBlocker().waitForSignals(1);

            assertNotNull(resolveTask);

            Task<AppLink> singleUrlResolveTask = (Task<AppLink>)resolveTask;

            assertTrue(singleUrlResolveTask.isCompleted() &&
                    !singleUrlResolveTask.isCancelled() &&
                    !singleUrlResolveTask.isFaulted());

            AppLink appLink = singleUrlResolveTask.getResult();

            assertEquals(appLink.getSourceUrl(), testUrl);
            assertEquals(appLink.getWebUrl(), testWebUri);
            assertTrue(targetListsAreEqual(appLink.getTargets(), testTargets));
        } catch (Exception e) {
            // Forcing the test to fail with details
            assertNull(e);
        }
    }

    public void testUrlWithNoAppLinkData() {
        String testNoAppLinkUrlString = "https://fb.me/732873156764191_no_app_link";
        Uri testNoAppLinkUrl = Uri.parse(testNoAppLinkUrlString);
        try {
            executeResolverOnBlockerThread(new FacebookAppLinkResolver(), testNoAppLinkUrl);

            getTestBlocker().waitForSignals(1);

            assertNotNull(resolveTask);

            Task<AppLink> singleUrlResolveTask = (Task<AppLink>)resolveTask;

            assertTrue(singleUrlResolveTask.isCompleted() &&
                    !singleUrlResolveTask.isCancelled() &&
                    !singleUrlResolveTask.isFaulted());

            AppLink appLink = singleUrlResolveTask.getResult();
            assertNull(appLink);
        } catch (Exception e) {
            // Forcing the test to fail with details
            assertNull(e);
        }
    }

    public void testCachedAppLinkData() {
        String testUrlString = "https://fb.me/732873156764191";
        Uri testUrl = Uri.parse(testUrlString);
        Uri testWebUri = Uri.parse("http://www.facebook.com/");
        ArrayList<AppLink.Target> testTargets = new ArrayList<AppLink.Target>();
        testTargets.add(new AppLink.Target(
                "com.myapp",
                null,
                Uri.parse("myapp://3"),
                "my app"));
        testTargets.add(new AppLink.Target(
                "com.myapp-test",
                null,
                Uri.parse("myapp-test://4"),
                "my test app"));

        try {
            FacebookAppLinkResolver resolver = new FacebookAppLinkResolver();

            // This will prefetch the app link
            executeResolverOnBlockerThread(resolver, testUrl);
            getTestBlocker().waitForSignals(1);
            assertNotNull(resolveTask);

            // Now let's fetch it again. This should complete the task synchronously.
            Task<AppLink> cachedUrlResolveTask = resolver.getAppLinkFromUrlInBackground(testUrl);

            assertTrue(cachedUrlResolveTask.isCompleted() &&
                    !cachedUrlResolveTask.isCancelled() &&
                    !cachedUrlResolveTask.isFaulted());

            AppLink appLink = cachedUrlResolveTask.getResult();

            assertEquals(appLink.getSourceUrl(), testUrl);
            assertEquals(appLink.getWebUrl(), testWebUri);
            assertTrue(targetListsAreEqual(appLink.getTargets(), testTargets));
        } catch (Exception e) {
            // Forcing the test to fail with details
            assertNull(e);
        }
    }

    public void executeResolverOnBlockerThread(final FacebookAppLinkResolver resolver, final Uri testUrl) {
        final TestBlocker blocker = getTestBlocker();
        Runnable runnable = new Runnable() {
            public void run() {
                try {
                    resolveTask = resolver.getAppLinkFromUrlInBackground(testUrl);
                    resolveTask.continueWith(new Continuation() {
                        @Override
                        public Object then(Task task) throws Exception {
                            // Once the task is complete, unblock the test thread, so it can inspect for errors/results.
                            blocker.signal();
                            return null;
                        }
                    });
                } catch (Exception e) {
                    // Get back to the test case if there was an uncaught exception
                    blocker.signal();
                }
            }
        };

        Handler handler = new Handler(blocker.getLooper());
        handler.post(runnable);
    }

    private static boolean targetListsAreEqual(List<AppLink.Target> list1, List<AppLink.Target> list2) {
        if (list1 == null) {
            return list2 == null;
        } else if (list2 == null || list1.size() != list2.size()) {
            return false;
        }

        ArrayList<AppLink.Target> list2Copy = new ArrayList<AppLink.Target>(list2);

        for(int i = 0; i < list1.size(); i++) {
            int j;
            for (j = 0; j < list2Copy.size(); j++) {
                if (targetsAreEqual(list1.get(i), list2Copy.get(j))) {
                    break;
                }
            }

            if (j < list2Copy.size()) {
                // Found a match. Remove from the copy to make sure the same target isn't matched twice.
                list2Copy.remove(j);
            } else {
                // Match not found
                return false;
            }
        }
        return true;
    }

    private static boolean targetsAreEqual(AppLink.Target target1, AppLink.Target target2) {
        boolean isEqual =
                objectsAreEqual(target1.getPackageName(), target2.getPackageName()) &&
                objectsAreEqual(target1.getClassName(), target2.getClassName()) &&
                objectsAreEqual(target1.getAppName(), target2.getAppName()) &&
                objectsAreEqual(target1.getUrl(), target2.getUrl()) ;

        return isEqual;
    }

    private static boolean objectsAreEqual(Object s1, Object s2) {
        return s1 == null
                ? s2 == null
                : s1.equals(s2);
    }
}
