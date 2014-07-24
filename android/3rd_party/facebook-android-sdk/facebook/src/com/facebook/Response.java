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

import android.content.Context;
import com.facebook.internal.*;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphObjectList;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

/**
 * Encapsulates the response, successful or otherwise, of a call to the Facebook platform.
 */
public class Response {
    private final HttpURLConnection connection;
    private final GraphObject graphObject;
    private final GraphObjectList<GraphObject> graphObjectList;
    private final boolean isFromCache;
    private final FacebookRequestError error;
    private final String rawResponse;
    private final Request request;

    /**
     * Property name of non-JSON results in the GraphObject. Certain calls to Facebook result in a non-JSON response
     * (e.g., the string literal "true" or "false"). To present a consistent way of accessing results, these are
     * represented as a GraphObject with a single string property with this name.
     */
    public static final String NON_JSON_RESPONSE_PROPERTY = "FACEBOOK_NON_JSON_RESULT";

    private static final int INVALID_SESSION_FACEBOOK_ERROR_CODE = 190;

    private static final String CODE_KEY = "code";
    private static final String BODY_KEY = "body";

    private static final String RESPONSE_LOG_TAG = "Response";

    private static final String RESPONSE_CACHE_TAG = "ResponseCache";
    private static FileLruCache responseCache;

    Response(Request request, HttpURLConnection connection, String rawResponse, GraphObject graphObject, boolean isFromCache) {
        this(request, connection, rawResponse, graphObject, null, isFromCache, null);
    }

    Response(Request request, HttpURLConnection connection, String rawResponse, GraphObjectList<GraphObject> graphObjects,
            boolean isFromCache) {
        this(request, connection, rawResponse, null, graphObjects, isFromCache, null);
    }

    Response(Request request, HttpURLConnection connection, FacebookRequestError error) {
        this(request, connection, null, null, null, false, error);
    }

    Response(Request request, HttpURLConnection connection, String rawResponse, GraphObject graphObject, GraphObjectList<GraphObject> graphObjects, boolean isFromCache, FacebookRequestError error) {
        this.request = request;
        this.connection = connection;
        this.rawResponse = rawResponse;
        this.graphObject = graphObject;
        this.graphObjectList = graphObjects;
        this.isFromCache = isFromCache;
        this.error = error;
    }

    /**
     * Returns information about any errors that may have occurred during the request.
     *
     * @return the error from the server, or null if there was no server error
     */
    public final FacebookRequestError getError() {
        return error;
    }

    /**
     * The single graph object returned for this request, if any.
     *
     * @return the graph object returned, or null if none was returned (or if the result was a list)
     */
    public final GraphObject getGraphObject() {
        return graphObject;
    }

    /**
     * The single graph object returned for this request, if any, cast into a particular type of GraphObject.
     *
     * @param graphObjectClass the GraphObject-derived interface to cast the graph object into
     * @return the graph object returned, or null if none was returned (or if the result was a list)
     * @throws FacebookException If the passed in Class is not a valid GraphObject interface
     */
    public final <T extends GraphObject> T getGraphObjectAs(Class<T> graphObjectClass) {
        if (graphObject == null) {
            return null;
        }
        if (graphObjectClass == null) {
            throw new NullPointerException("Must pass in a valid interface that extends GraphObject");
        }
        return graphObject.cast(graphObjectClass);
    }

    /**
     * The list of graph objects returned for this request, if any.
     *
     * @return the list of graph objects returned, or null if none was returned (or if the result was not a list)
     */
    public final GraphObjectList<GraphObject> getGraphObjectList() {
        return graphObjectList;
    }

    /**
     * The list of graph objects returned for this request, if any, cast into a particular type of GraphObject.
     *
     * @param graphObjectClass the GraphObject-derived interface to cast the graph objects into
     * @return the list of graph objects returned, or null if none was returned (or if the result was not a list)
     * @throws FacebookException If the passed in Class is not a valid GraphObject interface
     */
    public final <T extends GraphObject> GraphObjectList<T> getGraphObjectListAs(Class<T> graphObjectClass) {
        if (graphObjectList == null) {
            return null;
        }
        return graphObjectList.castToListOf(graphObjectClass);
    }

