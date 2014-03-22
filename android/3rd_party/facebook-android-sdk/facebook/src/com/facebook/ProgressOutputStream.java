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

import java.io.FilterOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Map;

class ProgressOutputStream extends FilterOutputStream implements RequestOutputStream {
    private final Map<Request, RequestProgress> progressMap;
    private final RequestBatch requests;
    private final long threshold;

    private long batchProgress, lastReportedProgress, maxProgress;
    private RequestProgress currentRequestProgress;

    ProgressOutputStream(OutputStream out, RequestBatch requests, Map<Request, RequestProgress> progressMap, long maxProgress) {
        super(out);
        this.requests = requests;
        this.progressMap = progressMap;
        this.maxProgress = maxProgress;

        this.threshold = Settings.getOnProgressThreshold();
    }

    private void addProgress(long size) {
        if (currentRequestProgress != null) {
            currentRequestProgress.addProgress(size);
        }

        batchProgress += size;

        if (batchProgress >= lastReportedProgress + threshold || batchProgress >= maxProgress) {
            reportBatchProgress();
        }
    }

    private void reportBatchProgress() {
        if (batchProgress > lastReportedProgress) {
            for (RequestBatch.Callback callback : requests.getCallbacks()) {
                if (callback instanceof RequestBatch.OnProgressCallback) {
                    final Handler callbackHandler = requests.getCallbackHandler();

                    // Keep copies to avoid threading issues
                    final RequestBatch.OnProgressCallback progressCallback = (RequestBatch.OnProgressCallback) callback;
                    if (callbackHandler == null) {
                        progressCallback.onBatchProgress(requests, batchProgress, maxProgress);
                    }
                    else {
                        callbackHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                progressCallback.onBatchProgress(requests, batchProgress, maxProgress);
                            }
                        });
                    }
                }
            }

            lastReportedProgress = batchProgress;
        }
    }

    public void setCurrentRequest(Request request) {
        currentRequestProgress = request != null? progressMap.get(request) : null;
    }

    long getBatchProgress() {
        return batchProgress;
    }

    long getMaxProgress() {
        return maxProgress;
    }

    @Override
    public void write(byte[] buffer) throws IOException {
        out.write(buffer);
        addProgress(buffer.length);
    }

    @Override
    public void write(byte[] buffer, int offset, int length) throws IOException {
        out.write(buffer, offset, length);
        addProgress(length);
    }

    @Override
    public void write(int oneByte) throws IOException {
        out.write(oneByte);
        addProgress(1);
    }

    @Override
    public void close() throws IOException {
        super.close();

        for (RequestProgress p : progressMap.values()) {
            p.reportProgress();
        }

        reportBatchProgress();
    }
}
