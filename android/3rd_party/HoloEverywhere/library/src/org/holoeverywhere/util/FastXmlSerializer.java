
package org.holoeverywhere.util;

import java.io.IOException;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.io.Writer;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CoderResult;
import java.nio.charset.IllegalCharsetNameException;
import java.nio.charset.UnsupportedCharsetException;

import org.xmlpull.v1.XmlSerializer;

public class FastXmlSerializer implements XmlSerializer {
    private static final int BUFFER_LEN = 8192;

    private static final String ESCAPE_TABLE[] = new String[] {
            null, null,
            null, null, null, null, null, null, // 0-7
            null, null, null, null, null, null, null, null, // 8-15
            null, null, null, null, null, null, null, null, // 16-23
            null, null, null, null, null, null, null, null, // 24-31
            null, null, "&quot;", null, null, null, "&amp;", null, // 32-39
            null, null, null, null, null, null, null, null, // 40-47
            null, null, null, null, null, null, null, null, // 48-55
            null, null, null, null, "&lt;", null, "&gt;", null, // 56-63
    };

    private ByteBuffer mBytes = ByteBuffer
            .allocate(FastXmlSerializer.BUFFER_LEN);
    private CharsetEncoder mCharset;

    private boolean mInTag;

    private OutputStream mOutputStream;
    private int mPos;
    private final char[] mText = new char[FastXmlSerializer.BUFFER_LEN];

    private Writer mWriter;

    private void append(char c) throws IOException {
        int pos = mPos;
        if (pos >= FastXmlSerializer.BUFFER_LEN - 1) {
            flush();
            pos = mPos;
        }
        mText[pos] = c;
        mPos = pos + 1;
    }

    private void append(char[] buf, int i, final int length) throws IOException {
        if (length > FastXmlSerializer.BUFFER_LEN) {
            final int end = i + length;
            while (i < end) {
                int next = i + FastXmlSerializer.BUFFER_LEN;
                append(buf, i, next < end ? FastXmlSerializer.BUFFER_LEN : end
                        - i);
                i = next;
            }
            return;
        }
        int pos = mPos;
        if (pos + length > FastXmlSerializer.BUFFER_LEN) {
            flush();
            pos = mPos;
        }
        System.arraycopy(buf, i, mText, pos, length);
        mPos = pos + length;
    }

    private void append(String str) throws IOException {
        append(str, 0, str.length());
    }

    private void append(String str, int i, final int length) throws IOException {
        if (length > FastXmlSerializer.BUFFER_LEN) {
            final int end = i + length;
            while (i < end) {
                int next = i + FastXmlSerializer.BUFFER_LEN;
                append(str, i, next < end ? FastXmlSerializer.BUFFER_LEN : end
                        - i);
                i = next;
            }
            return;
        }
        int pos = mPos;
        if (pos + length > FastXmlSerializer.BUFFER_LEN) {
            flush();
            pos = mPos;
        }
        str.getChars(i, i + length, mText, pos);
        mPos = pos + length;
    }

    @Override
    public XmlSerializer attribute(String namespace, String name, String value)
            throws IOException, IllegalArgumentException, IllegalStateException {
        append(' ');
        if (namespace != null) {
            append(namespace);
            append(':');
        }
        append(name);
        append("=\"");

        escapeAndAppendString(value);
        append('"');
        return this;
    }

    @Override
    public void cdsect(String text) throws IOException,
            IllegalArgumentException, IllegalStateException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void comment(String text) throws IOException,
            IllegalArgumentException, IllegalStateException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void docdecl(String text) throws IOException,
            IllegalArgumentException, IllegalStateException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void endDocument() throws IOException, IllegalArgumentException,
            IllegalStateException {
        flush();
    }

    @Override
    public XmlSerializer endTag(String namespace, String name)
            throws IOException, IllegalArgumentException, IllegalStateException {
        if (mInTag) {
            append(" />\n");
        } else {
            append("</");
            if (namespace != null) {
                append(namespace);
                append(':');
            }
            append(name);
            append(">\n");
        }
        mInTag = false;
        return this;
    }

    @Override
    public void entityRef(String text) throws IOException,
            IllegalArgumentException, IllegalStateException {
        throw new UnsupportedOperationException();
    }

    private void escapeAndAppendString(char[] buf, int start, int len)
            throws IOException {
        final char NE = (char) FastXmlSerializer.ESCAPE_TABLE.length;
        final String[] escapes = FastXmlSerializer.ESCAPE_TABLE;
        int end = start + len;
        int lastPos = start;
        int pos;
        for (pos = start; pos < end; pos++) {
            char c = buf[pos];
            if (c >= NE) {
                continue;
            }
            String escape = escapes[c];
            if (escape == null) {
                continue;
            }
            if (lastPos < pos) {
                append(buf, lastPos, pos - lastPos);
            }
            lastPos = pos + 1;
            append(escape);
        }
        if (lastPos < pos) {
            append(buf, lastPos, pos - lastPos);
        }
    }