    /**
     * Returns the HttpURLConnection that this response was generated from. If the response was retrieved
     * from the cache, this will be null.
     *
     * @return the connection, or null
     */
    public final HttpURLConnection getConnection() {
        return connection;
    }

    /**
     * Returns the request that this response is for.
     *
     * @return the request that this response is for
     */
    public Request getRequest() {
        return request;
    }

    /**
     * Returns the server response as a String that this response is for.
     *
     * @return A String representation of the actual response from the server
     */
    public String getRawResponse() {
        return rawResponse;
    }

    /**
     * Indicates whether paging is being done forward or backward.
     */
    public enum PagingDirection {
        /**
         * Indicates that paging is being performed in the forward direction.
         */
        NEXT,
        /**
         * Indicates that paging is being performed in the backward direction.
         */
        PREVIOUS
    }

    /**
     * If a Response contains results that contain paging information, returns a new
     * Request that will retrieve the next page of results, in whichever direction
     * is desired. If no paging information is available, returns null.
     *
     * @param direction enum indicating whether to page forward or backward
     * @return a Request that will retrieve the next page of results in the desired
     *         direction, or null if no paging information is available
     */
    public Request getRequestForPagedResults(PagingDirection direction) {
        String link = null;
        if (graphObject != null) {
            PagedResults pagedResults = graphObject.cast(PagedResults.class);
            PagingInfo pagingInfo = pagedResults.getPaging();
            if (pagingInfo != null) {
                if (direction == PagingDirection.NEXT) {
                    link = pagingInfo.getNext();
                } else {
                    link = pagingInfo.getPrevious();
                }
            }
        }
        if (Utility.isNullOrEmpty(link)) {
            return null;
        }

        if (link != null && link.equals(request.getUrlForSingleRequest())) {
            // We got the same "next" link as we just tried to retrieve. This could happen if cached
            // data is invalid. All we can do in this case is pretend we have finished.
            return null;
        }

        Request pagingRequest;
        try {
            pagingRequest = new Request(request.getSession(), new URL(link));
        } catch (MalformedURLException e) {
            return null;
        }

        return pagingRequest;
    }

    /**
     * Provides a debugging string for this response.
     */
    @Override
    public String toString() {
        String responseCode;
        try {
            responseCode = String.format("%d", (connection != null) ? connection.getResponseCode() : 200);
        } catch (IOException e) {
            responseCode = "unknown";
        }

        return new StringBuilder().append("{Response: ").append(" responseCode: ").append(responseCode)
                .append(", graphObject: ").append(graphObject).append(", error: ").append(error)
                .append(", isFromCache:").append(isFromCache).append("}")
                .toString();
    }

    /**
     * Indicates whether the response was retrieved from a local cache or from the server.
     *
     * @return true if the response was cached locally, false if it was retrieved from the server
     */
    public final boolean getIsFromCache() {
        return isFromCache;
    }

    static FileLruCache getResponseCache() {
        if (responseCache == null) {
            Context applicationContext = Session.getStaticContext();
            if (applicationContext != null) {
                responseCache = new FileLruCache(applicationContext, RESPONSE_CACHE_TAG, new FileLruCache.Limits());
            }
        }

        return responseCache;
    }

