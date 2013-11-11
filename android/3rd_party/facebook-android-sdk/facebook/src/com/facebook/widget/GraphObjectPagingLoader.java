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

package com.facebook.widget;

import android.content.Context;
import android.os.Handler;
import android.support.v4.content.Loader;
import com.facebook.*;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphObjectList;
import com.facebook.internal.CacheableRequestBatch;

class GraphObjectPagingLoader<T extends GraphObject> extends Loader<SimpleGraphObjectCursor<T>> {
    private final Class<T> graphObjectClass;
    private boolean skipRoundtripIfCached;
    private Request originalRequest;
    private Request currentRequest;
    private Request nextRequest;
    private OnErrorListener onErrorListener;
    private SimpleGraphObjectCursor<T> cursor;
    private boolean appendResults = false;
    private boolean loading = false;

    public interface OnErrorListener {
        public void onError(FacebookException error, GraphObjectPagingLoader<?> loader);
    }

    public GraphObjectPagingLoader(Context context, Class<T> graphObjectClass) {
        super(context);

        this.graphObjectClass = graphObjectClass;
    }

    public OnErrorListener getOnErrorListener() {
        return onErrorListener;
    }

    public void setOnErrorListener(OnErrorListener listener) {
        this.onErrorListener = listener;
    }

    public SimpleGraphObjectCursor<T> getCursor() {
        return cursor;
    }

    public void clearResults() {
        nextRequest = null;
        originalRequest = null;
        currentRequest = null;

        deliverResult(null);
    }

    public boolean isLoading() {
        return loading;
    }

    public void startLoading(Request request, boolean skipRoundtripIfCached) {
        originalRequest = request;
        startLoading(request, skipRoundtripIfCached, 0);
    }

    public void refreshOriginalRequest(long afterDelay) {
        if (originalRequest == null) {
            throw new FacebookException(
                    "refreshOriginalRequest may not be called until after startLoading has been called.");
        }
        startLoading(originalRequest, false, afterDelay);
    }

    public void followNextLink() {
        if (nextRequest != null) {
            appendResults = true;
            currentRequest = nextRequest;

            currentRequest.setCallback(new Request.Callback() {
                @Override
                public void onCompleted(Response response) {
                    requestCompleted(response);
                }
            });

            loading = true;
            CacheableRequestBatch batch = putRequestIntoBatch(currentRequest, skipRoundtripIfCached);
            Request.executeBatchAsync(batch);
        }
    }

    @Override
    public void deliverResult(SimpleGraphObjectCursor<T> cursor) {
        SimpleGraphObjectCursor<T> oldCursor = this.cursor;
        this.cursor = cursor;

        if (isStarted()) {
            super.deliverResult(cursor);

            if (oldCursor != null && oldCursor != cursor && !oldCursor.isClosed()) {
                oldCursor.close();
            }
        }
    }

    @Override
    protected void onStartLoading() {
        super.onStartLoading();

        if (cursor != null) {
            deliverResult(cursor);
        }
    }

    private void startLoading(Request request, boolean skipRoundtripIfCached, long afterDelay) {
        this.skipRoundtripIfCached = skipRoundtripIfCached;
        appendResults = false;
        nextRequest = null;
        currentRequest = request;
        currentRequest.setCallback(new Request.Callback() {
            @Override
            public void onCompleted(Response response) {
                requestCompleted(response);
            }
        });

        // We are considered loading even if we have a delay.
        loading = true;

        final RequestBatch batch = putRequestIntoBatch(request, skipRoundtripIfCached);
        Runnable r = new Runnable() {
            @Override
            public void run() {
                Request.executeBatchAsync(batch);
            }
        };
        if (afterDelay == 0) {
            r.run();
        } else {
            Handler handler = new Handler();
            handler.postDelayed(r, afterDelay);
        }
    }

    private CacheableRequestBatch putRequestIntoBatch(Request request, boolean skipRoundtripIfCached) {
        // We just use the request URL as the cache key.
        CacheableRequestBatch batch = new CacheableRequestBatch(request);
        // We use the default cache key (request URL).
        batch.setForceRoundTrip(!skipRoundtripIfCached);
        return batch;
    }

    private void requestCompleted(Response response) {
        Request request = response.getRequest();
        if (request != currentRequest) {
            return;
        }

        loading = false;
        currentRequest = null;

        FacebookRequestError requestError = response.getError();
        FacebookException exception = (requestError == null) ? null : requestError.getException();
        if (response.getGraphObject() == null && exception == null) {
            exception = new FacebookException("GraphObjectPagingLoader received neither a result nor an error.");
        }

        if (exception != null) {
            nextRequest = null;

            if (onErrorListener != null) {
                onErrorListener.onError(exception, this);
            }
        } else {
            addResults(response);
        }
    }

    private void addResults(Response response) {
        SimpleGraphObjectCursor<T> cursorToModify = (cursor == null || !appendResults) ? new SimpleGraphObjectCursor<T>() :
                new SimpleGraphObjectCursor<T>(cursor);

        PagedResults result = response.getGraphObjectAs(PagedResults.class);
        boolean fromCache = response.getIsFromCache();

        GraphObjectList<T> data = result.getData().castToListOf(graphObjectClass);
        boolean haveData = data.size() > 0;

        if (haveData) {
            nextRequest = response.getRequestForPagedResults(Response.PagingDirection.NEXT);

            cursorToModify.addGraphObjects(data, fromCache);
            cursorToModify.setMoreObjectsAvailable(true);
        }

        if (!haveData) {
            cursorToModify.setMoreObjectsAvailable(false);
            cursorToModify.setFromCache(fromCache);

            nextRequest = null;
        }

        // Once we get any set of results NOT from the cache, stop trying to get any future ones
        // from it.
        if (!fromCache) {
            skipRoundtripIfCached = false;
        }

        deliverResult(cursorToModify);
    }

    interface PagedResults extends GraphObject {
        GraphObjectList<GraphObject> getData();
    }
}
