
package org.holoeverywhere.util;

public class SparseArray<E> implements Cloneable {
    private static final Object DELETED = new Object();

    private static int binarySearch(int[] a, int start, int len, int key) {
        int high = start + len, low = start - 1, guess;
        while (high - low > 1) {
            guess = (high + low) / 2;
            if (a[guess] < key) {
                low = guess;
            } else {
                high = guess;
            }
        }
        if (high == start + len) {
            return ~(start + len);
        } else if (a[high] == key) {
            return high;
        } else {
            return ~high;
        }
    }

    private boolean mGarbage = false;
    private int[] mKeys;
    private int mSize;

    private Object[] mValues;

    public SparseArray() {
        this(10);
    }

    public SparseArray(int initialCapacity) {
        initialCapacity = ArrayUtils.idealIntArraySize(initialCapacity);
        mKeys = new int[initialCapacity];
        mValues = new Object[initialCapacity];
        mSize = 0;
    }

    public void append(int key, E value) {
        if (mSize != 0 && key <= mKeys[mSize - 1]) {
            put(key, value);
            return;
        }
        if (mGarbage && mSize >= mKeys.length) {
            gc();
        }
        int pos = mSize;
        if (pos >= mKeys.length) {
            int n = ArrayUtils.idealIntArraySize(pos + 1);
            int[] nkeys = new int[n];
            Object[] nvalues = new Object[n];
            System.arraycopy(mKeys, 0, nkeys, 0, mKeys.length);
            System.arraycopy(mValues, 0, nvalues, 0, mValues.length);
            mKeys = nkeys;
            mValues = nvalues;
        }
        mKeys[pos] = key;
        mValues[pos] = value;
        mSize = pos + 1;
    }

    public void clear() {
        int n = mSize;
        Object[] values = mValues;
        for (int i = 0; i < n; i++) {
            values[i] = null;
        }
        mSize = 0;
        mGarbage = false;
    }

    @Override
    @SuppressWarnings("unchecked")
    public SparseArray<E> clone() {
        SparseArray<E> clone = null;
        try {
            clone = (SparseArray<E>) super.clone();
            clone.mKeys = mKeys.clone();
            clone.mValues = mValues.clone();
        } catch (CloneNotSupportedException cnse) {
        }
        return clone;
    }

    public void delete(int key) {
        int i = SparseArray.binarySearch(mKeys, 0, mSize, key);

        if (i >= 0) {
            if (mValues[i] != SparseArray.DELETED) {
                mValues[i] = SparseArray.DELETED;
                mGarbage = true;
            }
        }
    }

    private void gc() {
        int n = mSize;
        int o = 0;
        int[] keys = mKeys;
        Object[] values = mValues;
        for (int i = 0; i < n; i++) {
            Object val = values[i];
            if (val != SparseArray.DELETED) {
                if (i != o) {
                    keys[o] = keys[i];
                    values[o] = val;
                    values[i] = null;
                }
                o++;
            }
        }
        mGarbage = false;
        mSize = o;
    }

    public E get(int key) {
        return get(key, null);
    }

    @SuppressWarnings("unchecked")
    public E get(int key, E valueIfKeyNotFound) {
        int i = SparseArray.binarySearch(mKeys, 0, mSize, key);

        if (i < 0 || mValues[i] == SparseArray.DELETED) {
            return valueIfKeyNotFound;
        } else {
            return (E) mValues[i];
        }
    }

    public int indexOfKey(int key) {
        if (mGarbage) {
            gc();
        }
        return SparseArray.binarySearch(mKeys, 0, mSize, key);
    }

    public int indexOfValue(E value) {
        if (mGarbage) {
            gc();
        }
        for (int i = 0; i < mSize; i++) {
            if (mValues[i] == value) {
                return i;
            }
        }
        return -1;
    }

    public int keyAt(int index) {
        if (mGarbage) {
            gc();
        }
        return mKeys[index];
    }

    public void put(int key, E value) {
        int i = SparseArray.binarySearch(mKeys, 0, mSize, key);
        if (i >= 0) {
            mValues[i] = value;
        } else {
            i = ~i;
            if (i < mSize && mValues[i] == SparseArray.DELETED) {
                mKeys[i] = key;
                mValues[i] = value;
                return;
            }
            if (mGarbage && mSize >= mKeys.length) {
                gc();
                i = ~SparseArray.binarySearch(mKeys, 0, mSize, key);
            }
            if (mSize >= mKeys.length) {
                int n = ArrayUtils.idealIntArraySize(mSize + 1);
                int[] nkeys = new int[n];
                Object[] nvalues = new Object[n];
                System.arraycopy(mKeys, 0, nkeys, 0, mKeys.length);
                System.arraycopy(mValues, 0, nvalues, 0, mValues.length);
                mKeys = nkeys;
                mValues = nvalues;
            }
            if (mSize - i != 0) {
                System.arraycopy(mKeys, i, mKeys, i + 1, mSize - i);
                System.arraycopy(mValues, i, mValues, i + 1, mSize - i);
            }
            mKeys[i] = key;
            mValues[i] = value;
            mSize++;
        }
    }

    public void remove(int key) {
        delete(key);
    }

    public void removeAt(int index) {
        if (mValues[index] != SparseArray.DELETED) {
            mValues[index] = SparseArray.DELETED;
            mGarbage = true;
        }
    }

    public void setValueAt(int index, E value) {
        if (mGarbage) {
            gc();
        }

        mValues[index] = value;
    }

    public int size() {
        if (mGarbage) {
            gc();
        }

        return mSize;
    }

    @SuppressWarnings("unchecked")
    public E valueAt(int index) {
        if (mGarbage) {
            gc();
        }

        return (E) mValues[index];
    }
}