    @SuppressWarnings("resource")
    static List<Response> fromHttpConnection(HttpURLConnection connection, RequestBatch requests) {
        InputStream stream = null;

        FileLruCache cache = null;
        String cacheKey = null;
        if (requests instanceof CacheableRequestBatch) {
            CacheableRequestBatch cacheableRequestBatch = (CacheableRequestBatch) requests;
            cache = getResponseCache();
            cacheKey = cacheableRequestBatch.getCacheKeyOverride();
            if (Utility.isNullOrEmpty(cacheKey)) {
                if (requests.size() == 1) {
                    // Default for single requests is to use the URL.
                    cacheKey = requests.get(0).getUrlForSingleRequest();
                } else {
                    Logger.log(LoggingBehavior.REQUESTS, RESPONSE_CACHE_TAG,
                            "Not using cache for cacheable request because no key was specified");
                }
            }

            // Try loading from cache.  If that fails, load from the network.
            if (!cacheableRequestBatch.getForceRoundTrip() && cache != null && !Utility.isNullOrEmpty(cacheKey)) {
                try {
                    stream = cache.get(cacheKey);
                    if (stream != null) {
                        return createResponsesFromStream(stream, null, requests, true);
                    }
                } catch (FacebookException exception) { // retry via roundtrip below
                } catch (JSONException exception) {
                } catch (IOException exception) {
                } finally {
                    Utility.closeQuietly(stream);
                }
            }
        }

        // Load from the network, and cache the result if not an error.
        try {
            if (connection.getResponseCode() >= 400) {
                stream = connection.getErrorStream();
            } else {
                stream = connection.getInputStream();
                if ((cache != null) && (cacheKey != null) && (stream != null)) {
                    InputStream interceptStream = cache.interceptAndPut(cacheKey, stream);
                    if (interceptStream != null) {
                        stream = interceptStream;
                    }
                }
            }

            return createResponsesFromStream(stream, connection, requests, false);
        } catch (FacebookException facebookException) {
            Logger.log(LoggingBehavior.REQUESTS, RESPONSE_LOG_TAG, "Response <Error>: %s", facebookException);
            return constructErrorResponses(requests, connection, facebookException);
        } catch (JSONException exception) {
            Logger.log(LoggingBehavior.REQUESTS, RESPONSE_LOG_TAG, "Response <Error>: %s", exception);
            return constructErrorResponses(requests, connection, new FacebookException(exception));
        } catch (IOException exception) {
            Logger.log(LoggingBehavior.REQUESTS, RESPONSE_LOG_TAG, "Response <Error>: %s", exception);
            return constructErrorResponses(requests, connection, new FacebookException(exception));
        } catch (SecurityException exception) {
            Logger.log(LoggingBehavior.REQUESTS, RESPONSE_LOG_TAG, "Response <Error>: %s", exception);
            return constructErrorResponses(requests, connection, new FacebookException(exception));
        } finally {
            Utility.closeQuietly(stream);
        }
    }

    static List<Response> createResponsesFromStream(InputStream stream, HttpURLConnection connection,
            RequestBatch requests, boolean isFromCache) throws FacebookException, JSONException, IOException {

        String responseString = Utility.readStreamToString(stream);
        Logger.log(LoggingBehavior.INCLUDE_RAW_RESPONSES, RESPONSE_LOG_TAG,
                "Response (raw)\n  Size: %d\n  Response:\n%s\n", responseString.length(),
                responseString);

        return createResponsesFromString(responseString, connection, requests, isFromCache);
    }

    static List<Response> createResponsesFromString(String responseString, HttpURLConnection connection,
            RequestBatch requests, boolean isFromCache) throws FacebookException, JSONException, IOException {
        JSONTokener tokener = new JSONTokener(responseString);
        Object resultObject = tokener.nextValue();

        List<Response> responses = createResponsesFromObject(connection, requests, resultObject, isFromCache);
        Logger.log(LoggingBehavior.REQUESTS, RESPONSE_LOG_TAG, "Response\n  Id: %s\n  Size: %d\n  Responses:\n%s\n",
                requests.getId(), responseString.length(), responses);

        return responses;
    }

