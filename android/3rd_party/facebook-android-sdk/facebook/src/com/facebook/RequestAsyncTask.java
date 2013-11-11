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

import android.annotation.TargetApi;
import android.os.AsyncTask;
import android.os.Handler;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.Executor;

/**
 * Defines an AsyncTask suitable for executing a Request in the background. May be subclassed
 * by applications having unique threading model needs.
 */
@TargetApi(3)
public class RequestAsyncTask extends AsyncTask<Void, Void, List<Response>> {
    private static final String TAG = RequestAsyncTask.class.getCanonicalName();
    private static Method executeOnExecutorMethod;

    private final HttpURLConnection connection;
    private final RequestBatch requests;

    private Exception exception;

    static {
        for (Method method : AsyncTask.class.getMethods()) {
            if ("executeOnExecutor".equals(method.getName())) {
                Class<?>[] parameters = method.getParameterTypes();
                if ((parameters.length == 2) && (parameters[0] == Executor.class) && parameters[1].isArray()) {
                    executeOnExecutorMethod = method;
                    break;
                }
            }
        }
    }

    /**
     * Constructor. Serialization of the requests will be done in the background, so any serialization-
     * related errors will be returned via the Response.getException() method.
     *
     * @param requests the requests to execute
     */
    public RequestAsyncTask(Request... requests) {
        this(null, new RequestBatch(requests));
    }

    /**
     * Constructor. Serialization of the requests will be done in the background, so any serialization-
     * related errors will be returned via the Response.getException() method.
     *
     * @param requests the requests to execute
     */
    public RequestAsyncTask(Collection<Request> requests) {
        this(null, new RequestBatch(requests));
    }

    /**
     * Constructor. Serialization of the requests will be done in the background, so any serialization-
     * related errors will be returned via the Response.getException() method.
     *
     * @param requests the requests to execute
     */
    public RequestAsyncTask(RequestBatch requests) {
        this(null, requests);
    }

    /**
     * Constructor that allows specification of an HTTP connection to use for executing
     * the requests. No validation is done that the contents of the connection actually
     * reflect the serialized requests, so it is the caller's responsibility to ensure
     * that it will correctly generate the desired responses.
     *
     * @param connection the HTTP connection to use to execute the requests
     * @param requests   the requests to execute
     */
    public RequestAsyncTask(HttpURLConnection connection, Request... requests) {
        this(connection, new RequestBatch(requests));
    }

    /**
     * Constructor that allows specification of an HTTP connection to use for executing
     * the requests. No validation is done that the contents of the connection actually
     * reflect the serialized requests, so it is the caller's responsibility to ensure
     * that it will correctly generate the desired responses.
     *
     * @param connection the HTTP connection to use to execute the requests
     * @param requests   the requests to execute
     */
    public RequestAsyncTask(HttpURLConnection connection, Collection<Request> requests) {
        this(connection, new RequestBatch(requests));
    }

    /**
     * Constructor that allows specification of an HTTP connection to use for executing
     * the requests. No validation is done that the contents of the connection actually
     * reflect the serialized requests, so it is the caller's responsibility to ensure
     * that it will correctly generate the desired responses.
     *
     * @param connection the HTTP connection to use to execute the requests
     * @param requests   the requests to execute
     */
    public RequestAsyncTask(HttpURLConnection connection, RequestBatch requests) {
        this.requests = requests;
        this.connection = connection;
    }

    protected final Exception getException() {
        return exception;
    }

    protected final RequestBatch getRequests() {
        return requests;
    }

    @Override
    public String toString() {
        return new StringBuilder().append("{RequestAsyncTask: ").append(" connection: ").append(connection)
                .append(", requests: ").append(requests).append("}").toString();
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();

        if (requests.getCallbackHandler() == null) {
            // We want any callbacks to go to a handler on this thread unless a handler has already been specified.
            requests.setCallbackHandler(new Handler());
        }
    }

    @Override
    protected void onPostExecute(List<Response> result) {
        super.onPostExecute(result);

        if (exception != null) {
            Log.d(TAG, String.format("onPostExecute: exception encountered during request: %s", exception.getMessage()));
        }
    }

    @Override
    protected List<Response> doInBackground(Void... params) {
        try {
            if (connection == null) {
                return requests.executeAndWait();
            } else {
                return Request.executeConnectionAndWait(connection, requests);
            }
        } catch (Exception e) {
            exception = e;
            return null;
        }
    }

    RequestAsyncTask executeOnSettingsExecutor() {
        try {
            if (executeOnExecutorMethod != null) {
                executeOnExecutorMethod.invoke(this, Settings.getExecutor(), null);
                return this;
            }
        } catch (InvocationTargetException e) {
            // fall-through
        } catch (IllegalAccessException e) {
            // fall-through
        }

        this.execute();
        return this;
    }
}
