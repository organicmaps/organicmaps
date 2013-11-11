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

import android.app.Activity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphObjectList;
import com.facebook.internal.Logger;
import com.facebook.internal.Utility;
import com.facebook.internal.Validate;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.*;

/**
 * Implements an subclass of Session that knows about test users for a particular
 * application. This should never be used from a real application, but may be useful
 * for writing unit tests, etc.
 * <p/>
 * Facebook allows developers to create test accounts for testing their applications'
 * Facebook integration (see https://developers.facebook.com/docs/test_users/). This class
 * simplifies use of these accounts for writing unit tests. It is not designed for use in
 * production application code.
 * <p/>
 * The main use case for this class is using {@link #createSessionWithPrivateUser(android.app.Activity, java.util.List)}
 * or {@link #createSessionWithSharedUser(android.app.Activity, java.util.List)}
 * to create a session for a test user. Two modes are supported. In "shared" mode, an attempt
 * is made to find an existing test user that has the required permissions. If no such user is available,
 * a new one is created with the required permissions. In "private" mode, designed for
 * scenarios which require a new user in a known clean state, a new test user will always be
 * created, and it will be automatically deleted when the TestSession is closed. The session
 * obeys the same lifecycle as a regular Session, meaning it must be opened after creation before
 * it can be used to make calls to the Facebook API.
 * <p/>
 * Prior to creating a TestSession, two static methods must be called to initialize the
 * application ID and application Secret to be used for managing test users. These methods are
 * {@link #setTestApplicationId(String)} and {@link #setTestApplicationSecret(String)}.
 * <p/>
 * Note that the shared test user functionality depends on a naming convention for the test users.
 * It is important that any testing of functionality which will mutate the permissions for a
 * test user NOT use a shared test user, or this scheme will break down. If a shared test user
 * seems to be in an invalid state, it can be deleted manually via the Web interface at
 * https://developers.facebook.com/apps/APP_ID/permissions?role=test+users.
 */
public class TestSession extends Session {
    private static final long serialVersionUID = 1L;

    private enum Mode {
        PRIVATE, SHARED
    }

    private static final String LOG_TAG = Logger.LOG_TAG_BASE + "TestSession";

    private static Map<String, TestAccount> appTestAccounts;
    private static String testApplicationSecret;
    private static String testApplicationId;

    private final String sessionUniqueUserTag;
    private final List<String> requestedPermissions;
    private final Mode mode;
    private String testAccountId;

    private boolean wasAskedToExtendAccessToken;

    TestSession(Activity activity, List<String> permissions, TokenCachingStrategy tokenCachingStrategy,
            String sessionUniqueUserTag, Mode mode) {
        super(activity, TestSession.testApplicationId, tokenCachingStrategy);

        Validate.notNull(permissions, "permissions");

        // Validate these as if they were arguments even though they are statics.
        Validate.notNullOrEmpty(testApplicationId, "testApplicationId");
        Validate.notNullOrEmpty(testApplicationSecret, "testApplicationSecret");

        this.sessionUniqueUserTag = sessionUniqueUserTag;
        this.mode = mode;
        this.requestedPermissions = permissions;
    }

    /**
     * Constructs a TestSession which creates a test user on open, and destroys the user on
     * close; This method should not be used in application code -- but is useful for creating unit tests
     * that use the Facebook SDK.
     *
     * @param activity    the Activity to use for opening the session
     * @param permissions list of strings containing permissions to request; nil will result in
     *                    a common set of permissions (email, publish_actions) being requested
     * @return a new TestSession that is in the CREATED state, ready to be opened
     */
    public static TestSession createSessionWithPrivateUser(Activity activity, List<String> permissions) {
        return createTestSession(activity, permissions, Mode.PRIVATE, null);
    }

    /**
     * Constructs a TestSession which uses a shared test user with the right permissions,
     * creating one if necessary on open (but not deleting it on close, so it can be re-used in later
     * tests).
     * <p/>
     * This method should not be used in application code -- but is useful for creating unit tests
     * that use the Facebook SDK.
     *
     * @param activity    the Activity to use for opening the session
     * @param permissions list of strings containing permissions to request; nil will result in
     *                    a common set of permissions (email, publish_actions) being requested
     * @return a new TestSession that is in the CREATED state, ready to be opened
     */
    public static TestSession createSessionWithSharedUser(Activity activity, List<String> permissions) {
        return createSessionWithSharedUser(activity, permissions, null);
    }

