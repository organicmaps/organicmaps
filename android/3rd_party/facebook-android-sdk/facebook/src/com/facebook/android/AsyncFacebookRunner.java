/**
 * Copyright 2010-present Facebook
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

package com.facebook.android;

import android.content.Context;
import android.os.Bundle;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.MalformedURLException;

/**
 * A sample implementation of asynchronous API requests. This class provides
 * the ability to execute API methods and have the call return immediately,
 * without blocking the calling thread. This is necessary when accessing the
 * API in the UI thread, for instance. The request response is returned to 
 * the caller via a callback interface, which the developer must implement.
 *
 * This sample implementation simply spawns a new thread for each request,
 * and makes the API call immediately.  This may work in many applications,
 * but more sophisticated users may re-implement this behavior using a thread
 * pool, a network thread, a request queue, or other mechanism.  Advanced
 * functionality could be built, such as rate-limiting of requests, as per
 * a specific application's needs.
 *
 * @deprecated
 *
 * @see RequestListener
 *        The callback interface.
 *
 * @author  Jim Brusstar (jimbru@fb.com),
 *          Yariv Sadan (yariv@fb.com),
 *          Luke Shepard (lshepard@fb.com)
 */
@Deprecated
public class AsyncFacebookRunner {

    Facebook fb;

    public AsyncFacebookRunner(Facebook fb) {
        this.fb = fb;
    }

    /**
     * Invalidate the current user session by removing the access token in
     * memory, clearing the browser cookies, and calling auth.expireSession
     * through the API. The application will be notified when logout is
     * complete via the callback interface.
     * <p/>
     * Note that this method is asynchronous and the callback will be invoked
     * in a background thread; operations that affect the UI will need to be
     * posted to the UI thread or an appropriate handler.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link com.facebook.Session} for more info.
     *
     * @param context
     *            The Android context in which the logout should be called: it
     *            should be the same context in which the login occurred in
     *            order to clear any stored cookies
     * @param listener
     *            Callback interface to notify the application when the request
     *            has completed.
     * @param state
     *            An arbitrary object used to identify the request when it
     *            returns to the callback. This has no effect on the request
     *            itself.
     */
    @Deprecated
    public void logout(final Context context,
                       final RequestListener listener,
                       final Object state) {
        new Thread() {
            @Override public void run() {
                try {
                    String response = fb.logoutImpl(context);
                    if (response.length() == 0 || response.equals("false")){
                        listener.onFacebookError(new FacebookError(
                                "auth.expireSession failed"), state);
                        return;
                    }
                    listener.onComplete(response, state);
                } catch (FileNotFoundException e) {
                    listener.onFileNotFoundException(e, state);
                } catch (MalformedURLException e) {
                    listener.onMalformedURLException(e, state);
                } catch (IOException e) {
                    listener.onIOException(e, state);
                }
            }
        }.start();
    }

    @Deprecated
    public void logout(final Context context, final RequestListener listener) {
        logout(context, listener, /* state */ null);
    }

    /**
     * Make a request to Facebook's old (pre-graph) API with the given
     * parameters. One of the parameter keys must be "method" and its value
     * should be a valid REST server API method.
     * <p/>
     * See http://developers.facebook.com/docs/reference/rest/
     * <p/>
     * Note that this method is asynchronous and the callback will be invoked
     * in a background thread; operations that affect the UI will need to be
     * posted to the UI thread or an appropriate handler.
     * <p/>
     * Example:
     * <code>
     *  Bundle parameters = new Bundle();
     *  parameters.putString("method", "auth.expireSession", new Listener());
     *  String response = request(parameters);
     * </code>
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link com.facebook.Request} for more info.
     *
     * @param parameters
     *            Key-value pairs of parameters to the request. Refer to the
     *            documentation: one of the parameters must be "method".
     * @param listener
     *            Callback interface to notify the application when the request
     *            has completed.
     * @param state
     *            An arbitrary object used to identify the request when it
     *            returns to the callback. This has no effect on the request
     *            itself.
     */
    @Deprecated
    public void request(Bundle parameters,
                        RequestListener listener,
                        final Object state) {
        request(null, parameters, "GET", listener, state);
    }

    @Deprecated
    public void request(Bundle parameters, RequestListener listener) {
        request(null, parameters, "GET", listener, /* state */ null);
    }

    /**
     * Make a request to the Facebook Graph API without any parameters.
     * <p/>
     * See http://developers.facebook.com/docs/api
     * <p/>
     * Note that this method is asynchronous and the callback will be invoked
     * in a background thread; operations that affect the UI will need to be
     * posted to the UI thread or an appropriate handler.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link com.facebook.Request} for more info.
     *
     * @param graphPath
     *            Path to resource in the Facebook graph, e.g., to fetch data
     *            about the currently logged authenticated user, provide "me",
     *            which will fetch http://graph.facebook.com/me
     * @param listener
     *            Callback interface to notify the application when the request
     *            has completed.
     * @param state
     *            An arbitrary object used to identify the request when it
     *            returns to the callback. This has no effect on the request
     *            itself.
     */
    @Deprecated
    public void request(String graphPath,
                        RequestListener listener,
                        final Object state) {
        request(graphPath, new Bundle(), "GET", listener, state);
    }

