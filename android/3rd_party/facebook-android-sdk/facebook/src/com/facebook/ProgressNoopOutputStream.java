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

import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

class ProgressNoopOutputStream extends OutputStream implements RequestOutputStream {
    private final Map<Request, RequestProgress> progressMap = new HashMap<Request, RequestProgress>();
    private final Handler callbackHandler;

    private Request currentRequest;
    private RequestProgress currentRequestProgress;
    private int batchMax;

    ProgressNoopOutputStream(Handler callbackHandler) {
        this.callbackHandler = callbackHandler;
    }

    public void setCurrentRequest(Request currentRequest) {
        this.currentRequest = currentRequest;
        this.currentRequestProgress = currentRequest != null? progressMap.get(currentRequest) : null;
    }

    int getMaxProgress() {
        return batchMax;
    }

    Map<Request,RequestProgress> getProgressMap() {
        return progressMap;
    }

    void addProgress(long size) {
        if (currentRequestProgress == null) {
            currentRequestProgress = new RequestProgress(callbackHandler, currentRequest);
            progressMap.put(currentRequest, currentRequestProgress);
        }

        currentRequestProgress.addToMax(size);
        batchMax += size;
    }

    @Override
    public void write(byte[] buffer) {
        addProgress(buffer.length);
    }

    @Override
    public void write(byte[] buffer, int offset, int length) {
        addProgress(length);
    }

    @Override
    public void write(int oneByte) {
        addProgress(1);
    }
}
