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

package com.facebook.internal;

import android.content.Context;
import android.util.Log;
import com.facebook.LoggingBehavior;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URI;
import java.net.URISyntaxException;

class UrlRedirectCache {
    static final String TAG = UrlRedirectCache.class.getSimpleName();
    private static final String REDIRECT_CONTENT_TAG = TAG + "_Redirect";

    private volatile static FileLruCache urlRedirectCache;

    synchronized static FileLruCache getCache(Context context) throws IOException{
        if (urlRedirectCache == null) {
            urlRedirectCache = new FileLruCache(context.getApplicationContext(), TAG, new FileLruCache.Limits());
        }
        return urlRedirectCache;
    }

    static URI getRedirectedUri(Context context, URI uri) {
        if (uri == null) {
            return null;
        }

        String uriString = uri.toString();
        InputStreamReader reader = null;
        try {
            InputStream stream;
            FileLruCache cache = getCache(context);
            boolean redirectExists = false;
            while ((stream = cache.get(uriString, REDIRECT_CONTENT_TAG)) != null) {
                redirectExists = true;

                // Get the redirected url
                reader = new InputStreamReader(stream);
                char[] buffer = new char[128];
                int bufferLength;
                StringBuilder urlBuilder = new StringBuilder();
                while ((bufferLength = reader.read(buffer, 0, buffer.length)) > 0) {
                    urlBuilder.append(buffer, 0, bufferLength);
                }
                Utility.closeQuietly(reader);

                // Iterate to the next url in the redirection
                uriString = urlBuilder.toString();
            }

            if (redirectExists) {
                return new URI(uriString);
            }
        } catch (URISyntaxException e) {
            // caching is best effort, so ignore the exception
        } catch (IOException ioe) {
        } finally {
            Utility.closeQuietly(reader);
        }

        return null;
    }

    static void cacheUriRedirect(Context context, URI fromUri, URI toUri) {
        if (fromUri == null || toUri == null) {
            return;
        }

        OutputStream redirectStream = null;
        try {
            FileLruCache cache = getCache(context);
            redirectStream = cache.openPutStream(fromUri.toString(), REDIRECT_CONTENT_TAG);
            redirectStream.write(toUri.toString().getBytes());
        } catch (IOException e) {
            // Caching is best effort
        } finally {
            Utility.closeQuietly(redirectStream);
        }
    }

    static void clearCache(Context context) {
        try {
            getCache(context).clearCache();
        } catch (IOException e) {
            Logger.log(LoggingBehavior.CACHE, Log.WARN, TAG, "clearCache failed " + e.getMessage());
        }
    }
}
