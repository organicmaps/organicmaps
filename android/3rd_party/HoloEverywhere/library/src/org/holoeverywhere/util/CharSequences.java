
package org.holoeverywhere.util;

public class CharSequences {
    public static int compareToIgnoreCase(CharSequence me, CharSequence another) {
        int myLen = me.length(), anotherLen = another.length();
        int myPos = 0, anotherPos = 0, result;
        int end = myLen < anotherLen ? myLen : anotherLen;
        while (myPos < end) {
            if ((result = Character.toLowerCase(me.charAt(myPos++))
                    - Character.toLowerCase(another.charAt(anotherPos++))) != 0) {
                return result;
            }
        }
        return myLen - anotherLen;
    }

    public static boolean equals(CharSequence a, CharSequence b) {
        if (a.length() != b.length()) {
            return false;
        }
        int length = a.length();
        for (int i = 0; i < length; i++) {
            if (a.charAt(i) != b.charAt(i)) {
                return false;
            }
        }
        return true;
    }

    public static CharSequence forAsciiBytes(final byte[] bytes) {
        return new CharSequence() {
            @Override
            public char charAt(int index) {
                return (char) bytes[index];
            }

            @Override
            public int length() {
                return bytes.length;
            }

            @Override
            public CharSequence subSequence(int start, int end) {
                return CharSequences.forAsciiBytes(bytes, start, end);
            }

            @Override
            public String toString() {
                return new String(bytes);
            }
        };
    }

    public static CharSequence forAsciiBytes(final byte[] bytes,
            final int start, final int end) {
        CharSequences.validate(start, end, bytes.length);
        return new CharSequence() {
            @Override
            public char charAt(int index) {
                return (char) bytes[index + start];
            }

            @Override
            public int length() {
                return end - start;
            }

            @Override
            public CharSequence subSequence(int newStart, int newEnd) {
                newStart -= start;
                newEnd -= start;
                CharSequences.validate(newStart, newEnd, length());
                return CharSequences.forAsciiBytes(bytes, newStart, newEnd);
            }

            @Override
            public String toString() {
                return new String(bytes, start, length());
            }
        };
    }

    static void validate(int start, int end, int length) {
        if (start < 0 || end < 0 || end > length || start > end) {
            throw new IndexOutOfBoundsException();
        }
    }
}
