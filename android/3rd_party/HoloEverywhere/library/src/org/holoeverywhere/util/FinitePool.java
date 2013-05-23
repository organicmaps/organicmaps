
package org.holoeverywhere.util;

import android.util.Log;

public class FinitePool<T extends Poolable<T>> implements Pool<T> {
    private static final String TAG = "FinitePool";
    private final boolean mInfinite;
    private final int mLimit;
    private final PoolableManager<T> mManager;
    private int mPoolCount;
    private T mRoot;

    public FinitePool(PoolableManager<T> manager) {
        mManager = manager;
        mLimit = 0;
        mInfinite = true;
    }

    public FinitePool(PoolableManager<T> manager, int limit) {
        if (limit <= 0) {
            throw new IllegalArgumentException("The pool limit must be > 0");
        }
        mManager = manager;
        mLimit = limit;
        mInfinite = false;
    }

    @Override
    public T acquire() {
        T element;
        if (mRoot != null) {
            element = mRoot;
            mRoot = element.getNextPoolable();
            mPoolCount--;
        } else {
            element = mManager.newInstance();
        }
        if (element != null) {
            element.setNextPoolable(null);
            element.setPooled(false);
            mManager.onAcquired(element);
        }
        return element;
    }

    @Override
    public void release(T element) {
        if (!element.isPooled()) {
            if (mInfinite || mPoolCount < mLimit) {
                mPoolCount++;
                element.setNextPoolable(mRoot);
                element.setPooled(true);
                mRoot = element;
            }
            mManager.onReleased(element);
        } else {
            Log.w(FinitePool.TAG, "Element is already in pool: " + element);
        }
    }
}
