
package org.holoeverywhere.util;

import android.os.Parcel;
import android.os.Parcelable;

public class SparseIntArray implements Cloneable, Parcelable {
    public static final Creator<SparseIntArray> CREATOR = new Creator<SparseIntArray>() {
        @Override
        public SparseIntArray createFromParcel(Parcel source) {
            final int size = source.readInt();
            SparseIntArray array = new SparseIntArray(size);
            array.mSize = size;
            array.mKeys = source.createIntArray();
            array.mValues = source.createIntArray();
            return array;
        }

        @Override
        public SparseIntArray[] newArray(int size) {
            return new SparseIntArray[size];
        }
    };

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

    private int[] mKeys;

    private int mSize;

    private int[] mValues;

    public SparseIntArray() {
        this(10);
    }

    public SparseIntArray(int initialCapacity) {
        initialCapacity = ArrayUtils.idealIntArraySize(initialCapacity);
        mKeys = new int[initialCapacity];
        mValues = new int[initialCapacity];
        mSize = 0;
    }

    public SparseIntArray(SparseIntArray arrayForCopy) {
        if (arrayForCopy == null) {
            int initialCapacity = ArrayUtils.idealIntArraySize(10);
            mKeys = new int[initialCapacity];
            mValues = new int[initialCapacity];
            mSize = 0;
        } else {
            mKeys = arrayForCopy.mKeys.clone();
            mValues = arrayForCopy.mValues.clone();
        }
    }

    public void append(int key, int value) {
        if (mSize != 0 && key <= mKeys[mSize - 1]) {
            put(key, value);
            return;
        }
        int pos = mSize;
        if (pos >= mKeys.length) {
            int n = ArrayUtils.idealIntArraySize(pos + 1);
            int[] nkeys = new int[n];
            int[] nvalues = new int[n];
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
        mSize = 0;
    }

    @Override
    public SparseIntArray clone() {
        SparseIntArray clone = null;
        try {
            clone = (SparseIntArray) super.clone();
            clone.mKeys = mKeys.clone();
            clone.mValues = mValues.clone();
            clone.mSize = mSize;
        } catch (CloneNotSupportedException cnse) {
        }
        return clone;
    }

    public void delete(int key) {
        int i = binarySearch(mKeys, 0, mSize, key);
        if (i >= 0) {
            removeAt(i);
        }
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public int get(int key) {
        return get(key, 0);
    }

    public int get(int key, int valueIfKeyNotFound) {
        int i = binarySearch(mKeys, 0, mSize, key);
        if (i < 0) {
            return valueIfKeyNotFound;
        } else {
            return mValues[i];
        }
    }

    public int indexOfKey(int key) {
        return binarySearch(mKeys, 0, mSize, key);
    }

    public int indexOfValue(int value) {
        for (int i = 0; i < mSize; i++) {
            if (mValues[i] == value) {
                return i;
            }
        }
        return -1;
    }

    public int keyAt(int index) {
        return mKeys[index];
    }

    public void put(int key, int value) {
        int i = binarySearch(mKeys, 0, mSize, key);
        if (i >= 0) {
            mValues[i] = value;
        } else {
            i = ~i;
            if (mSize >= mKeys.length) {
                int n = ArrayUtils.idealIntArraySize(mSize + 1);
                int[] nkeys = new int[n];
                int[] nvalues = new int[n];
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

    public void removeAt(int index) {
        System.arraycopy(mKeys, index + 1, mKeys, index, mSize - (index + 1));
        System.arraycopy(mValues, index + 1, mValues, index, mSize - (index + 1));
        mSize--;
    }

    public int size() {
        return mSize;
    }

    public int valueAt(int index) {
        return mValues[index];
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mSize);
        dest.writeIntArray(mKeys);
        dest.writeIntArray(mValues);
    }
}
