
package org.holoeverywhere.util;

public class SynchronizedPool<T extends Poolable<T>> implements Pool<T> {
    private final Object mLock;
    private final Pool<T> mPool;

    public SynchronizedPool(Pool<T> pool) {
        mPool = pool;
        mLock = this;
    }

    public SynchronizedPool(Pool<T> pool, Object lock) {
        mPool = pool;
        mLock = lock;
    }

    @Override
    public T acquire() {
        synchronized (mLock) {
            return mPool.acquire();
        }
    }

    @Override
    public void release(T element) {
        synchronized (mLock) {
            mPool.release(element);
        }
    }
}
