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

import android.database.CursorIndexOutOfBoundsException;
import com.facebook.model.GraphObject;

import java.util.ArrayList;
import java.util.Collection;

class SimpleGraphObjectCursor<T extends GraphObject> implements GraphObjectCursor<T> {
    private int pos = -1;
    private boolean closed = false;
    private ArrayList<T> graphObjects = new ArrayList<T>();
    private boolean moreObjectsAvailable = false;
    private boolean fromCache = false;

    SimpleGraphObjectCursor() {
    }

    SimpleGraphObjectCursor(SimpleGraphObjectCursor<T> other) {
        pos = other.pos;
        closed = other.closed;
        graphObjects = new ArrayList<T>();
        graphObjects.addAll(other.graphObjects);
        fromCache = other.fromCache;

        // We do not copy observers.
    }

    public void addGraphObjects(Collection<T> graphObjects, boolean fromCache) {
        this.graphObjects.addAll(graphObjects);
        // We consider this cached if ANY results were from the cache.
        this.fromCache |= fromCache;
    }

    public boolean isFromCache() {
        return fromCache;
    }

    public void setFromCache(boolean fromCache) {
        this.fromCache = fromCache;
    }

    public boolean areMoreObjectsAvailable() {
        return moreObjectsAvailable;
    }

    public void setMoreObjectsAvailable(boolean moreObjectsAvailable) {
        this.moreObjectsAvailable = moreObjectsAvailable;
    }

    @Override
    public int getCount() {
        return graphObjects.size();
    }

    @Override
    public int getPosition() {
        return pos;
    }

    @Override
    public boolean move(int offset) {
        return moveToPosition(pos + offset);
    }

    @Override
    public boolean moveToPosition(int position) {
        final int count = getCount();
        if (position >= count) {
            pos = count;
            return false;
        }

        if (position < 0) {
            pos = -1;
            return false;
        }

        pos = position;
        return true;
    }

    @Override
    public boolean moveToFirst() {
        return moveToPosition(0);
    }

    @Override
    public boolean moveToLast() {
        return moveToPosition(getCount() - 1);
    }

    @Override
    public boolean moveToNext() {
        return moveToPosition(pos + 1);
    }

    @Override
    public boolean moveToPrevious() {
        return moveToPosition(pos - 1);
    }

    @Override
    public boolean isFirst() {
        return (pos == 0) && (getCount() != 0);
    }

    @Override
    public boolean isLast() {
        final int count = getCount();
        return (pos == (count - 1)) && (count != 0);
    }

    @Override
    public boolean isBeforeFirst() {
        return (getCount() == 0) || (pos == -1);
    }

    @Override
    public boolean isAfterLast() {
        final int count = getCount();
        return (count == 0) || (pos == count);
    }

    @Override
    public T getGraphObject() {
        if (pos < 0) {
            throw new CursorIndexOutOfBoundsException("Before first object.");
        }
        if (pos >= graphObjects.size()) {
            throw new CursorIndexOutOfBoundsException("After last object.");
        }
        return graphObjects.get(pos);
    }

    @Override
    public void close() {
        closed = true;
    }

    @Override
    public boolean isClosed() {
        return closed;
    }

}