    /**
     * Constructs a TestSession which uses a shared test user with the right permissions,
     * creating one if necessary on open (but not deleting it on close, so it can be re-used in later
     * tests).
     * <p/>
     * This method should not be used in application code -- but is useful for creating unit tests
     * that use the Facebook SDK.
     *
     * @param activity             the Activity to use for opening the session
     * @param permissions          list of strings containing permissions to request; nil will result in
     *                             a common set of permissions (email, publish_actions) being requested
     * @param sessionUniqueUserTag a string which will be used to make this user unique among other
     *                             users with the same permissions. Useful for tests which require two or more users to interact
     *                             with each other, and which therefore must have sessions associated with different users.
     * @return a new TestSession that is in the CREATED state, ready to be opened
     */
    public static TestSession createSessionWithSharedUser(Activity activity, List<String> permissions,
            String sessionUniqueUserTag) {
        return createTestSession(activity, permissions, Mode.SHARED, sessionUniqueUserTag);
    }

    /**
     * Gets the Facebook Application ID for the application under test.
     *
     * @return the application ID
     */
    public static synchronized String getTestApplicationId() {
        return testApplicationId;
    }

    /**
     * Sets the Facebook Application ID for the application under test. This must be specified
     * prior to creating a TestSession.
     *
     * @param applicationId the application ID
     */
    public static synchronized void setTestApplicationId(String applicationId) {
        if (testApplicationId != null && !testApplicationId.equals(applicationId)) {
            throw new FacebookException("Can't have more than one test application ID");
        }
        testApplicationId = applicationId;
    }

    /**
     * Gets the Facebook Application Secret for the application under test.
     *
     * @return the application secret
     */
    public static synchronized String getTestApplicationSecret() {
        return testApplicationSecret;
    }

    /**
     * Sets the Facebook Application Secret for the application under test. This must be specified
     * prior to creating a TestSession.
     *
     * @param applicationSecret the application secret
     */
    public static synchronized void setTestApplicationSecret(String applicationSecret) {
        if (testApplicationSecret != null && !testApplicationSecret.equals(applicationSecret)) {
            throw new FacebookException("Can't have more than one test application secret");
        }
        testApplicationSecret = applicationSecret;
    }

    /**
     * Gets the ID of the test user that this TestSession is authenticated as.
     *
     * @return the Facebook user ID of the test user
     */
    public final String getTestUserId() {
        return testAccountId;
    }

    private static synchronized TestSession createTestSession(Activity activity, List<String> permissions, Mode mode,
            String sessionUniqueUserTag) {
        if (Utility.isNullOrEmpty(testApplicationId) || Utility.isNullOrEmpty(testApplicationSecret)) {
            throw new FacebookException("Must provide app ID and secret");
        }

        if (Utility.isNullOrEmpty(permissions)) {
            permissions = Arrays.asList("email", "publish_actions");
        }

        return new TestSession(activity, permissions, new TestTokenCachingStrategy(), sessionUniqueUserTag,
                mode);
    }

    private static synchronized void retrieveTestAccountsForAppIfNeeded() {
        if (appTestAccounts != null) {
            return;
        }

        appTestAccounts = new HashMap<String, TestAccount>();

        // The data we need is split across two different FQL tables. We construct two queries, submit them
        // together (the second one refers to the first one), then cross-reference the results.

        // Get the test accounts for this app.
        String testAccountQuery = String.format("SELECT id,access_token FROM test_account WHERE app_id = %s",
                testApplicationId);
        // Get the user names for those accounts.
        String userQuery = "SELECT uid,name FROM user WHERE uid IN (SELECT id FROM #test_accounts)";

        Bundle parameters = new Bundle();

        // Build a JSON string that contains our queries and pass it as the 'q' parameter of the query.
        JSONObject multiquery;
        try {
            multiquery = new JSONObject();
            multiquery.put("test_accounts", testAccountQuery);
            multiquery.put("users", userQuery);
        } catch (JSONException exception) {
            throw new FacebookException(exception);
        }
        parameters.putString("q", multiquery.toString());

        // We need to authenticate as this app.
        parameters.putString("access_token", getAppAccessToken());

        Request request = new Request(null, "fql", parameters, null);
        Response response = request.executeAndWait();

        if (response.getError() != null) {
            throw response.getError().getException();
        }

        FqlResponse fqlResponse = response.getGraphObjectAs(FqlResponse.class);

        GraphObjectList<FqlResult> fqlResults = fqlResponse.getData();
        if (fqlResults == null || fqlResults.size() != 2) {
            throw new FacebookException("Unexpected number of results from FQL query");
        }

        // We get back two sets of results. The first is from the test_accounts query, the second from the users query.
        Collection<TestAccount> testAccounts = fqlResults.get(0).getFqlResultSet().castToListOf(TestAccount.class);
        Collection<UserAccount> userAccounts = fqlResults.get(1).getFqlResultSet().castToListOf(UserAccount.class);

        // Use both sets of results to populate our static array of accounts.
        populateTestAccounts(testAccounts, userAccounts);

        return;
    }