    private void escapeAndAppendString(final String string) throws IOException {
        final int N = string.length();
        final char NE = (char) FastXmlSerializer.ESCAPE_TABLE.length;
        final String[] escapes = FastXmlSerializer.ESCAPE_TABLE;
        int lastPos = 0;
        int pos;
        for (pos = 0; pos < N; pos++) {
            char c = string.charAt(pos);
            if (c >= NE) {
                continue;
            }
            String escape = escapes[c];
            if (escape == null) {
                continue;
            }
            if (lastPos < pos) {
                append(string, lastPos, pos - lastPos);
            }
            lastPos = pos + 1;
            append(escape);
        }
        if (lastPos < pos) {
            append(string, lastPos, pos - lastPos);
        }
    }

    @Override
    public void flush() throws IOException {
        // Log.i("PackageManager", "flush mPos=" + mPos);
        if (mPos > 0) {
            if (mOutputStream != null) {
                CharBuffer charBuffer = CharBuffer.wrap(mText, 0, mPos);
                CoderResult result = mCharset.encode(charBuffer, mBytes, true);
                while (true) {
                    if (result.isError()) {
                        throw new IOException(result.toString());
                    } else if (result.isOverflow()) {
                        flushBytes();
                        result = mCharset.encode(charBuffer, mBytes, true);
                        continue;
                    }
                    break;
                }
                flushBytes();
                mOutputStream.flush();
            } else {
                mWriter.write(mText, 0, mPos);
                mWriter.flush();
            }
            mPos = 0;
        }
    }

    private void flushBytes() throws IOException {
        int position;
        if ((position = mBytes.position()) > 0) {
            mBytes.flip();
            mOutputStream.write(mBytes.array(), 0, position);
            mBytes.clear();
        }
    }

    @Override
    public int getDepth() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean getFeature(String name) {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getName() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getNamespace() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getPrefix(String namespace, boolean generatePrefix)
            throws IllegalArgumentException {
        throw new UnsupportedOperationException();
    }

    @Override
    public Object getProperty(String name) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void ignorableWhitespace(String text) throws IOException,
            IllegalArgumentException, IllegalStateException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void processingInstruction(String text) throws IOException,
            IllegalArgumentException, IllegalStateException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setFeature(String name, boolean state)
            throws IllegalArgumentException, IllegalStateException {
        if (name.equals("http://xmlpull.org/v1/doc/features.html#indent-output")) {
            return;
        }
        throw new UnsupportedOperationException();
    }

    @Override
    public void setOutput(OutputStream os, String encoding) throws IOException,
            IllegalArgumentException, IllegalStateException {
        if (os == null) {
            throw new IllegalArgumentException();
        }
        try {
            mCharset = Charset.forName(encoding).newEncoder();
        } catch (IllegalCharsetNameException e) {
            throw (UnsupportedEncodingException) new UnsupportedEncodingException(
                    encoding).initCause(e);
        } catch (UnsupportedCharsetException e) {
            throw (UnsupportedEncodingException) new UnsupportedEncodingException(
                    encoding).initCause(e);
        }
        mOutputStream = os;
    }

    @Override
    public void setOutput(Writer writer) throws IOException,
            IllegalArgumentException, IllegalStateException {
        mWriter = writer;
    }

    @Override
    public void setPrefix(String prefix, String namespace) throws IOException,
            IllegalArgumentException, IllegalStateException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setProperty(String name, Object value)
            throws IllegalArgumentException, IllegalStateException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void startDocument(String encoding, Boolean standalone)
            throws IOException, IllegalArgumentException, IllegalStateException {
        append("<?xml version='1.0' encoding='utf-8' standalone='"
                + (standalone ? "yes" : "no") + "' ?>\n");
    }

    @Override
    public XmlSerializer startTag(String namespace, String name)
            throws IOException, IllegalArgumentException, IllegalStateException {
        if (mInTag) {
            append(">\n");
        }
        append('<');
        if (namespace != null) {
            append(namespace);
            append(':');
        }
        append(name);
        mInTag = true;
        return this;
    }

    @Override
    public XmlSerializer text(char[] buf, int start, int len)
            throws IOException, IllegalArgumentException, IllegalStateException {
        if (mInTag) {
            append(">");
            mInTag = false;
        }
        escapeAndAppendString(buf, start, len);
        return this;
    }

    @Override
    public XmlSerializer text(String text) throws IOException,
            IllegalArgumentException, IllegalStateException {
        if (mInTag) {
            append(">");
            mInTag = false;
        }
        escapeAndAppendString(text);
        return this;
    }

}
