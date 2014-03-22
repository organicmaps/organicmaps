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

import android.os.Handler;

class RequestProgress {
    private final Request request;
    private final Handler callbackHandler;
    private final long threshold;

    private long progress, lastReportedProgress, maxProgress;

    RequestProgress(Handler callbackHandler, Request request) {
        this.request = request;
        this.callbackHandler = callbackHandler;

        this.threshold = Settings.getOnProgressThreshold();
    }

    long getProgress() {
        return progress;
    }

    long getMaxProgress() {
        return maxProgress;
    }

    void addProgress(long size) {
        progress += size;

        if (progress >= lastReportedProgress + threshold || progress >= maxProgress) {
            reportProgress();
        }
    }

    void addToMax(long size) {
        maxProgress += size;
    }

    void reportProgress() {
        if (progress > lastReportedProgress) {
            Request.Callback callback = request.getCallback();
            if (maxProgress > 0 && callback instanceof Request.OnProgressCallback) {
                // Keep copies to avoid threading issues
                final long currentCopy = progress;
                final long maxProgressCopy = maxProgress;
                final Request.OnProgressCallback callbackCopy = (Request.OnProgressCallback) callback;
                if (callbackHandler == null) {
                    callbackCopy.onProgress(currentCopy, maxProgressCopy);
                }
                else {
                    callbackHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            callbackCopy.onProgress(currentCopy, maxProgressCopy);
                        }
                    });
                }
                lastReportedProgress = progress;
            }
        }
    }
}