    private static synchronized void populateTestAccounts(Collection<TestAccount> testAccounts,
            Collection<UserAccount> userAccounts) {
        // We get different sets of data from each of these queries. We want to combine them into a single data
        // structure. We have added a Name property to the TestAccount interface, even though we don't really get
        // a name back from the service from that query. We stick the Name from the corresponding UserAccount in it.
        for (TestAccount testAccount : testAccounts) {
            storeTestAccount(testAccount);
        }

        for (UserAccount userAccount : userAccounts) {
            TestAccount testAccount = appTestAccounts.get(userAccount.getUid());
            if (testAccount != null) {
                testAccount.setName(userAccount.getName());
            }
        }
    }

    private static synchronized void storeTestAccount(TestAccount testAccount) {
        appTestAccounts.put(testAccount.getId(), testAccount);
    }

    private static synchronized TestAccount findTestAccountMatchingIdentifier(String identifier) {
        retrieveTestAccountsForAppIfNeeded();

        for (TestAccount testAccount : appTestAccounts.values()) {
            if (testAccount.getName().contains(identifier)) {
                return testAccount;
            }
        }
        return null;
    }

    @Override
    public final String toString() {
        String superString = super.toString();

        return new StringBuilder().append("{TestSession").append(" testUserId:").append(testAccountId)
                .append(" ").append(superString).append("}").toString();
    }

    @Override
    void authorize(AuthorizationRequest request) {
        if (mode == Mode.PRIVATE) {
            createTestAccountAndFinishAuth();
        } else {
            findOrCreateSharedTestAccount();
        }
    }

    @Override
    void postStateChange(final SessionState oldState, final SessionState newState, final Exception error) {
        // Make sure this doesn't get overwritten.
        String id = testAccountId;

        super.postStateChange(oldState, newState, error);

        if (newState.isClosed() && id != null && mode == Mode.PRIVATE) {
            deleteTestAccount(id, getAppAccessToken());
        }
    }

    boolean getWasAskedToExtendAccessToken() {
        return wasAskedToExtendAccessToken;
    }

    void forceExtendAccessToken(boolean forceExtendAccessToken) {
        AccessToken currentToken = getTokenInfo();
        setTokenInfo(
                new AccessToken(currentToken.getToken(), new Date(), currentToken.getPermissions(),
                        AccessTokenSource.TEST_USER, new Date(0)));
        setLastAttemptedTokenExtendDate(new Date(0));
    }

    @Override
    boolean shouldExtendAccessToken() {
        boolean result = super.shouldExtendAccessToken();
        wasAskedToExtendAccessToken = false;
        return result;
    }

    @Override
    void extendAccessToken() {
        wasAskedToExtendAccessToken = true;
        super.extendAccessToken();
    }

    void fakeTokenRefreshAttempt() {
        setCurrentTokenRefreshRequest(new TokenRefreshRequest());
    }

    static final String getAppAccessToken() {
        return testApplicationId + "|" + testApplicationSecret;
    }

    private void findOrCreateSharedTestAccount() {
        TestAccount testAccount = findTestAccountMatchingIdentifier(getSharedTestAccountIdentifier());
        if (testAccount != null) {
            finishAuthWithTestAccount(testAccount);
        } else {
            createTestAccountAndFinishAuth();
        }
    }

    private void finishAuthWithTestAccount(TestAccount testAccount) {
        testAccountId = testAccount.getId();

        AccessToken accessToken = AccessToken.createFromString(testAccount.getAccessToken(), requestedPermissions,
                AccessTokenSource.TEST_USER);
        finishAuthOrReauth(accessToken, null);
    }

