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

import com.facebook.sdk.tests.R;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class FacebookRequestErrorTests extends FacebookTestCase {
    public static final String ERROR_SINGLE_RESPONSE =
            "{\n" +
            "  \"error\": {\n" +
            "    \"message\": \"Unknown path components: /unknown\",\n" +
            "    \"type\": \"OAuthException\",\n" +
            "    \"code\": 2500\n" +
            "  }\n" +
            "}";

    public static final String ERROR_BATCH_RESPONSE =
            "[\n" +
            "  {\n" +
            "    \"headers\": [\n" +
            "      {\n" +
            "        \"value\": \"*\",\n" +
            "        \"name\": \"Access-Control-Allow-Origin\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"no-store\",\n" +
            "        \"name\": \"Cache-Control\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"close\",\n" +
            "        \"name\": \"Connection\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"text\\/javascript; charset=UTF-8\",\n" +
            "        \"name\": \"Content-Type\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"Sat, 01 Jan 2000 00:00:00 GMT\",\n" +
            "        \"name\": \"Expires\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"no-cache\",\n" +
            "        \"name\": \"Pragma\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"OAuth \\\"Facebook Platform\\\" \\\"invalid_request\\\" \\\"An active access token must be used to query information about the current user.\\\"\",\n" +
            "        \"name\": \"WWW-Authenticate\"\n" +
            "      }\n" +
            "    ],\n" +
            "    \"body\": \"{\\\"error\\\":{\\\"message\\\":\\\"An active access token must be used to query information about the current user.\\\",\\\"type\\\":\\\"OAuthException\\\",\\\"code\\\":2500}}\",\n" +
            "    \"code\": 400\n" +
            "  },\n" +
            "  {\n" +
            "    \"headers\": [\n" +
            "      {\n" +
            "        \"value\": \"*\",\n" +
            "        \"name\": \"Access-Control-Allow-Origin\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"no-store\",\n" +
            "        \"name\": \"Cache-Control\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"close\",\n" +
            "        \"name\": \"Connection\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"text\\/javascript; charset=UTF-8\",\n" +
            "        \"name\": \"Content-Type\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"Sat, 01 Jan 2000 00:00:00 GMT\",\n" +
            "        \"name\": \"Expires\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"no-cache\",\n" +
            "        \"name\": \"Pragma\"\n" +
            "      },\n" +
            "      {\n" +
            "        \"value\": \"OAuth \\\"Facebook Platform\\\" \\\"invalid_request\\\" \\\"An active access token must be used to query information about the current user.\\\"\",\n" +
            "        \"name\": \"WWW-Authenticate\"\n" +
            "      }\n" +
            "    ],\n" +
            "    \"body\": \"{\\\"error\\\":{\\\"message\\\":\\\"An active access token must be used to query information about the current user.\\\",\\\"type\\\":\\\"OAuthException\\\",\\\"code\\\":2500}}\",\n" +
            "    \"code\": 400\n" +
            "  }\n" +
            "]";


    public static final String ERROR_SINGLE_RESPONSE_THROTTLE =
            "{\n" +
            "  \"error\": {\n" +
            "    \"message\": \"Application request limit reached\",\n" +
            "    \"code\": 4\n" +
            "  }\n" +
            "}";

    public static final String ERROR_SINGLE_RESPONSE_SERVER =
            "{\n" +
            "  \"error\": {\n" +
            "    \"message\": \"Some Server Error\",\n" +
            "    \"code\": 2\n" +
            "  }\n" +
            "}";

    public static final String ERROR_SINGLE_RESPONSE_PERMISSION =
            "{\n" +
            "  \"error\": {\n" +
            "    \"type\": \"OAuthException\",\n" +
            "    \"message\": \"(#200) Requires extended permission: publish_actions\",\n" +
            "    \"code\": 200\n" +
            "  }\n" +
            "}";

    public static final String ERROR_SINGLE_RESPONSE_WEB_LOGIN =
            "{\n" +
            "  \"error\": {\n" +
            "    \"message\": \"User need to login\",\n" +
            "    \"type\": \"OAuthException\",\n" +
            "    \"code\": 102,\n" +
            "    \"error_subcode\": 459\n" +
            "  }\n" +
            "}";

    public static final String ERROR_SINGLE_RESPONSE_RELOGIN =
            "{\n" +
            "  \"error\": {\n" +
            "    \"message\": \"User need to relogin\",\n" +
            "    \"type\": \"OAuthException\",\n" +
            "    \"code\": 102\n" +
            "  }\n" +
            "}";

    public static final String ERROR_SINGLE_RESPONSE_RELOGIN_DELETED_APP =
            "{\n" +
            "  \"error\": {\n" +
            "    \"message\": \"User need to relogin\",\n" +
            "    \"type\": \"OAuthException\",\n" +
            "    \"code\": 190,\n" +
            "    \"error_subcode\": 458\n" +
            "  }\n" +
            "}";

    public void testClientException() {
        final String errorMsg = "some error happened";
        FacebookRequestError error = new FacebookRequestError(null, new FacebookException(errorMsg));
        assertEquals(errorMsg, error.getErrorMessage());
        assertEquals(FacebookRequestError.Category.CLIENT, error.getCategory());
        assertEquals(FacebookRequestError.INVALID_ERROR_CODE, error.getErrorCode());
        assertEquals(FacebookRequestError.INVALID_HTTP_STATUS_CODE, error.getRequestStatusCode());
        assertEquals(0, error.getUserActionMessageId());
    }

    public void testSingleRequestWithoutBody() throws JSONException {
        JSONObject withStatusCode = new JSONObject();
        withStatusCode.put("code", 400);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(withStatusCode, withStatusCode, null);
        assertNotNull(error);
        assertEquals(400, error.getRequestStatusCode());
        assertEquals(FacebookRequestError.Category.BAD_REQUEST, error.getCategory());
        assertEquals(0, error.getUserActionMessageId());
    }

    public void testSingleErrorWithBody() throws JSONException {
        JSONObject originalResponse = new JSONObject(ERROR_SINGLE_RESPONSE);
        JSONObject withStatusCodeAndBody = new JSONObject();
        withStatusCodeAndBody.put("code", 400);
        withStatusCodeAndBody.put("body", originalResponse);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(withStatusCodeAndBody, originalResponse, null);
        assertNotNull(error);
        assertEquals(400, error.getRequestStatusCode());
        assertEquals("Unknown path components: /unknown", error.getErrorMessage());
        assertEquals("OAuthException", error.getErrorType());
        assertEquals(2500, error.getErrorCode());
        assertTrue(error.getBatchRequestResult() instanceof JSONObject);
        assertEquals(FacebookRequestError.Category.BAD_REQUEST, error.getCategory());
        assertEquals(0, error.getUserActionMessageId());
    }

    public void testBatchRequest() throws JSONException {
        JSONArray batchResponse = new JSONArray(ERROR_BATCH_RESPONSE);
        assertEquals(2, batchResponse.length());
        JSONObject firstResponse = (JSONObject) batchResponse.get(0);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(firstResponse, batchResponse, null);
        assertNotNull(error);
        assertEquals(400, error.getRequestStatusCode());
        assertEquals("An active access token must be used to query information about the current user.",
                error.getErrorMessage());
        assertEquals("OAuthException", error.getErrorType());
        assertEquals(2500, error.getErrorCode());
        assertTrue(error.getBatchRequestResult() instanceof  JSONArray);
        assertEquals(FacebookRequestError.Category.BAD_REQUEST, error.getCategory());
        assertEquals(0, error.getUserActionMessageId());
    }

    public void testSingleThrottledError() throws JSONException {
        JSONObject originalResponse = new JSONObject(ERROR_SINGLE_RESPONSE_THROTTLE);
        JSONObject withStatusCodeAndBody = new JSONObject();
        withStatusCodeAndBody.put("code", 403);
        withStatusCodeAndBody.put("body", originalResponse);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(withStatusCodeAndBody, originalResponse, null);
        assertNotNull(error);
        assertEquals(403, error.getRequestStatusCode());
        assertEquals("Application request limit reached", error.getErrorMessage());
        assertNull(error.getErrorType());
        assertEquals(4, error.getErrorCode());
        assertTrue(error.getBatchRequestResult() instanceof JSONObject);
        assertEquals(FacebookRequestError.Category.THROTTLING, error.getCategory());
        assertEquals(0, error.getUserActionMessageId());
    }

    public void testSingleServerError() throws JSONException {
        JSONObject originalResponse = new JSONObject(ERROR_SINGLE_RESPONSE_SERVER);
        JSONObject withStatusCodeAndBody = new JSONObject();
        withStatusCodeAndBody.put("code", 500);
        withStatusCodeAndBody.put("body", originalResponse);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(withStatusCodeAndBody, originalResponse, null);
        assertNotNull(error);
        assertEquals(500, error.getRequestStatusCode());
        assertEquals("Some Server Error", error.getErrorMessage());
        assertNull(error.getErrorType());
        assertEquals(2, error.getErrorCode());
        assertTrue(error.getBatchRequestResult() instanceof JSONObject);
        assertEquals(FacebookRequestError.Category.SERVER, error.getCategory());
        assertEquals(0, error.getUserActionMessageId());
    }

    public void testSinglePermissionError() throws JSONException {
        JSONObject originalResponse = new JSONObject(ERROR_SINGLE_RESPONSE_PERMISSION);
        JSONObject withStatusCodeAndBody = new JSONObject();
        withStatusCodeAndBody.put("code", 400);
        withStatusCodeAndBody.put("body", originalResponse);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(withStatusCodeAndBody, originalResponse, null);
        assertNotNull(error);
        assertEquals(400, error.getRequestStatusCode());
        assertEquals("(#200) Requires extended permission: publish_actions", error.getErrorMessage());
        assertEquals("OAuthException", error.getErrorType());
        assertEquals(200, error.getErrorCode());
        assertEquals(FacebookRequestError.INVALID_ERROR_CODE, error.getSubErrorCode());
        assertTrue(error.getBatchRequestResult() instanceof JSONObject);
        assertEquals(FacebookRequestError.Category.PERMISSION, error.getCategory());
        assertEquals(R.string.com_facebook_requesterror_permissions, error.getUserActionMessageId());
    }

    public void testSingleWebLoginError() throws JSONException {
        JSONObject originalResponse = new JSONObject(ERROR_SINGLE_RESPONSE_WEB_LOGIN);
        JSONObject withStatusCodeAndBody = new JSONObject();
        withStatusCodeAndBody.put("code", 400);
        withStatusCodeAndBody.put("body", originalResponse);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(withStatusCodeAndBody, originalResponse, null);
        assertNotNull(error);
        assertEquals(400, error.getRequestStatusCode());
        assertEquals("User need to login", error.getErrorMessage());
        assertEquals("OAuthException", error.getErrorType());
        assertEquals(102, error.getErrorCode());
        assertEquals(459, error.getSubErrorCode());
        assertTrue(error.getBatchRequestResult() instanceof JSONObject);
        assertEquals(FacebookRequestError.Category.AUTHENTICATION_RETRY, error.getCategory());
        assertEquals(R.string.com_facebook_requesterror_web_login, error.getUserActionMessageId());
    }

    public void testSingleReloginError() throws JSONException {
        JSONObject originalResponse = new JSONObject(ERROR_SINGLE_RESPONSE_RELOGIN);
        JSONObject withStatusCodeAndBody = new JSONObject();
        withStatusCodeAndBody.put("code", 400);
        withStatusCodeAndBody.put("body", originalResponse);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(withStatusCodeAndBody, originalResponse, null);
        assertNotNull(error);
        assertEquals(400, error.getRequestStatusCode());
        assertEquals("User need to relogin", error.getErrorMessage());
        assertEquals("OAuthException", error.getErrorType());
        assertEquals(102, error.getErrorCode());
        assertEquals(FacebookRequestError.INVALID_ERROR_CODE, error.getSubErrorCode());
        assertTrue(error.getBatchRequestResult() instanceof JSONObject);
        assertEquals(FacebookRequestError.Category.AUTHENTICATION_REOPEN_SESSION, error.getCategory());
        assertEquals(R.string.com_facebook_requesterror_reconnect, error.getUserActionMessageId());
    }

    public void testSingleReloginDeletedAppError() throws JSONException {
        JSONObject originalResponse = new JSONObject(ERROR_SINGLE_RESPONSE_RELOGIN_DELETED_APP);
        JSONObject withStatusCodeAndBody = new JSONObject();
        withStatusCodeAndBody.put("code", 400);
        withStatusCodeAndBody.put("body", originalResponse);
        FacebookRequestError error =
                FacebookRequestError.checkResponseAndCreateError(withStatusCodeAndBody, originalResponse, null);
        assertNotNull(error);
        assertEquals(400, error.getRequestStatusCode());
        assertEquals("User need to relogin", error.getErrorMessage());
        assertEquals("OAuthException", error.getErrorType());
        assertEquals(190, error.getErrorCode());
        assertEquals(458, error.getSubErrorCode());
        assertTrue(error.getBatchRequestResult() instanceof JSONObject);
        assertEquals(FacebookRequestError.Category.AUTHENTICATION_REOPEN_SESSION, error.getCategory());
        assertEquals(R.string.com_facebook_requesterror_relogin, error.getUserActionMessageId());
    }
}
