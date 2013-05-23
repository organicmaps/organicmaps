
package org.holoeverywhere.util;

import java.lang.reflect.Array;

public class Arrays {
    @SuppressWarnings("unchecked")
    public static <T> T[] copyOfRange(T[] original, int from, int to) {
        return Arrays.copyOfRange(original, from, to,
                (Class<T[]>) original.getClass());
    }

    @SuppressWarnings("unchecked")
    public static <T, U> T[] copyOfRange(U[] original, int from, int to,
            Class<? extends T[]> newType) {
        int newSize = to - from;
        if (newSize < 0) {
            throw new IllegalArgumentException(from + " > " + to);
        }
        T[] copy = (Object) newType == (Object) Object[].class ? (T[]) new Object[newSize]
                : (T[]) Array.newInstance(newType.getComponentType(), newSize);
        System.arraycopy(original, from, copy, 0,
                Math.min(original.length - from, newSize));
        return copy;
    }

}