    private TestAccount createTestAccountAndFinishAuth() {
        Bundle parameters = new Bundle();
        parameters.putString("installed", "true");
        parameters.putString("permissions", getPermissionsString());
        parameters.putString("access_token", getAppAccessToken());

        // If we're in shared mode, we want to rename this user to encode its permissions, so we can find it later
        // in another shared session. If we're in private mode, don't bother renaming it since we're just going to
        // delete it at the end of the session.
        if (mode == Mode.SHARED) {
            parameters.putString("name", String.format("Shared %s Testuser", getSharedTestAccountIdentifier()));
        }

        String graphPath = String.format("%s/accounts/test-users", testApplicationId);
        Request createUserRequest = new Request(null, graphPath, parameters, HttpMethod.POST);
        Response response = createUserRequest.executeAndWait();

        FacebookRequestError error = response.getError();
        TestAccount testAccount = response.getGraphObjectAs(TestAccount.class);
        if (error != null) {
            finishAuthOrReauth(null, error.getException());
            return null;
        } else {
            assert testAccount != null;

            // If we are in shared mode, store this new account in the dictionary so we can re-use it later.
            if (mode == Mode.SHARED) {
                // Remember the new name we gave it, since we didn't get it back in the results of the create request.
                testAccount.setName(parameters.getString("name"));
                storeTestAccount(testAccount);
            }

            finishAuthWithTestAccount(testAccount);

            return testAccount;
        }
    }

    private void deleteTestAccount(String testAccountId, String appAccessToken) {
        Bundle parameters = new Bundle();
        parameters.putString("access_token", appAccessToken);

        Request request = new Request(null, testAccountId, parameters, HttpMethod.DELETE);
        Response response = request.executeAndWait();

        FacebookRequestError error = response.getError();
        GraphObject graphObject = response.getGraphObject();
        if (error != null) {
            Log.w(LOG_TAG, String.format("Could not delete test account %s: %s", testAccountId, error.getException().toString()));
        } else if (graphObject.getProperty(Response.NON_JSON_RESPONSE_PROPERTY) == (Boolean) false) {
            Log.w(LOG_TAG, String.format("Could not delete test account %s: unknown reason", testAccountId));
        }
    }

    private String getPermissionsString() {
        return TextUtils.join(",", requestedPermissions);
    }

    private String getSharedTestAccountIdentifier() {
        // We use long even though hashes are ints to avoid sign issues.
        long permissionsHash = getPermissionsString().hashCode() & 0xffffffffL;
        long sessionTagHash = (sessionUniqueUserTag != null) ? sessionUniqueUserTag.hashCode() & 0xffffffffL : 0;

        long combinedHash = permissionsHash ^ sessionTagHash;
        return validNameStringFromInteger(combinedHash);
    }

    private String validNameStringFromInteger(long i) {
        String s = Long.toString(i);
        StringBuilder result = new StringBuilder("Perm");

        // We know each character is a digit. Convert it into a letter 'a'-'j'. Avoid repeated characters
        //  that might make Facebook reject the name by converting every other repeated character into one
        //  10 higher ('k'-'t').
        char lastChar = 0;
        for (char c : s.toCharArray()) {
            if (c == lastChar) {
                c += 10;
            }
            result.append((char) (c + 'a' - '0'));
            lastChar = c;
        }

        return result.toString();
    }

    private interface TestAccount extends GraphObject {
        String getId();

        String getAccessToken();

        // Note: We don't actually get Name from our FQL query. We fill it in by correlating with UserAccounts.
        String getName();

        void setName(String name);
    }

    private interface UserAccount extends GraphObject {
        String getUid();

        String getName();

        void setName(String name);
    }

    private interface FqlResult extends GraphObject {
        GraphObjectList<GraphObject> getFqlResultSet();

    }

    private interface FqlResponse extends GraphObject {
        GraphObjectList<FqlResult> getData();
    }

    private static final class TestTokenCachingStrategy extends TokenCachingStrategy {
        private Bundle bundle;

        @Override
        public Bundle load() {
            return bundle;
        }

        @Override
        public void save(Bundle value) {
            bundle = value;
        }

        @Override
        public void clear() {
            bundle = null;
        }
    }
}