    private static List<Response> createResponsesFromObject(HttpURLConnection connection, List<Request> requests,
            Object object, boolean isFromCache) throws FacebookException, JSONException {
        assert (connection != null) || isFromCache;

        int numRequests = requests.size();
        List<Response> responses = new ArrayList<Response>(numRequests);
        Object originalResult = object;

        if (numRequests == 1) {
            Request request = requests.get(0);
            try {
                // Single request case -- the entire response is the result, wrap it as "body" so we can handle it
                // the same as we do in the batched case. We get the response code from the actual HTTP response,
                // as opposed to the batched case where it is returned as a "code" element.
                JSONObject jsonObject = new JSONObject();
                jsonObject.put(BODY_KEY, object);
                int responseCode = (connection != null) ? connection.getResponseCode() : 200;
                jsonObject.put(CODE_KEY, responseCode);

                JSONArray jsonArray = new JSONArray();
                jsonArray.put(jsonObject);

                // Pretend we got an array of 1 back.
                object = jsonArray;
            } catch (JSONException e) {
                responses.add(new Response(request, connection, new FacebookRequestError(connection, e)));
            } catch (IOException e) {
                responses.add(new Response(request, connection, new FacebookRequestError(connection, e)));
            }
        }

        if (!(object instanceof JSONArray) || ((JSONArray) object).length() != numRequests) {
            FacebookException exception = new FacebookException("Unexpected number of results");
            throw exception;
        }

        JSONArray jsonArray = (JSONArray) object;

        for (int i = 0; i < jsonArray.length(); ++i) {
            Request request = requests.get(i);
            try {
                Object obj = jsonArray.get(i);
                responses.add(createResponseFromObject(request, connection, obj, isFromCache, originalResult));
            } catch (JSONException e) {
                responses.add(new Response(request, connection, new FacebookRequestError(connection, e)));
            } catch (FacebookException e) {
                responses.add(new Response(request, connection, new FacebookRequestError(connection, e)));
            }
        }

        return responses;
    }

    private static Response createResponseFromObject(Request request, HttpURLConnection connection, Object object,
            boolean isFromCache, Object originalResult) throws JSONException {
        if (object instanceof JSONObject) {
            JSONObject jsonObject = (JSONObject) object;

            FacebookRequestError error =
                    FacebookRequestError.checkResponseAndCreateError(jsonObject, originalResult, connection);
            if (error != null) {
                if (error.getErrorCode() == INVALID_SESSION_FACEBOOK_ERROR_CODE) {
                    Session session = request.getSession();
                    if (session != null) {
                        session.closeAndClearTokenInformation();
                    }
                }
                return new Response(request, connection, error);
            }

            Object body = Utility.getStringPropertyAsJSON(jsonObject, BODY_KEY, NON_JSON_RESPONSE_PROPERTY);

            if (body instanceof JSONObject) {
                GraphObject graphObject = GraphObject.Factory.create((JSONObject) body);
                return new Response(request, connection, body.toString(), graphObject, isFromCache);
            } else if (body instanceof JSONArray) {
                GraphObjectList<GraphObject> graphObjectList = GraphObject.Factory.createList(
                        (JSONArray) body, GraphObject.class);
                return new Response(request, connection, body.toString(), graphObjectList, isFromCache);
            }
            // We didn't get a body we understand how to handle, so pretend we got nothing.
            object = JSONObject.NULL;
        }

        if (object == JSONObject.NULL) {
            return new Response(request, connection, object.toString(), (GraphObject)null, isFromCache);
        } else {
            throw new FacebookException("Got unexpected object type in response, class: "
                    + object.getClass().getSimpleName());
        }
    }

    static List<Response> constructErrorResponses(List<Request> requests, HttpURLConnection connection,
            FacebookException error) {
        int count = requests.size();
        List<Response> responses = new ArrayList<Response>(count);
        for (int i = 0; i < count; ++i) {
            Response response = new Response(requests.get(i), connection, new FacebookRequestError(connection, error));
            responses.add(response);
        }
        return responses;
    }

    interface PagingInfo extends GraphObject {
        String getNext();

        String getPrevious();
    }

    interface PagedResults extends GraphObject {
        GraphObjectList<GraphObject> getData();

        PagingInfo getPaging();
    }

}