    @Deprecated
    public void request(String graphPath, RequestListener listener) {
        request(graphPath, new Bundle(), "GET", listener, /* state */ null);
    }

    /**
     * Make a request to the Facebook Graph API with the given string parameters
     * using an HTTP GET (default method).
     * <p/>
     * See http://developers.facebook.com/docs/api
     * <p/>
     * Note that this method is asynchronous and the callback will be invoked
     * in a background thread; operations that affect the UI will need to be
     * posted to the UI thread or an appropriate handler.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link com.facebook.Request} for more info.
     *
     * @param graphPath
     *            Path to resource in the Facebook graph, e.g., to fetch data
     *            about the currently logged authenticated user, provide "me",
     *            which will fetch http://graph.facebook.com/me
     * @param parameters
     *            key-value string parameters, e.g. the path "search" with
     *            parameters "q" : "facebook" would produce a query for the
     *            following graph resource:
     *            https://graph.facebook.com/search?q=facebook
     * @param listener
     *            Callback interface to notify the application when the request
     *            has completed.
     * @param state
     *            An arbitrary object used to identify the request when it
     *            returns to the callback. This has no effect on the request
     *            itself.
     */
    @Deprecated
    public void request(String graphPath,
                        Bundle parameters,
                        RequestListener listener,
                        final Object state) {
        request(graphPath, parameters, "GET", listener, state);
    }

    @Deprecated
    public void request(String graphPath,
                        Bundle parameters,
                        RequestListener listener) {
        request(graphPath, parameters, "GET", listener, /* state */ null);
    }

    /**
     * Make a request to the Facebook Graph API with the given HTTP method and
     * string parameters. Note that binary data parameters (e.g. pictures) are
     * not yet supported by this helper function.
     * <p/>
     * See http://developers.facebook.com/docs/api
     * <p/>
     * Note that this method is asynchronous and the callback will be invoked
     * in a background thread; operations that affect the UI will need to be
     * posted to the UI thread or an appropriate handler.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link com.facebook.Request} for more info.
     *
     * @param graphPath
     *            Path to resource in the Facebook graph, e.g., to fetch data
     *            about the currently logged authenticated user, provide "me",
     *            which will fetch http://graph.facebook.com/me
     * @param parameters
     *            key-value string parameters, e.g. the path "search" with
     *            parameters {"q" : "facebook"} would produce a query for the
     *            following graph resource:
     *            https://graph.facebook.com/search?q=facebook
     * @param httpMethod
     *            http verb, e.g. "POST", "DELETE"
     * @param listener
     *            Callback interface to notify the application when the request
     *            has completed.
     * @param state
     *            An arbitrary object used to identify the request when it
     *            returns to the callback. This has no effect on the request
     *            itself.
     */
    @Deprecated
    public void request(final String graphPath,
                        final Bundle parameters,
                        final String httpMethod,
                        final RequestListener listener,
                        final Object state) {
        new Thread() {
            @Override public void run() {
                try {
                    String resp = fb.requestImpl(graphPath, parameters, httpMethod);
                    listener.onComplete(resp, state);
                } catch (FileNotFoundException e) {
                    listener.onFileNotFoundException(e, state);
                } catch (MalformedURLException e) {
                    listener.onMalformedURLException(e, state);
                } catch (IOException e) {
                    listener.onIOException(e, state);
                }
            }
        }.start();
    }

    /**
     * Callback interface for API requests.
     * <p/>
     * Each method includes a 'state' parameter that identifies the calling
     * request. It will be set to the value passed when originally calling the
     * request method, or null if none was passed.
     * <p/>
     * This interface is deprecated.  See {@link Facebook} and {@link com.facebook.Request} for more info.
     */
    @Deprecated
    public static interface RequestListener {

        /**
         * Called when a request completes with the given response.
         *
         * Executed by a background thread: do not update the UI in this method.
         */
        public void onComplete(String response, Object state);

        /**
         * Called when a request has a network or request error.
         *
         * Executed by a background thread: do not update the UI in this method.
         */
        public void onIOException(IOException e, Object state);

        /**
         * Called when a request fails because the requested resource is
         * invalid or does not exist.
         *
         * Executed by a background thread: do not update the UI in this method.
         */
        public void onFileNotFoundException(FileNotFoundException e,
                                            Object state);

        /**
         * Called if an invalid graph path is provided (which may result in a
         * malformed URL).
         *
         * Executed by a background thread: do not update the UI in this method.
         */
        public void onMalformedURLException(MalformedURLException e,
                                            Object state);

        /**
         * Called when the server-side Facebook method fails.
         *
         * Executed by a background thread: do not update the UI in this method.
         */
        public void onFacebookError(FacebookError e, Object state);

    }

}
