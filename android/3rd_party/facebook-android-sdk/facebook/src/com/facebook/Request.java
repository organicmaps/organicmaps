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
import android.graphics.Bitmap;
import android.location.Location;
import android.net.Uri;
import android.os.*;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;
import com.facebook.internal.*;
import com.facebook.model.*;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLEncoder;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.Map.Entry;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A single request to be sent to the Facebook Platform through the <a
 * href="https://developers.facebook.com/docs/reference/api/">Graph API</a>. The Request class provides functionality
 * relating to serializing and deserializing requests and responses, making calls in batches (with a single round-trip
 * to the service) and making calls asynchronously.
 *
 * The particular service endpoint that a request targets is determined by a graph path (see the
 * {@link #setGraphPath(String) setGraphPath} method).
 *
 * A Request can be executed either anonymously or representing an authenticated user. In the former case, no Session
 * needs to be specified, while in the latter, a Session that is in an opened state must be provided. If requests are
 * executed in a batch, a Facebook application ID must be associated with the batch, either by supplying a Session for
 * at least one of the requests in the batch (the first one found in the batch will be used) or by calling the
 * {@link #setDefaultBatchApplicationId(String) setDefaultBatchApplicationId} method.
 *
 * After completion of a request, its Session, if any, will be checked to determine if its Facebook access token needs
 * to be extended; if so, a request to extend it will be issued in the background.
 */
public class Request {
    /**
     * The maximum number of requests that can be submitted in a single batch. This limit is enforced on the service
     * side by the Facebook platform, not by the Request class.
     */
    public static final int MAXIMUM_BATCH_SIZE = 50;

    public static final String TAG = Request.class.getSimpleName();

    private static final String ME = "me";
    private static final String MY_FRIENDS = "me/friends";
    private static final String MY_PHOTOS = "me/photos";
    private static final String MY_VIDEOS = "me/videos";
    private static final String VIDEOS_SUFFIX = "/videos";
    private static final String SEARCH = "search";
    private static final String MY_FEED = "me/feed";
    private static final String MY_STAGING_RESOURCES = "me/staging_resources";
    private static final String MY_OBJECTS_FORMAT = "me/objects/%s";
    private static final String MY_ACTION_FORMAT = "me/%s";

    private static final String USER_AGENT_BASE = "FBAndroidSDK";
    private static final String USER_AGENT_HEADER = "User-Agent";
    private static final String CONTENT_TYPE_HEADER = "Content-Type";
    private static final String ACCEPT_LANGUAGE_HEADER = "Accept-Language";

    // Parameter names/values
    private static final String PICTURE_PARAM = "picture";
    private static final String FORMAT_PARAM = "format";
    private static final String FORMAT_JSON = "json";
    private static final String SDK_PARAM = "sdk";
    private static final String SDK_ANDROID = "android";
    private static final String ACCESS_TOKEN_PARAM = "access_token";
    private static final String BATCH_ENTRY_NAME_PARAM = "name";
    private static final String BATCH_ENTRY_OMIT_RESPONSE_ON_SUCCESS_PARAM = "omit_response_on_success";
    private static final String BATCH_ENTRY_DEPENDS_ON_PARAM = "depends_on";
    private static final String BATCH_APP_ID_PARAM = "batch_app_id";
    private static final String BATCH_RELATIVE_URL_PARAM = "relative_url";
    private static final String BATCH_BODY_PARAM = "body";
    private static final String BATCH_METHOD_PARAM = "method";
    private static final String BATCH_PARAM = "batch";
    private static final String ATTACHMENT_FILENAME_PREFIX = "file";
    private static final String ATTACHED_FILES_PARAM = "attached_files";
    private static final String ISO_8601_FORMAT_STRING = "yyyy-MM-dd'T'HH:mm:ssZ";
    private static final String STAGING_PARAM = "file";
    private static final String OBJECT_PARAM = "object";

    private static final String MIME_BOUNDARY = "3i2ndDfv2rTHiSisAbouNdArYfORhtTPEefj3q2f";

    private static String defaultBatchApplicationId;

    // Group 1 in the pattern is the path without the version info
    private static Pattern versionPattern = Pattern.compile("^/?v\\d+\\.\\d+/(.*)");

    private Session session;
    private HttpMethod httpMethod;
    private String graphPath;
    private GraphObject graphObject;
    private String batchEntryName;
    private String batchEntryDependsOn;
    private boolean batchEntryOmitResultOnSuccess = true;
    private Bundle parameters;
    private Callback callback;
    private String overriddenURL;
    private Object tag;
    private String version;

    /**
     * Constructs a request without a session, graph path, or any other parameters.
     */
    public Request() {
        this(null, null, null, null, null);
    }

    /**
     * Constructs a request with a Session to retrieve a particular graph path. A Session need not be provided, in which
     * case the request is sent without an access token and thus is not executed in the context of any particular user.
     * Only certain graph requests can be expected to succeed in this case. If a Session is provided, it must be in an
     * opened state or the request will fail.
     *
     * @param session
     *            the Session to use, or null
     * @param graphPath
     *            the graph path to retrieve
     */
    public Request(Session session, String graphPath) {
        this(session, graphPath, null, null, null);
    }

    /**
     * Constructs a request with a specific Session, graph path, parameters, and HTTP method. A Session need not be
     * provided, in which case the request is sent without an access token and thus is not executed in the context of
     * any particular user. Only certain graph requests can be expected to succeed in this case. If a Session is
     * provided, it must be in an opened state or the request will fail.
     *
     * Depending on the httpMethod parameter, the object at the graph path may be retrieved, created, or deleted.
     *
     * @param session
     *            the Session to use, or null
     * @param graphPath
     *            the graph path to retrieve, create, or delete
     * @param parameters
     *            additional parameters to pass along with the Graph API request; parameters must be Strings, Numbers,
     *            Bitmaps, Dates, or Byte arrays.
     * @param httpMethod
     *            the {@link HttpMethod} to use for the request, or null for default (HttpMethod.GET)
     */
    public Request(Session session, String graphPath, Bundle parameters, HttpMethod httpMethod) {
        this(session, graphPath, parameters, httpMethod, null);
    }

    /**
     * Constructs a request with a specific Session, graph path, parameters, and HTTP method. A Session need not be
     * provided, in which case the request is sent without an access token and thus is not executed in the context of
     * any particular user. Only certain graph requests can be expected to succeed in this case. If a Session is
     * provided, it must be in an opened state or the request will fail.
     *
     * Depending on the httpMethod parameter, the object at the graph path may be retrieved, created, or deleted.
     *
     * @param session
     *            the Session to use, or null
     * @param graphPath
     *            the graph path to retrieve, create, or delete
     * @param parameters
     *            additional parameters to pass along with the Graph API request; parameters must be Strings, Numbers,
     *            Bitmaps, Dates, or Byte arrays.
     * @param httpMethod
     *            the {@link HttpMethod} to use for the request, or null for default (HttpMethod.GET)
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     */
    public Request(Session session, String graphPath, Bundle parameters, HttpMethod httpMethod, Callback callback) {
        this(session, graphPath, parameters, httpMethod, callback, null);
    }

    /**
     * Constructs a request with a specific Session, graph path, parameters, and HTTP method. A Session need not be
     * provided, in which case the request is sent without an access token and thus is not executed in the context of
     * any particular user. Only certain graph requests can be expected to succeed in this case. If a Session is
     * provided, it must be in an opened state or the request will fail.
     *
     * Depending on the httpMethod parameter, the object at the graph path may be retrieved, created, or deleted.
     *
     * @param session
     *            the Session to use, or null
     * @param graphPath
     *            the graph path to retrieve, create, or delete
     * @param parameters
     *            additional parameters to pass along with the Graph API request; parameters must be Strings, Numbers,
     *            Bitmaps, Dates, or Byte arrays.
     * @param httpMethod
     *            the {@link HttpMethod} to use for the request, or null for default (HttpMethod.GET)
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @param version
     *            the version of the Graph API
     */
    public Request(Session session, String graphPath, Bundle parameters, HttpMethod httpMethod, Callback callback, String version) {
        this.session = session;
        this.graphPath = graphPath;
        this.callback = callback;
        this.version = version;

        setHttpMethod(httpMethod);

        if (parameters != null) {
            this.parameters = new Bundle(parameters);
        } else {
            this.parameters = new Bundle();
        }

        if (this.version == null) {
            this.version = ServerProtocol.getAPIVersion();
        }
    }

    Request(Session session, URL overriddenURL) {
        this.session = session;
        this.overriddenURL = overriddenURL.toString();

        setHttpMethod(HttpMethod.GET);

        this.parameters = new Bundle();
    }

    /**
     * Creates a new Request configured to post a GraphObject to a particular graph path, to either create or update the
     * object at that path.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param graphPath
     *            the graph path to retrieve, create, or delete
     * @param graphObject
     *            the GraphObject to create or update
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newPostRequest(Session session, String graphPath, GraphObject graphObject, Callback callback) {
        Request request = new Request(session, graphPath, null, HttpMethod.POST , callback);
        request.setGraphObject(graphObject);
        return request;
    }

    /**
     * Creates a new Request configured to retrieve a user's own profile.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newMeRequest(Session session, final GraphUserCallback callback) {
        Callback wrapper = new Callback() {
            @Override
            public void onCompleted(Response response) {
                if (callback != null) {
                    callback.onCompleted(response.getGraphObjectAs(GraphUser.class), response);
                }
            }
        };
        return new Request(session, ME, null, null, wrapper);
    }

    /**
     * Creates a new Request configured to retrieve a user's friend list.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newMyFriendsRequest(Session session, final GraphUserListCallback callback) {
        Callback wrapper = new Callback() {
            @Override
            public void onCompleted(Response response) {
                if (callback != null) {
                    callback.onCompleted(typedListFromResponse(response, GraphUser.class), response);
                }
            }
        };
        return new Request(session, MY_FRIENDS, null, null, wrapper);
    }

    /**
     * Creates a new Request configured to upload a photo to the user's default photo album.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param image
     *            the image to upload
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newUploadPhotoRequest(Session session, Bitmap image, Callback callback) {
        Bundle parameters = new Bundle(1);
        parameters.putParcelable(PICTURE_PARAM, image);

        return new Request(session, MY_PHOTOS, parameters, HttpMethod.POST, callback);
    }

    /**
     * Creates a new Request configured to upload a photo to the user's default photo album. The photo
     * will be read from the specified stream.
     *
     * @param session  the Session to use, or null; if non-null, the session must be in an opened state
     * @param file     the file containing the photo to upload
     * @param callback a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newUploadPhotoRequest(Session session, File file,
            Callback callback) throws FileNotFoundException {
        ParcelFileDescriptor descriptor = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
        Bundle parameters = new Bundle(1);
        parameters.putParcelable(PICTURE_PARAM, descriptor);

        return new Request(session, MY_PHOTOS, parameters, HttpMethod.POST, callback);
    }

    /**
     * Creates a new Request configured to upload a photo to the user's default photo album. The photo
     * will be read from the specified file descriptor.
     *
     * @param session  the Session to use, or null; if non-null, the session must be in an opened state
     * @param file     the file to upload
     * @param callback a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newUploadVideoRequest(Session session, File file,
            Callback callback) throws FileNotFoundException {
        ParcelFileDescriptor descriptor = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
        Bundle parameters = new Bundle(1);
        parameters.putParcelable(file.getName(), descriptor);

        return new Request(session, MY_VIDEOS, parameters, HttpMethod.POST, callback);
    }

    /**
     * Creates a new Request configured to retrieve a particular graph path.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param graphPath
     *            the graph path to retrieve
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newGraphPathRequest(Session session, String graphPath, Callback callback) {
        return new Request(session, graphPath, null, null, callback);
    }

    /**
     * Creates a new Request that is configured to perform a search for places near a specified location via the Graph
     * API. At least one of location or searchText must be specified.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param location
     *            the location around which to search; only the latitude and longitude components of the location are
     *            meaningful
     * @param radiusInMeters
     *            the radius around the location to search, specified in meters; this is ignored if
     *            no location is specified
     * @param resultsLimit
     *            the maximum number of results to return
     * @param searchText
     *            optional text to search for as part of the name or type of an object
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     *
     * @throws FacebookException If neither location nor searchText is specified
     */
    public static Request newPlacesSearchRequest(Session session, Location location, int radiusInMeters,
            int resultsLimit, String searchText, final GraphPlaceListCallback callback) {
        if (location == null && Utility.isNullOrEmpty(searchText)) {
            throw new FacebookException("Either location or searchText must be specified.");
        }

        Bundle parameters = new Bundle(5);
        parameters.putString("type", "place");
        parameters.putInt("limit", resultsLimit);
        if (location != null) {
            parameters.putString("center",
                    String.format(Locale.US, "%f,%f", location.getLatitude(), location.getLongitude()));
            parameters.putInt("distance", radiusInMeters);
        }
        if (!Utility.isNullOrEmpty(searchText)) {
            parameters.putString("q", searchText);
        }

        Callback wrapper = new Callback() {
            @Override
            public void onCompleted(Response response) {
                if (callback != null) {
                    callback.onCompleted(typedListFromResponse(response, GraphPlace.class), response);
                }
            }
        };

        return new Request(session, SEARCH, parameters, HttpMethod.GET, wrapper);
    }

    /**
     * Creates a new Request configured to post a status update to a user's feed.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param message
     *            the text of the status update
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newStatusUpdateRequest(Session session, String message, Callback callback) {
        return newStatusUpdateRequest(session, message, (String)null, null, callback);
    }

    /**
     * Creates a new Request configured to post a status update to a user's feed.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param message
     *            the text of the status update
     * @param placeId
     *            an optional place id to associate with the post
     * @param tagIds
     *            an optional list of user ids to tag in the post
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    private static Request newStatusUpdateRequest(Session session, String message, String placeId, List<String> tagIds,
            Callback callback) {

        Bundle parameters = new Bundle();
        parameters.putString("message", message);

        if (placeId != null) {
            parameters.putString("place", placeId);
        }

        if (tagIds != null && tagIds.size() > 0) {
            String tags = TextUtils.join(",", tagIds);
            parameters.putString("tags", tags);
        }

        return new Request(session, MY_FEED, parameters, HttpMethod.POST, callback);
    }

    /**
     * Creates a new Request configured to post a status update to a user's feed.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param message
     *            the text of the status update
     * @param place
     *            an optional place to associate with the post
     * @param tags
     *            an optional list of users to tag in the post
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newStatusUpdateRequest(Session session, String message, GraphPlace place,
            List<GraphUser> tags, Callback callback) {

        List<String> tagIds = null;
        if (tags != null) {
            tagIds = new ArrayList<String>(tags.size());
            for (GraphUser tag: tags) {
                tagIds.add(tag.getId());
            }
        }
        String placeId = place == null ? null : place.getId();
        return newStatusUpdateRequest(session, message, placeId, tagIds, callback);
    }

    /**
     * Creates a new Request configured to retrieve an App User ID for the app's Facebook user.  Callers
     * will send this ID back to their own servers, collect up a set to create a Facebook Custom Audience with,
     * and then use the resultant Custom Audience to target ads.
     * <p/>
     * The GraphObject in the response will include an "custom_audience_third_party_id" property, with the value
     * being the ID retrieved.  This ID is an encrypted encoding of the Facebook user's ID and the
     * invoking Facebook app ID.  Multiple calls with the same user will return different IDs, thus these IDs cannot be
     * used to correlate behavior across devices or applications, and are only meaningful when sent back to Facebook
     * for creating Custom Audiences.
     * <p/>
     * The ID retrieved represents the Facebook user identified in the following way: if the specified session
     * (or activeSession if the specified session is `null`) is open, the ID will represent the user associated with
     * the activeSession; otherwise the ID will represent the user logged into the native Facebook app on the device.
     * A `null` ID will be provided into the callback if a) there is no native Facebook app, b) no one is logged into
     * it, or c) the app has previously called
     * {@link Settings#setLimitEventAndDataUsage(android.content.Context, boolean)} with `true` for this user.
     * <b>You must call this method from a background thread for it to work properly.</b>
     *
     * @param session
     *            the Session to issue the Request on, or null; if non-null, the session must be in an opened state.
     *            If there is no logged-in Facebook user, null is the expected choice.
     * @param context
     *            the Application context from which the app ID will be pulled, and from which the 'attribution ID'
     *            for the Facebook user is determined.  If there has been no app ID set, an exception will be thrown.
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions.
     *            The GraphObject in the Response will contain a "custom_audience_third_party_id" property that
     *            represents the user as described above.
     * @return a Request that is ready to execute
     */
    public static Request newCustomAudienceThirdPartyIdRequest(Session session, Context context, Callback callback) {
        return newCustomAudienceThirdPartyIdRequest(session, context, null, callback);
    }

    /**
     * Creates a new Request configured to retrieve an App User ID for the app's Facebook user.  Callers
     * will send this ID back to their own servers, collect up a set to create a Facebook Custom Audience with,
     * and then use the resultant Custom Audience to target ads.
     * <p/>
     * The GraphObject in the response will include an "custom_audience_third_party_id" property, with the value
     * being the ID retrieved.  This ID is an encrypted encoding of the Facebook user's ID and the
     * invoking Facebook app ID.  Multiple calls with the same user will return different IDs, thus these IDs cannot be
     * used to correlate behavior across devices or applications, and are only meaningful when sent back to Facebook
     * for creating Custom Audiences.
     * <p/>
     * The ID retrieved represents the Facebook user identified in the following way: if the specified session
     * (or activeSession if the specified session is `null`) is open, the ID will represent the user associated with
     * the activeSession; otherwise the ID will represent the user logged into the native Facebook app on the device.
     * A `null` ID will be provided into the callback if a) there is no native Facebook app, b) no one is logged into
     * it, or c) the app has previously called
     * {@link Settings#setLimitEventAndDataUsage(android.content.Context, boolean)} ;} with `true` for this user.
     * <b>You must call this method from a background thread for it to work properly.</b>
     *
     * @param session
     *            the Session to issue the Request on, or null; if non-null, the session must be in an opened state.
     *            If there is no logged-in Facebook user, null is the expected choice.
     * @param context
     *            the Application context from which the app ID will be pulled, and from which the 'attribution ID'
     *            for the Facebook user is determined.  If there has been no app ID set, an exception will be thrown.
     * @param applicationId
     *            explicitly specified Facebook App ID.  If null, and there's a valid session, then the application ID
     *            from the session will be used, otherwise the application ID from metadata will be used.
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions.
     *            The GraphObject in the Response will contain a "custom_audience_third_party_id" property that
     *            represents the user as described above.
     * @return a Request that is ready to execute
     */
    public static Request newCustomAudienceThirdPartyIdRequest(Session session,
            Context context, String applicationId, Callback callback) {

        // if provided session or activeSession is opened, use it.
        if (session == null) {
            session = Session.getActiveSession();
        }

        if (session != null && !session.isOpened()) {
            session = null;
        }

        if (applicationId == null) {
            if (session != null) {
                applicationId = session.getApplicationId();
            } else {
                applicationId = Utility.getMetadataApplicationId(context);
            }
        }

        if (applicationId == null) {
            throw new FacebookException("Facebook App ID cannot be determined");
        }

        String endpoint = applicationId + "/custom_audience_third_party_id";
        AttributionIdentifiers attributionIdentifiers = AttributionIdentifiers.getAttributionIdentifiers(context);
        Bundle parameters = new Bundle();

        if (session == null) {
            // Only use the attributionID if we don't have an open session.  If we do have an open session, then
            // the user token will be used to identify the user, and is more reliable than the attributionID.
            String udid = attributionIdentifiers.getAttributionId() != null
                ? attributionIdentifiers.getAttributionId()
                : attributionIdentifiers.getAndroidAdvertiserId();
            if (attributionIdentifiers.getAttributionId() != null) {
                parameters.putString("udid", udid);
            }
        }

        // Server will choose to not provide the App User ID in the event that event usage has been limited for
        // this user for this app.
        if (Settings.getLimitEventAndDataUsage(context) || attributionIdentifiers.isTrackingLimited()) {
            parameters.putString("limit_event_usage", "1");
        }

        return new Request(session, endpoint, parameters, HttpMethod.GET, callback);
    }

    /**
     * Creates a new Request configured to upload an image to create a staging resource. Staging resources
     * allow you to post binary data such as images, in preparation for a post of an Open Graph object or action
     * which references the image. The URI returned when uploading a staging resource may be passed as the image
     * property for an Open Graph object or action.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param image
     *            the image to upload
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newUploadStagingResourceWithImageRequest(Session session,
            Bitmap image, Callback callback) {
        Bundle parameters = new Bundle(1);
        parameters.putParcelable(STAGING_PARAM, image);

        return new Request(session, MY_STAGING_RESOURCES, parameters, HttpMethod.POST, callback);
    }

    /**
     * Creates a new Request configured to upload an image to create a staging resource. Staging resources
     * allow you to post binary data such as images, in preparation for a post of an Open Graph object or action
     * which references the image. The URI returned when uploading a staging resource may be passed as the image
     * property for an Open Graph object or action.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param file
     *            the file containing the image to upload
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newUploadStagingResourceWithImageRequest(Session session,
            File file, Callback callback) throws FileNotFoundException {
        ParcelFileDescriptor descriptor = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
        ParcelFileDescriptorWithMimeType descriptorWithMimeType = new ParcelFileDescriptorWithMimeType(descriptor, "image/png");
        Bundle parameters = new Bundle(1);
        parameters.putParcelable(STAGING_PARAM, descriptorWithMimeType);

        return new Request(session, MY_STAGING_RESOURCES, parameters, HttpMethod.POST, callback);
    }

    /**
     * Creates a new Request configured to create a user owned Open Graph object.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param openGraphObject
     *            the Open Graph object to create; must not be null, and must have a non-empty type and title
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newPostOpenGraphObjectRequest(Session session,
            OpenGraphObject openGraphObject, Callback callback) {
        if (openGraphObject == null) {
            throw new FacebookException("openGraphObject cannot be null");
        }
        if (Utility.isNullOrEmpty(openGraphObject.getType())) {
            throw new FacebookException("openGraphObject must have non-null 'type' property");
        }
        if (Utility.isNullOrEmpty(openGraphObject.getTitle())) {
            throw new FacebookException("openGraphObject must have non-null 'title' property");
        }

        String path = String.format(MY_OBJECTS_FORMAT, openGraphObject.getType());
        Bundle bundle = new Bundle();
        bundle.putString(OBJECT_PARAM, openGraphObject.getInnerJSONObject().toString());
        return new Request(session, path, bundle, HttpMethod.POST, callback);
    }

    /**
     * Creates a new Request configured to create a user owned Open Graph object.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param type
     *            the fully-specified Open Graph object type (e.g., my_app_namespace:my_object_name); must not be null
     * @param title
     *            the title of the Open Graph object; must not be null
     * @param imageUrl
     *            the link to an image to be associated with the Open Graph object; may be null
     * @param url
     *            the url to be associated with the Open Graph object; may be null
     * @param description
     *            the description to be associated with the object; may be null
     * @param objectProperties
     *            any additional type-specific properties for the Open Graph object; may be null
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions;
     *            may be null
     * @return a Request that is ready to execute
     */
    public static Request newPostOpenGraphObjectRequest(Session session, String type, String title, String imageUrl,
            String url, String description, GraphObject objectProperties, Callback callback) {
        OpenGraphObject openGraphObject = OpenGraphObject.Factory.createForPost(OpenGraphObject.class, type, title,
                imageUrl, url, description);
        if (objectProperties != null) {
            openGraphObject.setData(objectProperties);
        }

        return newPostOpenGraphObjectRequest(session, openGraphObject, callback);
    }

    /**
     * Creates a new Request configured to publish an Open Graph action.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param openGraphAction
     *            the Open Graph object to create; must not be null, and must have a non-empty 'type'
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newPostOpenGraphActionRequest(Session session, OpenGraphAction openGraphAction,
            Callback callback) {
        if (openGraphAction == null) {
            throw new FacebookException("openGraphAction cannot be null");
        }
        if (Utility.isNullOrEmpty(openGraphAction.getType())) {
            throw new FacebookException("openGraphAction must have non-null 'type' property");
        }

        String path = String.format(MY_ACTION_FORMAT, openGraphAction.getType());
        return newPostRequest(session, path, openGraphAction, callback);
    }

    /**
     * Creates a new Request configured to delete a resource through the Graph API.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param id
     *            the id of the object to delete
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newDeleteObjectRequest(Session session, String id, Callback callback) {
        return new Request(session, id, null, HttpMethod.DELETE, callback);
    }

    /**
     * Creates a new Request configured to update a user owned Open Graph object.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param openGraphObject
     *            the Open Graph object to update, which must have a valid 'id' property
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newUpdateOpenGraphObjectRequest(Session session, OpenGraphObject openGraphObject,
            Callback callback) {
        if (openGraphObject == null) {
            throw new FacebookException("openGraphObject cannot be null");
        }

        String path = openGraphObject.getId();
        if (path == null) {
            throw new FacebookException("openGraphObject must have an id");
        }

        Bundle bundle = new Bundle();
        bundle.putString(OBJECT_PARAM, openGraphObject.getInnerJSONObject().toString());
        return new Request(session, path, bundle, HttpMethod.POST, callback);
    }

    /**
     * Creates a new Request configured to update a user owned Open Graph object.
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param id
     *            the id of the Open Graph object
     * @param title
     *            the title of the Open Graph object
     * @param imageUrl
     *            the link to an image to be associated with the Open Graph object
     * @param url
     *            the url to be associated with the Open Graph object
     * @param description
     *            the description to be associated with the object
     * @param objectProperties
     *            any additional type-specific properties for the Open Graph object
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a Request that is ready to execute
     */
    public static Request newUpdateOpenGraphObjectRequest(Session session, String id, String title, String imageUrl,
            String url, String description, GraphObject objectProperties, Callback callback) {
        OpenGraphObject openGraphObject = OpenGraphObject.Factory.createForPost(OpenGraphObject.class, null, title,
                imageUrl, url, description);
        openGraphObject.setId(id);
        openGraphObject.setData(objectProperties);

        return newUpdateOpenGraphObjectRequest(session, openGraphObject, callback);
    }

    /**
     * Returns the GraphObject, if any, associated with this request.
     *
     * @return the GraphObject associated with this requeset, or null if there is none
     */
    public final GraphObject getGraphObject() {
        return this.graphObject;
    }

    /**
     * Sets the GraphObject associated with this request. This is meaningful only for POST requests.
     *
     * @param graphObject
     *            the GraphObject to upload along with this request
     */
    public final void setGraphObject(GraphObject graphObject) {
        this.graphObject = graphObject;
    }

    /**
     * Returns the graph path of this request, if any.
     *
     * @return the graph path of this request, or null if there is none
     */
    public final String getGraphPath() {
        return this.graphPath;
    }

    /**
     * Sets the graph path of this request.
     *
     * @param graphPath
     *            the graph path for this request
     */
    public final void setGraphPath(String graphPath) {
        this.graphPath = graphPath;
    }

    /**
     * Returns the {@link HttpMethod} to use for this request.
     *
     * @return the HttpMethod
     */
    public final HttpMethod getHttpMethod() {
        return this.httpMethod;
    }

    /**
     * Sets the {@link HttpMethod} to use for this request.
     *
     * @param httpMethod
     *            the HttpMethod, or null for the default (HttpMethod.GET).
     */
    public final void setHttpMethod(HttpMethod httpMethod) {
        if (overriddenURL != null && httpMethod != HttpMethod.GET) {
            throw new FacebookException("Can't change HTTP method on request with overridden URL.");
            }
        this.httpMethod = (httpMethod != null) ? httpMethod : HttpMethod.GET;
    }

    /**
     * Returns the version of the API that this request will use.  By default this is the current API at the time
     * the SDK is released.
     *
     * @return the version that this request will use
     */
    public final String getVersion() {
        return this.version;
    }

    /**
     * Set the version to use for this request.  By default the version will be the current API at the time the SDK
     * is released.  Only use this if you need to explicitly override.
     *
     * @param version The version to use.  Should look like "v2.0"
     */
    public final void setVersion(String version) {
        this.version = version;
    }

    /**
     * Returns the parameters for this request.
     *
     * @return the parameters
     */
    public final Bundle getParameters() {
        return this.parameters;
    }

    /**
     * Sets the parameters for this request.
     *
     * @param parameters
     *            the parameters
     */
    public final void setParameters(Bundle parameters) {
        this.parameters = parameters;
    }

    /**
     * Returns the Session associated with this request.
     *
     * @return the Session associated with this request, or null if none has been specified
     */
    public final Session getSession() {
        return this.session;
    }

    /**
     * Sets the Session to use for this request. The Session does not need to be opened at the time it is specified, but
     * it must be opened by the time the request is executed.
     *
     * @param session
     *            the Session to use for this request
     */
    public final void setSession(Session session) {
        this.session = session;
    }

    /**
     * Returns the name of this request's entry in a batched request.
     *
     * @return the name of this request's batch entry, or null if none has been specified
     */
    public final String getBatchEntryName() {
        return this.batchEntryName;
    }

    /**
     * Sets the name of this request's entry in a batched request. This value is only used if this request is submitted
     * as part of a batched request. It can be used to specified dependencies between requests. See <a
     * href="https://developers.facebook.com/docs/reference/api/batch/">Batch Requests</a> in the Graph API
     * documentation for more details.
     *
     * @param batchEntryName
     *            the name of this request's entry in a batched request, which must be unique within a particular batch
     *            of requests
     */
    public final void setBatchEntryName(String batchEntryName) {
        this.batchEntryName = batchEntryName;
    }

    /**
     * Returns the name of the request that this request entry explicitly depends on in a batched request.
     *
     * @return the name of this request's dependency, or null if none has been specified
     */
    public final String getBatchEntryDependsOn() {
        return this.batchEntryDependsOn;
    }

    /**
     * Sets the name of the request entry that this request explicitly depends on in a batched request. This value is
     * only used if this request is submitted as part of a batched request. It can be used to specified dependencies
     * between requests. See <a href="https://developers.facebook.com/docs/reference/api/batch/">Batch Requests</a> in
     * the Graph API documentation for more details.
     *
     * @param batchEntryDependsOn
     *            the name of the request entry that this entry depends on in a batched request
     */
    public final void setBatchEntryDependsOn(String batchEntryDependsOn) {
        this.batchEntryDependsOn = batchEntryDependsOn;
    }


    /**
     * Returns whether or not this batch entry will return a response if it is successful. Only applies if another
     * request entry in the batch specifies this entry as a dependency.
     *
     * @return the name of this request's dependency, or null if none has been specified
     */
    public final boolean getBatchEntryOmitResultOnSuccess() {
        return this.batchEntryOmitResultOnSuccess;
    }

    /**
     * Sets whether or not this batch entry will return a response if it is successful. Only applies if another
     * request entry in the batch specifies this entry as a dependency. See
     * <a href="https://developers.facebook.com/docs/reference/api/batch/">Batch Requests</a> in the Graph API
     * documentation for more details.
     *
     * @param batchEntryOmitResultOnSuccess
     *            the name of the request entry that this entry depends on in a batched request
     */
    public final void setBatchEntryOmitResultOnSuccess(boolean batchEntryOmitResultOnSuccess) {
        this.batchEntryOmitResultOnSuccess = batchEntryOmitResultOnSuccess;
    }

    /**
     * Gets the default Facebook application ID that will be used to submit batched requests if none of those requests
     * specifies a Session. Batched requests require an application ID, so either at least one request in a batch must
     * specify a Session or the application ID must be specified explicitly.
     *
     * @return the Facebook application ID to use for batched requests if none can be determined
     */
    public static final String getDefaultBatchApplicationId() {
        return Request.defaultBatchApplicationId;
    }

    /**
     * Sets the default application ID that will be used to submit batched requests if none of those requests specifies
     * a Session. Batched requests require an application ID, so either at least one request in a batch must specify a
     * Session or the application ID must be specified explicitly.
     *
     * @param applicationId
     *            the Facebook application ID to use for batched requests if none can be determined
     */
    public static final void setDefaultBatchApplicationId(String applicationId) {
        defaultBatchApplicationId = applicationId;
    }

    /**
     * Returns the callback which will be called when the request finishes.
     *
     * @return the callback
     */
    public final Callback getCallback() {
        return callback;
    }

    /**
     * Sets the callback which will be called when the request finishes.
     *
     * @param callback
     *            the callback
     */
    public final void setCallback(Callback callback) {
        this.callback = callback;
    }

    /**
     * Sets the tag on the request; this is an application-defined object that can be used to distinguish
     * between different requests. Its value has no effect on the execution of the request.
     *
     * @param tag an object to serve as a tag, or null
     */
    public final void setTag(Object tag) {
        this.tag = tag;
    }

    /**
     * Gets the tag on the request; this is an application-defined object that can be used to distinguish
     * between different requests. Its value has no effect on the execution of the request.
     *
     * @return an object that serves as a tag, or null
     */
    public final Object getTag() {
        return tag;
    }

    /**
     * Starts a new Request configured to post a GraphObject to a particular graph path, to either create or update the
     * object at that path.
     * <p/>
     * This should only be called from the UI thread.
     *
     * This method is deprecated. Prefer to call Request.newPostRequest(...).executeAsync();
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param graphPath
     *            the graph path to retrieve, create, or delete
     * @param graphObject
     *            the GraphObject to create or update
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a RequestAsyncTask that is executing the request
     */
    @Deprecated
    public static RequestAsyncTask executePostRequestAsync(Session session, String graphPath, GraphObject graphObject,
            Callback callback) {
        return newPostRequest(session, graphPath, graphObject, callback).executeAsync();
    }

    /**
     * Starts a new Request configured to retrieve a user's own profile.
     * <p/>
     * This should only be called from the UI thread.
     *
     * This method is deprecated. Prefer to call Request.newMeRequest(...).executeAsync();
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a RequestAsyncTask that is executing the request
     */
    @Deprecated
    public static RequestAsyncTask executeMeRequestAsync(Session session, GraphUserCallback callback) {
        return newMeRequest(session, callback).executeAsync();
    }

    /**
     * Starts a new Request configured to retrieve a user's friend list.
     * <p/>
     * This should only be called from the UI thread.
     *
     * This method is deprecated. Prefer to call Request.newMyFriendsRequest(...).executeAsync();
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a RequestAsyncTask that is executing the request
     */
    @Deprecated
    public static RequestAsyncTask executeMyFriendsRequestAsync(Session session, GraphUserListCallback callback) {
        return newMyFriendsRequest(session, callback).executeAsync();
    }

    /**
     * Starts a new Request configured to upload a photo to the user's default photo album.
     * <p/>
     * This should only be called from the UI thread.
     *
     * This method is deprecated. Prefer to call Request.newUploadPhotoRequest(...).executeAsync();
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param image
     *            the image to upload
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a RequestAsyncTask that is executing the request
     */
    @Deprecated
    public static RequestAsyncTask executeUploadPhotoRequestAsync(Session session, Bitmap image, Callback callback) {
        return newUploadPhotoRequest(session, image, callback).executeAsync();
    }

    /**
     * Starts a new Request configured to upload a photo to the user's default photo album. The photo
     * will be read from the specified stream.
     * <p/>
     * This should only be called from the UI thread.
     *
     * This method is deprecated. Prefer to call Request.newUploadPhotoRequest(...).executeAsync();
     *
     * @param session  the Session to use, or null; if non-null, the session must be in an opened state
     * @param file     the file containing the photo to upload
     * @param callback a callback that will be called when the request is completed to handle success or error conditions
     * @return a RequestAsyncTask that is executing the request
     */
    @Deprecated
    public static RequestAsyncTask executeUploadPhotoRequestAsync(Session session, File file,
            Callback callback) throws FileNotFoundException {
        return newUploadPhotoRequest(session, file, callback).executeAsync();
    }

    /**
     * Starts a new Request configured to retrieve a particular graph path.
     * <p/>
     * This should only be called from the UI thread.
     *
     * This method is deprecated. Prefer to call Request.newGraphPathRequest(...).executeAsync();
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param graphPath
     *            the graph path to retrieve
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a RequestAsyncTask that is executing the request
     */
    @Deprecated
    public static RequestAsyncTask executeGraphPathRequestAsync(Session session, String graphPath, Callback callback) {
        return newGraphPathRequest(session, graphPath, callback).executeAsync();
    }

    /**
     * Starts a new Request that is configured to perform a search for places near a specified location via the Graph
     * API.
     * <p/>
     * This should only be called from the UI thread.
     *
     * This method is deprecated. Prefer to call Request.newPlacesSearchRequest(...).executeAsync();
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param location
     *            the location around which to search; only the latitude and longitude components of the location are
     *            meaningful
     * @param radiusInMeters
     *            the radius around the location to search, specified in meters
     * @param resultsLimit
     *            the maximum number of results to return
     * @param searchText
     *            optional text to search for as part of the name or type of an object
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a RequestAsyncTask that is executing the request
     *
     * @throws FacebookException If neither location nor searchText is specified
     */
    @Deprecated
    public static RequestAsyncTask executePlacesSearchRequestAsync(Session session, Location location,
            int radiusInMeters, int resultsLimit, String searchText, GraphPlaceListCallback callback) {
        return newPlacesSearchRequest(session, location, radiusInMeters, resultsLimit, searchText, callback)
                .executeAsync();
    }

    /**
     * Starts a new Request configured to post a status update to a user's feed.
     * <p/>
     * This should only be called from the UI thread.
     *
     * This method is deprecated. Prefer to call Request.newStatusUpdateRequest(...).executeAsync();
     *
     * @param session
     *            the Session to use, or null; if non-null, the session must be in an opened state
     * @param message
     *            the text of the status update
     * @param callback
     *            a callback that will be called when the request is completed to handle success or error conditions
     * @return a RequestAsyncTask that is executing the request
     */
    @Deprecated
    public static RequestAsyncTask executeStatusUpdateRequestAsync(Session session, String message, Callback callback) {
        return newStatusUpdateRequest(session, message, callback).executeAsync();
    }

    /**
     * Executes this request and returns the response.
     * <p/>
     * This should only be called if you have transitioned off the UI thread.
     *
     * @return the Response object representing the results of the request
     *
     * @throws FacebookException
     *            If there was an error in the protocol used to communicate with the service
     * @throws IllegalArgumentException
     */
    public final Response executeAndWait() {
        return Request.executeAndWait(this);
    }

    /**
     * Executes this request and returns the response.
     * <p/>
     * This should only be called from the UI thread.
     *
     * @return a RequestAsyncTask that is executing the request
     *
     * @throws IllegalArgumentException
     */
    public final RequestAsyncTask executeAsync() {
        return Request.executeBatchAsync(this);
    }

    /**
     * Serializes one or more requests but does not execute them. The resulting HttpURLConnection can be executed
     * explicitly by the caller.
     *
     * @param requests
     *            one or more Requests to serialize
     * @return an HttpURLConnection which is ready to execute
     *
     * @throws FacebookException
     *            If any of the requests in the batch are badly constructed or if there are problems
     *            contacting the service
     * @throws IllegalArgumentException if the passed in array is zero-length
     * @throws NullPointerException if the passed in array or any of its contents are null
     */
    public static HttpURLConnection toHttpConnection(Request... requests) {
        return toHttpConnection(Arrays.asList(requests));
    }

    /**
     * Serializes one or more requests but does not execute them. The resulting HttpURLConnection can be executed
     * explicitly by the caller.
     *
     * @param requests
     *            one or more Requests to serialize
     * @return an HttpURLConnection which is ready to execute
     *
     * @throws FacebookException
     *            If any of the requests in the batch are badly constructed or if there are problems
     *            contacting the service
     * @throws IllegalArgumentException if the passed in collection is empty
     * @throws NullPointerException if the passed in collection or any of its contents are null
     */
    public static HttpURLConnection toHttpConnection(Collection<Request> requests) {
        Validate.notEmptyAndContainsNoNulls(requests, "requests");

        return toHttpConnection(new RequestBatch(requests));
    }


    /**
     * Serializes one or more requests but does not execute them. The resulting HttpURLConnection can be executed
     * explicitly by the caller.
     *
     * @param requests
     *            a RequestBatch to serialize
     * @return an HttpURLConnection which is ready to execute
     *
     * @throws FacebookException
     *            If any of the requests in the batch are badly constructed or if there are problems
     *            contacting the service
     * @throws IllegalArgumentException
     */
    public static HttpURLConnection toHttpConnection(RequestBatch requests) {

        URL url = null;
        try {
            if (requests.size() == 1) {
                // Single request case.
                Request request = requests.get(0);
                // In the non-batch case, the URL we use really is the same one returned by getUrlForSingleRequest.
                url = new URL(request.getUrlForSingleRequest());
            } else {
                // Batch case -- URL is just the graph API base, individual request URLs are serialized
                // as relative_url parameters within each batch entry.
                url = new URL(ServerProtocol.getGraphUrlBase());
            }
        } catch (MalformedURLException e) {
            throw new FacebookException("could not construct URL for request", e);
        }

        HttpURLConnection connection;
        try {
            connection = createConnection(url);

            serializeToUrlConnection(requests, connection);
        } catch (IOException e) {
            throw new FacebookException("could not construct request body", e);
        } catch (JSONException e) {
            throw new FacebookException("could not construct request body", e);
        }

        return connection;
    }

    /**
     * Executes a single request on the current thread and returns the response.
     * <p/>
     * This should only be used if you have transitioned off the UI thread.
     *
     * @param request
     *            the Request to execute
     *
     * @return the Response object representing the results of the request
     *
     * @throws FacebookException
     *            If there was an error in the protocol used to communicate with the service
     */
    public static Response executeAndWait(Request request) {
        List<Response> responses = executeBatchAndWait(request);

        if (responses == null || responses.size() != 1) {
            throw new FacebookException("invalid state: expected a single response");
        }

        return responses.get(0);
    }

    /**
     * Executes requests on the current thread as a single batch and returns the responses.
     * <p/>
     * This should only be used if you have transitioned off the UI thread.
     *
     * @param requests
     *            the Requests to execute
     *
     * @return a list of Response objects representing the results of the requests; responses are returned in the same
     *         order as the requests were specified.
     *
     * @throws NullPointerException
     *            In case of a null request
     * @throws FacebookException
     *            If there was an error in the protocol used to communicate with the service
     */
    public static List<Response> executeBatchAndWait(Request... requests) {
        Validate.notNull(requests, "requests");

        return executeBatchAndWait(Arrays.asList(requests));
    }

    /**
     * Executes requests as a single batch on the current thread and returns the responses.
     * <p/>
     * This should only be used if you have transitioned off the UI thread.
     *
     * @param requests
     *            the Requests to execute
     *
     * @return a list of Response objects representing the results of the requests; responses are returned in the same
     *         order as the requests were specified.
     *
     * @throws FacebookException
     *            If there was an error in the protocol used to communicate with the service
     */
    public static List<Response> executeBatchAndWait(Collection<Request> requests) {
        return executeBatchAndWait(new RequestBatch(requests));
    }

    /**
     * Executes requests on the current thread as a single batch and returns the responses.
     * <p/>
     * This should only be used if you have transitioned off the UI thread.
     *
     * @param requests
     *            the batch of Requests to execute
     *
     * @return a list of Response objects representing the results of the requests; responses are returned in the same
     *         order as the requests were specified.
     *
     * @throws FacebookException
     *            If there was an error in the protocol used to communicate with the service
     * @throws IllegalArgumentException if the passed in RequestBatch is empty
     * @throws NullPointerException if the passed in RequestBatch or any of its contents are null
     */
    public static List<Response> executeBatchAndWait(RequestBatch requests) {
        Validate.notEmptyAndContainsNoNulls(requests, "requests");

        HttpURLConnection connection = null;
        try {
            connection = toHttpConnection(requests);
        } catch (Exception ex) {
            List<Response> responses = Response.constructErrorResponses(requests.getRequests(), null, new FacebookException(ex));
            runCallbacks(requests, responses);
            return responses;
        }

        List<Response> responses = executeConnectionAndWait(connection, requests);
        return responses;
    }

    /**
     * Executes requests as a single batch asynchronously. This function will return immediately, and the requests will
     * be processed on a separate thread. In order to process results of a request, or determine whether a request
     * succeeded or failed, a callback must be specified (see the {@link #setCallback(Callback) setCallback} method).
     * <p/>
     * This should only be called from the UI thread.
     *
     * @param requests
     *            the Requests to execute
     * @return a RequestAsyncTask that is executing the request
     *
     * @throws NullPointerException
     *            If a null request is passed in
     */
    public static RequestAsyncTask executeBatchAsync(Request... requests) {
        Validate.notNull(requests, "requests");

        return executeBatchAsync(Arrays.asList(requests));
    }

    /**
     * Executes requests as a single batch asynchronously. This function will return immediately, and the requests will
     * be processed on a separate thread. In order to process results of a request, or determine whether a request
     * succeeded or failed, a callback must be specified (see the {@link #setCallback(Callback) setCallback} method).
     * <p/>
     * This should only be called from the UI thread.
     *
     * @param requests
     *            the Requests to execute
     * @return a RequestAsyncTask that is executing the request
     *
     * @throws IllegalArgumentException if the passed in collection is empty
     * @throws NullPointerException if the passed in collection or any of its contents are null
     */
    public static RequestAsyncTask executeBatchAsync(Collection<Request> requests) {
        return executeBatchAsync(new RequestBatch(requests));
    }

    /**
     * Executes requests as a single batch asynchronously. This function will return immediately, and the requests will
     * be processed on a separate thread. In order to process results of a request, or determine whether a request
     * succeeded or failed, a callback must be specified (see the {@link #setCallback(Callback) setCallback} method).
     * <p/>
     * This should only be called from the UI thread.
     *
     * @param requests
     *            the RequestBatch to execute
     * @return a RequestAsyncTask that is executing the request
     *
     * @throws IllegalArgumentException if the passed in RequestBatch is empty
     * @throws NullPointerException if the passed in RequestBatch or any of its contents are null
     */
    public static RequestAsyncTask executeBatchAsync(RequestBatch requests) {
        Validate.notEmptyAndContainsNoNulls(requests, "requests");

        RequestAsyncTask asyncTask = new RequestAsyncTask(requests);
        asyncTask.executeOnSettingsExecutor();
        return asyncTask;
    }

    /**
     * Executes requests that have already been serialized into an HttpURLConnection. No validation is done that the
     * contents of the connection actually reflect the serialized requests, so it is the caller's responsibility to
     * ensure that it will correctly generate the desired responses.
     * <p/>
     * This should only be called if you have transitioned off the UI thread.
     *
     * @param connection
     *            the HttpURLConnection that the requests were serialized into
     * @param requests
     *            the requests represented by the HttpURLConnection
     * @return a list of Responses corresponding to the requests
     *
     * @throws FacebookException
     *            If there was an error in the protocol used to communicate with the service
     */
    public static List<Response> executeConnectionAndWait(HttpURLConnection connection, Collection<Request> requests) {
        return executeConnectionAndWait(connection, new RequestBatch(requests));
    }

    /**
     * Executes requests that have already been serialized into an HttpURLConnection. No validation is done that the
     * contents of the connection actually reflect the serialized requests, so it is the caller's responsibility to
     * ensure that it will correctly generate the desired responses.
     * <p/>
     * This should only be called if you have transitioned off the UI thread.
     *
     * @param connection
     *            the HttpURLConnection that the requests were serialized into
     * @param requests
     *            the RequestBatch represented by the HttpURLConnection
     * @return a list of Responses corresponding to the requests
     *
     * @throws FacebookException
     *            If there was an error in the protocol used to communicate with the service
     */
    public static List<Response> executeConnectionAndWait(HttpURLConnection connection, RequestBatch requests) {
        List<Response> responses = Response.fromHttpConnection(connection, requests);

        Utility.disconnectQuietly(connection);

        int numRequests = requests.size();
        if (numRequests != responses.size()) {
            throw new FacebookException(String.format("Received %d responses while expecting %d", responses.size(),
                    numRequests));
        }

        runCallbacks(requests, responses);

        // See if any of these sessions needs its token to be extended. We do this after issuing the request so as to
        // reduce network contention.
        HashSet<Session> sessions = new HashSet<Session>();
        for (Request request : requests) {
            if (request.session != null) {
                sessions.add(request.session);
            }
        }
        for (Session session : sessions) {
            session.extendAccessTokenIfNeeded();
        }

        return responses;
    }

    /**
     * Asynchronously executes requests that have already been serialized into an HttpURLConnection. No validation is
     * done that the contents of the connection actually reflect the serialized requests, so it is the caller's
     * responsibility to ensure that it will correctly generate the desired responses. This function will return
     * immediately, and the requests will be processed on a separate thread. In order to process results of a request,
     * or determine whether a request succeeded or failed, a callback must be specified (see the
     * {@link #setCallback(Callback) setCallback} method).
     * <p/>
     * This should only be called from the UI thread.
     *
     * @param connection
     *            the HttpURLConnection that the requests were serialized into
     * @param requests
     *            the requests represented by the HttpURLConnection
     * @return a RequestAsyncTask that is executing the request
     */
    public static RequestAsyncTask executeConnectionAsync(HttpURLConnection connection, RequestBatch requests) {
        return executeConnectionAsync(null, connection, requests);
    }

    /**
     * Asynchronously executes requests that have already been serialized into an HttpURLConnection. No validation is
     * done that the contents of the connection actually reflect the serialized requests, so it is the caller's
     * responsibility to ensure that it will correctly generate the desired responses. This function will return
     * immediately, and the requests will be processed on a separate thread. In order to process results of a request,
     * or determine whether a request succeeded or failed, a callback must be specified (see the
     * {@link #setCallback(Callback) setCallback} method)
     * <p/>
     * This should only be called from the UI thread.
     *
     * @param callbackHandler
     *            a Handler that will be used to post calls to the callback for each request; if null, a Handler will be
     *            instantiated on the calling thread
     * @param connection
     *            the HttpURLConnection that the requests were serialized into
     * @param requests
     *            the requests represented by the HttpURLConnection
     * @return a RequestAsyncTask that is executing the request
     */
    public static RequestAsyncTask executeConnectionAsync(Handler callbackHandler, HttpURLConnection connection,
            RequestBatch requests) {
        Validate.notNull(connection, "connection");

        RequestAsyncTask asyncTask = new RequestAsyncTask(connection, requests);
        requests.setCallbackHandler(callbackHandler);
        asyncTask.executeOnSettingsExecutor();
        return asyncTask;
    }

    /**
     * Returns a string representation of this Request, useful for debugging.
     *
     * @return the debugging information
     */
    @Override
    public String toString() {
        return new StringBuilder().append("{Request: ").append(" session: ").append(session).append(", graphPath: ")
                .append(graphPath).append(", graphObject: ").append(graphObject)
                .append(", httpMethod: ").append(httpMethod).append(", parameters: ")
                .append(parameters).append("}").toString();
    }

    static void runCallbacks(final RequestBatch requests, List<Response> responses) {
        int numRequests = requests.size();

        // Compile the list of callbacks to call and then run them either on this thread or via the Handler we received
        final ArrayList<Pair<Callback, Response>> callbacks = new ArrayList<Pair<Callback, Response>>();
        for (int i = 0; i < numRequests; ++i) {
            Request request = requests.get(i);
            if (request.callback != null) {
                callbacks.add(new Pair<Callback, Response>(request.callback, responses.get(i)));
            }
        }

        if (callbacks.size() > 0) {
            Runnable runnable = new Runnable() {
                public void run() {
                    for (Pair<Callback, Response> pair : callbacks) {
                        pair.first.onCompleted(pair.second);
                    }

                    List<RequestBatch.Callback> batchCallbacks = requests.getCallbacks();
                    for (RequestBatch.Callback batchCallback : batchCallbacks) {
                        batchCallback.onBatchCompleted(requests);
                    }
                }
            };

            Handler callbackHandler = requests.getCallbackHandler();
            if (callbackHandler == null) {
                // Run on this thread.
                runnable.run();
            } else {
                // Post to the handler.
                callbackHandler.post(runnable);
            }
        }
    }

    static HttpURLConnection createConnection(URL url) throws IOException {
        HttpURLConnection connection;
        connection = (HttpURLConnection) url.openConnection();

        connection.setRequestProperty(USER_AGENT_HEADER, getUserAgent());
        connection.setRequestProperty(CONTENT_TYPE_HEADER, getMimeContentType());
        connection.setRequestProperty(ACCEPT_LANGUAGE_HEADER, Locale.getDefault().toString());

        connection.setChunkedStreamingMode(0);
        return connection;
    }


    private void addCommonParameters() {
        if (this.session != null) {
            if (!this.session.isOpened()) {
                throw new FacebookException("Session provided to a Request in un-opened state.");
            } else if (!this.parameters.containsKey(ACCESS_TOKEN_PARAM)) {
                String accessToken = this.session.getAccessToken();
                Logger.registerAccessToken(accessToken);
                this.parameters.putString(ACCESS_TOKEN_PARAM, accessToken);
            }
        } else if (!this.parameters.containsKey(ACCESS_TOKEN_PARAM)) {
            String appID = Settings.getApplicationId();
            String clientToken = Settings.getClientToken();
            if (!Utility.isNullOrEmpty(appID) && !Utility.isNullOrEmpty(clientToken)) {
                String accessToken = appID + "|" + clientToken;
                this.parameters.putString(ACCESS_TOKEN_PARAM, accessToken);
            } else {
                Log.d(TAG,
                        "Warning: Sessionless Request needs token but missing either application ID or client token.");
            }
        }
        this.parameters.putString(SDK_PARAM, SDK_ANDROID);
        this.parameters.putString(FORMAT_PARAM, FORMAT_JSON);
    }

    private String appendParametersToBaseUrl(String baseUrl) {
        Uri.Builder uriBuilder = new Uri.Builder().encodedPath(baseUrl);

        Set<String> keys = this.parameters.keySet();
        for (String key : keys) {
            Object value = this.parameters.get(key);

            if (value == null) {
                value = "";
            }

            if (isSupportedParameterType(value)) {
                value = parameterToString(value);
            } else {
                if (httpMethod == HttpMethod.GET) {
                    throw new IllegalArgumentException(String.format("Unsupported parameter type for GET request: %s",
                                    value.getClass().getSimpleName()));
                }
                continue;
            }

            uriBuilder.appendQueryParameter(key, value.toString());
        }

        return uriBuilder.toString();
    }

    final String getUrlForBatchedRequest() {
        if (overriddenURL != null) {
            throw new FacebookException("Can't override URL for a batch request");
        }

        String baseUrl = getGraphPathWithVersion();
        addCommonParameters();
        return appendParametersToBaseUrl(baseUrl);
    }

    final String getUrlForSingleRequest() {
        if (overriddenURL != null) {
            return overriddenURL.toString();
        }

        String graphBaseUrlBase;
        if (this.getHttpMethod() == HttpMethod.POST && graphPath != null && graphPath.endsWith(VIDEOS_SUFFIX)) {
            graphBaseUrlBase = ServerProtocol.getGraphVideoUrlBase();
        } else {
            graphBaseUrlBase = ServerProtocol.getGraphUrlBase();
        }
        String baseUrl = String.format("%s/%s", graphBaseUrlBase, getGraphPathWithVersion());

        addCommonParameters();
        return appendParametersToBaseUrl(baseUrl);
    }

    private String getGraphPathWithVersion() {
        Matcher matcher = versionPattern.matcher(this.graphPath);
        if (matcher.matches()) {
            return this.graphPath;
        }
        return String.format("%s/%s", this.version, this.graphPath);
    }

    private static class Attachment {
        private final Request request;
        private final Object value;

        public Attachment(Request request, Object value) {
            this.request = request;
            this.value = value;
        }

        public Request getRequest() {
            return request;
        }

        public Object getValue() {
            return value;
        }
    }

    private void serializeToBatch(JSONArray batch, Map<String, Attachment> attachments) throws JSONException, IOException {
        JSONObject batchEntry = new JSONObject();

        if (this.batchEntryName != null) {
            batchEntry.put(BATCH_ENTRY_NAME_PARAM, this.batchEntryName);
            batchEntry.put(BATCH_ENTRY_OMIT_RESPONSE_ON_SUCCESS_PARAM, this.batchEntryOmitResultOnSuccess);
        }
        if (this.batchEntryDependsOn != null) {
            batchEntry.put(BATCH_ENTRY_DEPENDS_ON_PARAM, this.batchEntryDependsOn);
        }

        String relativeURL = getUrlForBatchedRequest();
        batchEntry.put(BATCH_RELATIVE_URL_PARAM, relativeURL);
        batchEntry.put(BATCH_METHOD_PARAM, httpMethod);
        if (this.session != null) {
            String accessToken = this.session.getAccessToken();
            Logger.registerAccessToken(accessToken);
        }

        // Find all of our attachments. Remember their names and put them in the attachment map.
        ArrayList<String> attachmentNames = new ArrayList<String>();
        Set<String> keys = this.parameters.keySet();
        for (String key : keys) {
            Object value = this.parameters.get(key);
            if (isSupportedAttachmentType(value)) {
                // Make the name unique across this entire batch.
                String name = String.format("%s%d", ATTACHMENT_FILENAME_PREFIX, attachments.size());
                attachmentNames.add(name);
                attachments.put(name, new Attachment(this, value));
            }
        }

        if (!attachmentNames.isEmpty()) {
            String attachmentNamesString = TextUtils.join(",", attachmentNames);
            batchEntry.put(ATTACHED_FILES_PARAM, attachmentNamesString);
        }

        if (this.graphObject != null) {
            // Serialize the graph object into the "body" parameter.
            final ArrayList<String> keysAndValues = new ArrayList<String>();
            processGraphObject(this.graphObject, relativeURL, new KeyValueSerializer() {
                @Override
                public void writeString(String key, String value) throws IOException {
                    keysAndValues.add(String.format("%s=%s", key, URLEncoder.encode(value, "UTF-8")));
                }
            });
            String bodyValue = TextUtils.join("&", keysAndValues);
            batchEntry.put(BATCH_BODY_PARAM, bodyValue);
        }

        batch.put(batchEntry);
    }

    private static boolean hasOnProgressCallbacks(RequestBatch requests) {
        for (RequestBatch.Callback callback : requests.getCallbacks()) {
            if (callback instanceof RequestBatch.OnProgressCallback) {
                return true;
            }
        }

        for (Request request : requests) {
            if (request.getCallback() instanceof OnProgressCallback) {
                return true;
            }
        }

        return false;
    }

    final static void serializeToUrlConnection(RequestBatch requests, HttpURLConnection connection)
    throws IOException, JSONException {
        Logger logger = new Logger(LoggingBehavior.REQUESTS, "Request");

        int numRequests = requests.size();

        HttpMethod connectionHttpMethod = (numRequests == 1) ? requests.get(0).httpMethod : HttpMethod.POST;
        connection.setRequestMethod(connectionHttpMethod.name());

        URL url = connection.getURL();
        logger.append("Request:\n");
        logger.appendKeyValue("Id", requests.getId());
        logger.appendKeyValue("URL", url);
        logger.appendKeyValue("Method", connection.getRequestMethod());
        logger.appendKeyValue("User-Agent", connection.getRequestProperty("User-Agent"));
        logger.appendKeyValue("Content-Type", connection.getRequestProperty("Content-Type"));

        connection.setConnectTimeout(requests.getTimeout());
        connection.setReadTimeout(requests.getTimeout());

        // If we have a single non-POST request, don't try to serialize anything or HttpURLConnection will
        // turn it into a POST.
        boolean isPost = (connectionHttpMethod == HttpMethod.POST);
        if (!isPost) {
            logger.log();
            return;
        }

        connection.setDoOutput(true);

        OutputStream outputStream = null;
        try {
            if (hasOnProgressCallbacks(requests)) {
                ProgressNoopOutputStream countingStream = null;
                countingStream = new ProgressNoopOutputStream(requests.getCallbackHandler());
                processRequest(requests, null, numRequests, url, countingStream);

                int max = countingStream.getMaxProgress();
                Map<Request, RequestProgress> progressMap = countingStream.getProgressMap();

                BufferedOutputStream buffered = new BufferedOutputStream(connection.getOutputStream());
                outputStream = new ProgressOutputStream(buffered, requests, progressMap, max);
            }
            else {
                outputStream = new BufferedOutputStream(connection.getOutputStream());
            }

            processRequest(requests, logger, numRequests, url, outputStream);
        }
        finally {
            outputStream.close();
        }

        logger.log();
    }

    private static void processRequest(RequestBatch requests, Logger logger, int numRequests, URL url, OutputStream outputStream)
            throws IOException, JSONException
    {
        Serializer serializer = new Serializer(outputStream, logger);

        if (numRequests == 1) {
            Request request = requests.get(0);

            Map<String, Attachment> attachments = new HashMap<String, Attachment>();
            for(String key : request.parameters.keySet()) {
                Object value = request.parameters.get(key);
                if (isSupportedAttachmentType(value)) {
                    attachments.put(key, new Attachment(request, value));
                }
            }

            if (logger != null) {
                logger.append("  Parameters:\n");
            }
            serializeParameters(request.parameters, serializer, request);

            if (logger != null) {
                logger.append("  Attachments:\n");
            }
            serializeAttachments(attachments, serializer);

            if (request.graphObject != null) {
                processGraphObject(request.graphObject, url.getPath(), serializer);
            }
        } else {
            String batchAppID = getBatchAppId(requests);
            if (Utility.isNullOrEmpty(batchAppID)) {
                throw new FacebookException("At least one request in a batch must have an open Session, or a "
                        + "default app ID must be specified.");
            }

            serializer.writeString(BATCH_APP_ID_PARAM, batchAppID);

            // We write out all the requests as JSON, remembering which file attachments they have, then
            // write out the attachments.
            Map<String, Attachment> attachments = new HashMap<String, Attachment>();
            serializeRequestsAsJSON(serializer, requests, attachments);

            if (logger != null) {
                logger.append("  Attachments:\n");
            }
            serializeAttachments(attachments, serializer);
        }
    }

    private static boolean isMeRequest(String path) {
        Matcher matcher = versionPattern.matcher(path);
        if (matcher.matches()) {
            // Group 1 contains the path aside from version
            path = matcher.group(1);
        }
        if (path.startsWith("me/") || path.startsWith("/me/")) {
            return true;
        }
        return false;
    }

    private static void processGraphObject(GraphObject graphObject, String path, KeyValueSerializer serializer)
            throws IOException {
        // In general, graph objects are passed by reference (ID/URL). But if this is an OG Action,
        // we need to pass the entire values of the contents of the 'image' property, as they
        // contain important metadata beyond just a URL. We don't have a 100% foolproof way of knowing
        // if we are posting an OG Action, given that batched requests can have parameter substitution,
        // but passing the OG Action type as a substituted parameter is unlikely.
        // It looks like an OG Action if it's posted to me/namespace:action[?other=stuff].
        boolean isOGAction = false;
        if (isMeRequest(path)) {
            int colonLocation = path.indexOf(":");
            int questionMarkLocation = path.indexOf("?");
            isOGAction = colonLocation > 3 && (questionMarkLocation == -1 || colonLocation < questionMarkLocation);
        }

        Set<Entry<String, Object>> entries = graphObject.asMap().entrySet();
        for (Entry<String, Object> entry : entries) {
            boolean passByValue = isOGAction && entry.getKey().equalsIgnoreCase("image");
            processGraphObjectProperty(entry.getKey(), entry.getValue(), serializer, passByValue);
        }
    }

    private static void processGraphObjectProperty(String key, Object value, KeyValueSerializer serializer,
            boolean passByValue) throws IOException {
        Class<?> valueClass = value.getClass();
        if (GraphObject.class.isAssignableFrom(valueClass)) {
            value = ((GraphObject) value).getInnerJSONObject();
            valueClass = value.getClass();
        } else if (GraphObjectList.class.isAssignableFrom(valueClass)) {
            value = ((GraphObjectList<?>) value).getInnerJSONArray();
            valueClass = value.getClass();
        }

        if (JSONObject.class.isAssignableFrom(valueClass)) {
            JSONObject jsonObject = (JSONObject) value;
            if (passByValue) {
                // We need to pass all properties of this object in key[propertyName] format.
                @SuppressWarnings("unchecked")
                Iterator<String> keys = jsonObject.keys();
                while (keys.hasNext()) {
                    String propertyName = keys.next();
                    String subKey = String.format("%s[%s]", key, propertyName);
                    processGraphObjectProperty(subKey, jsonObject.opt(propertyName), serializer, passByValue);
                }
            } else {
                // Normal case is passing objects by reference, so just pass the ID or URL, if any, as the value
                // for "key"
                if (jsonObject.has("id")) {
                    processGraphObjectProperty(key, jsonObject.optString("id"), serializer, passByValue);
                } else if (jsonObject.has("url")) {
                    processGraphObjectProperty(key, jsonObject.optString("url"), serializer, passByValue);
                } else if (jsonObject.has(NativeProtocol.OPEN_GRAPH_CREATE_OBJECT_KEY)) {
                    processGraphObjectProperty(key, jsonObject.toString(), serializer, passByValue);
                }
            }
        } else if (JSONArray.class.isAssignableFrom(valueClass)) {
            JSONArray jsonArray = (JSONArray) value;
            int length = jsonArray.length();
            for (int i = 0; i < length; ++i) {
                String subKey = String.format("%s[%d]", key, i);
                processGraphObjectProperty(subKey, jsonArray.opt(i), serializer, passByValue);
            }
        } else if (String.class.isAssignableFrom(valueClass) ||
                Number.class.isAssignableFrom(valueClass) ||
                Boolean.class.isAssignableFrom(valueClass)) {
            serializer.writeString(key, value.toString());
        } else if (Date.class.isAssignableFrom(valueClass)) {
            Date date = (Date) value;
            // The "Events Timezone" platform migration affects what date/time formats Facebook accepts and returns.
            // Apps created after 8/1/12 (or apps that have explicitly enabled the migration) should send/receive
            // dates in ISO-8601 format. Pre-migration apps can send as Unix timestamps. Since the future is ISO-8601,
            // that is what we support here. Apps that need pre-migration behavior can explicitly send these as
            // integer timestamps rather than Dates.
            final SimpleDateFormat iso8601DateFormat = new SimpleDateFormat(ISO_8601_FORMAT_STRING, Locale.US);
            serializer.writeString(key, iso8601DateFormat.format(date));
        }
    }

    private static void serializeParameters(Bundle bundle, Serializer serializer, Request request) throws IOException {
        Set<String> keys = bundle.keySet();

        for (String key : keys) {
            Object value = bundle.get(key);
            if (isSupportedParameterType(value)) {
                serializer.writeObject(key, value, request);
            }
        }
    }

    private static void serializeAttachments(Map<String, Attachment> attachments, Serializer serializer) throws IOException {
        Set<String> keys = attachments.keySet();

        for (String key : keys) {
            Attachment attachment = attachments.get(key);
            if (isSupportedAttachmentType(attachment.getValue())) {
                serializer.writeObject(key, attachment.getValue(), attachment.getRequest());
            }
        }
    }

    private static void serializeRequestsAsJSON(Serializer serializer, Collection<Request> requests, Map<String, Attachment> attachments)
            throws JSONException, IOException {
        JSONArray batch = new JSONArray();
        for (Request request : requests) {
            request.serializeToBatch(batch, attachments);
        }

        serializer.writeRequestsAsJson(BATCH_PARAM, batch, requests);
    }

    private static String getMimeContentType() {
        return String.format("multipart/form-data; boundary=%s", MIME_BOUNDARY);
    }

    private static volatile String userAgent;

    private static String getUserAgent() {
        if (userAgent == null) {
            userAgent = String.format("%s.%s", USER_AGENT_BASE, FacebookSdkVersion.BUILD);
        }

        return userAgent;
    }

    private static String getBatchAppId(RequestBatch batch) {
        if (!Utility.isNullOrEmpty(batch.getBatchApplicationId())) {
            return batch.getBatchApplicationId();
        }

        for (Request request : batch) {
            Session session = request.session;
            if (session != null) {
                return session.getApplicationId();
            }
        }
        return Request.defaultBatchApplicationId;
    }

    private static <T extends GraphObject> List<T> typedListFromResponse(Response response, Class<T> clazz) {
        GraphMultiResult multiResult = response.getGraphObjectAs(GraphMultiResult.class);
        if (multiResult == null) {
            return null;
        }

        GraphObjectList<GraphObject> data = multiResult.getData();
        if (data == null) {
            return null;
        }

        return data.castToListOf(clazz);
    }

    private static boolean isSupportedAttachmentType(Object value) {
        return value instanceof Bitmap || value instanceof byte[] || value instanceof ParcelFileDescriptor ||
                value instanceof ParcelFileDescriptorWithMimeType;
    }

    private static boolean isSupportedParameterType(Object value) {
        return value instanceof String || value instanceof Boolean || value instanceof Number ||
                value instanceof Date;
    }

    private static String parameterToString(Object value) {
        if (value instanceof String) {
            return (String) value;
        } else if (value instanceof Boolean || value instanceof Number) {
            return value.toString();
        } else if (value instanceof Date) {
            final SimpleDateFormat iso8601DateFormat = new SimpleDateFormat(ISO_8601_FORMAT_STRING, Locale.US);
            return iso8601DateFormat.format(value);
        }
        throw new IllegalArgumentException("Unsupported parameter type.");
    }

    private interface KeyValueSerializer {
        void writeString(String key, String value) throws IOException;
    }

    private static class Serializer implements KeyValueSerializer {
        private final OutputStream outputStream;
        private final Logger logger;
        private boolean firstWrite = true;

        public Serializer(OutputStream outputStream, Logger logger) {
            this.outputStream = outputStream;
            this.logger = logger;
        }

        public void writeObject(String key, Object value, Request request) throws IOException {
            if (outputStream instanceof RequestOutputStream) {
                ((RequestOutputStream) outputStream).setCurrentRequest(request);
            }

            if (isSupportedParameterType(value)) {
                writeString(key, parameterToString(value));
            } else if (value instanceof Bitmap) {
                writeBitmap(key, (Bitmap) value);
            } else if (value instanceof byte[]) {
                writeBytes(key, (byte[]) value);
            } else if (value instanceof ParcelFileDescriptor) {
                writeFile(key, (ParcelFileDescriptor) value, null);
            } else if (value instanceof ParcelFileDescriptorWithMimeType) {
                writeFile(key, (ParcelFileDescriptorWithMimeType) value);
            } else {
                throw new IllegalArgumentException("value is not a supported type: String, Bitmap, byte[]");
            }
        }

        public void writeRequestsAsJson(String key, JSONArray requestJsonArray, Collection<Request> requests)
                throws IOException, JSONException {
            if (! (outputStream instanceof RequestOutputStream)) {
                writeString(key, requestJsonArray.toString());
                return;
            }

            RequestOutputStream requestOutputStream = (RequestOutputStream) outputStream;
            writeContentDisposition(key, null, null);
            write("[");
            int i = 0;
            for (Request request : requests) {
                JSONObject requestJson = requestJsonArray.getJSONObject(i);
                requestOutputStream.setCurrentRequest(request);
                if (i > 0) {
                    write(",%s", requestJson.toString());
                } else {
                    write("%s", requestJson.toString());
                }
                i++;
            }
            write("]");
            if (logger != null) {
                logger.appendKeyValue("    " + key, requestJsonArray.toString());
            }
        }

        public void writeString(String key, String value) throws IOException {
            writeContentDisposition(key, null, null);
            writeLine("%s", value);
            writeRecordBoundary();
            if (logger != null) {
                logger.appendKeyValue("    " + key, value);
            }
        }

        public void writeBitmap(String key, Bitmap bitmap) throws IOException {
            writeContentDisposition(key, key, "image/png");
            // Note: quality parameter is ignored for PNG
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, outputStream);
            writeLine("");
            writeRecordBoundary();
            if (logger != null) {
                logger.appendKeyValue("    " + key, "<Image>");
            }
        }

        public void writeBytes(String key, byte[] bytes) throws IOException {
            writeContentDisposition(key, key, "content/unknown");
            this.outputStream.write(bytes);
            writeLine("");
            writeRecordBoundary();
            if (logger != null) {
                logger.appendKeyValue("    " + key, String.format("<Data: %d>", bytes.length));
            }
        }

        public void writeFile(String key, ParcelFileDescriptorWithMimeType descriptorWithMimeType) throws IOException {
            writeFile(key, descriptorWithMimeType.getFileDescriptor(), descriptorWithMimeType.getMimeType());
        }

        public void writeFile(String key, ParcelFileDescriptor descriptor, String mimeType) throws IOException {
            if (mimeType == null) {
                mimeType = "content/unknown";
            }
            writeContentDisposition(key, key, mimeType);

            int totalBytes = 0;

            if (outputStream instanceof ProgressNoopOutputStream) {
                // If we are only counting bytes then skip reading the file
                ((ProgressNoopOutputStream) outputStream).addProgress(descriptor.getStatSize());
            }
            else {
                ParcelFileDescriptor.AutoCloseInputStream inputStream = null;
                BufferedInputStream bufferedInputStream = null;
                try {
                    inputStream = new ParcelFileDescriptor.AutoCloseInputStream(descriptor);
                    bufferedInputStream = new BufferedInputStream(inputStream);

                    byte[] buffer = new byte[8192];
                    int bytesRead;
                    while ((bytesRead = bufferedInputStream.read(buffer)) != -1) {
                        this.outputStream.write(buffer, 0, bytesRead);
                        totalBytes += bytesRead;
                    }
                } finally {
                    if (bufferedInputStream != null) {
                        bufferedInputStream.close();
                    }
                    if (inputStream != null) {
                        inputStream.close();
                    }
                }
            }
            writeLine("");
            writeRecordBoundary();
            if (logger != null) {
                logger.appendKeyValue("    " + key, String.format("<Data: %d>", totalBytes));
            }
        }

        public void writeRecordBoundary() throws IOException {
            writeLine("--%s", MIME_BOUNDARY);
        }

        public void writeContentDisposition(String name, String filename, String contentType) throws IOException {
            write("Content-Disposition: form-data; name=\"%s\"", name);
            if (filename != null) {
                write("; filename=\"%s\"", filename);
            }
            writeLine(""); // newline after Content-Disposition
            if (contentType != null) {
                writeLine("%s: %s", CONTENT_TYPE_HEADER, contentType);
            }
            writeLine(""); // blank line before content
        }

        public void write(String format, Object... args) throws IOException {
            if (firstWrite) {
                // Prepend all of our output with a boundary string.
                this.outputStream.write("--".getBytes());
                this.outputStream.write(MIME_BOUNDARY.getBytes());
                this.outputStream.write("\r\n".getBytes());
                firstWrite = false;
            }
            this.outputStream.write(String.format(format, args).getBytes());
        }

        public void writeLine(String format, Object... args) throws IOException {
            write(format, args);
            write("\r\n");
        }

    }

    /**
     * Specifies the interface that consumers of the Request class can implement in order to be notified when a
     * particular request completes, either successfully or with an error.
     */
    public interface Callback {
        /**
         * The method that will be called when a request completes.
         *
         * @param response
         *            the Response of this request, which may include error information if the request was unsuccessful
         */
        void onCompleted(Response response);
    }

    /**
     * Specifies the interface that consumers of the Request class can implement in order to be notified when a
     * progress is made on a particular request. The frequency of the callbacks can be controlled using
     * {@link com.facebook.Settings#setOnProgressThreshold(long)}
     */
    public interface OnProgressCallback extends Callback {
        /**
         * The method that will be called when progress is made.
         *
         * @param current
         *            the current value of the progress of the request.
         * @param max
         *            the maximum value (target) value that the progress will have.
         */
        void onProgress(long current, long max);
    }

    /**
     * Specifies the interface that consumers of
     * {@link Request#executeMeRequestAsync(Session, com.facebook.Request.GraphUserCallback)}
     * can use to be notified when the request completes, either successfully or with an error.
     */
    public interface GraphUserCallback {
        /**
         * The method that will be called when the request completes.
         *
         * @param user     the GraphObject representing the returned user, or null
         * @param response the Response of this request, which may include error information if the request was unsuccessful
         */
        void onCompleted(GraphUser user, Response response);
    }

    /**
     * Specifies the interface that consumers of
     * {@link Request#executeMyFriendsRequestAsync(Session, com.facebook.Request.GraphUserListCallback)}
     * can use to be notified when the request completes, either successfully or with an error.
     */
    public interface GraphUserListCallback {
        /**
         * The method that will be called when the request completes.
         *
         * @param users    the list of GraphObjects representing the returned friends, or null
         * @param response the Response of this request, which may include error information if the request was unsuccessful
         */
        void onCompleted(List<GraphUser> users, Response response);
    }

    /**
     * Specifies the interface that consumers of
     * {@link Request#executePlacesSearchRequestAsync(Session, android.location.Location, int, int, String, com.facebook.Request.GraphPlaceListCallback)}
     * can use to be notified when the request completes, either successfully or with an error.
     */
    public interface GraphPlaceListCallback {
        /**
         * The method that will be called when the request completes.
         *
         * @param places   the list of GraphObjects representing the returned places, or null
         * @param response the Response of this request, which may include error information if the request was unsuccessful
         */
        void onCompleted(List<GraphPlace> places, Response response);
    }

    private static class ParcelFileDescriptorWithMimeType implements Parcelable {
        private final String mimeType;
        private final ParcelFileDescriptor fileDescriptor;

        public String getMimeType() {
            return mimeType;
        }

        public ParcelFileDescriptor getFileDescriptor() {
            return fileDescriptor;
        }

        public int describeContents() {
            return CONTENTS_FILE_DESCRIPTOR;
        }

        public void writeToParcel(Parcel out, int flags) {
            out.writeString(mimeType);
            out.writeFileDescriptor(fileDescriptor.getFileDescriptor());
        }

        @SuppressWarnings("unused")
        public static final Parcelable.Creator<ParcelFileDescriptorWithMimeType> CREATOR
                = new Parcelable.Creator<ParcelFileDescriptorWithMimeType>() {
            public ParcelFileDescriptorWithMimeType createFromParcel(Parcel in) {
                return new ParcelFileDescriptorWithMimeType(in);
            }

            public ParcelFileDescriptorWithMimeType[] newArray(int size) {
                return new ParcelFileDescriptorWithMimeType[size];
            }
        };

        public ParcelFileDescriptorWithMimeType(ParcelFileDescriptor fileDescriptor, String mimeType) {
            this.mimeType = mimeType;
            this.fileDescriptor = fileDescriptor;
        }

        private ParcelFileDescriptorWithMimeType(Parcel in) {
            mimeType = in.readString();
            fileDescriptor = in.readFileDescriptor();
        }
    }
}
