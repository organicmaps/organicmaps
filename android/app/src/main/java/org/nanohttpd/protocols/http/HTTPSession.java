package org.nanohttpd.protocols.http;

/*
 * #%L
 * NanoHttpd-Core
 * %%
 * Copyright (C) 2012 - 2016 nanohttpd
 * %%
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the nanohttpd nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * #L%
 */

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.logging.Level;
import java.util.regex.Matcher;

import javax.net.ssl.SSLException;

import org.nanohttpd.protocols.http.NanoHTTPD.ResponseException;
import org.nanohttpd.protocols.http.content.ContentType;
import org.nanohttpd.protocols.http.content.CookieHandler;
import org.nanohttpd.protocols.http.request.Method;
import org.nanohttpd.protocols.http.response.Response;
import org.nanohttpd.protocols.http.response.Status;
import org.nanohttpd.protocols.http.tempfiles.ITempFile;
import org.nanohttpd.protocols.http.tempfiles.ITempFileManager;

public class HTTPSession implements IHTTPSession {

    public static final String POST_DATA = "postData";

    private static final int REQUEST_BUFFER_LEN = 512;

    private static final int MEMORY_STORE_LIMIT = 1024;

    public static final int BUFSIZE = 8192;

    public static final int MAX_HEADER_SIZE = 1024;

    private final NanoHTTPD httpd;

    private final ITempFileManager tempFileManager;

    private final OutputStream outputStream;

    private final BufferedInputStream inputStream;

    private int splitbyte;

    private int rlen;

    private String uri;

    private Method method;

    private Map<String, List<String>> parms;

    private Map<String, String> headers;

    private CookieHandler cookies;

    private String queryParameterString;

    private String remoteIp;

    private String protocolVersion;

    public HTTPSession(NanoHTTPD httpd, ITempFileManager tempFileManager, InputStream inputStream, OutputStream outputStream) {
        this.httpd = httpd;
        this.tempFileManager = tempFileManager;
        this.inputStream = new BufferedInputStream(inputStream, HTTPSession.BUFSIZE);
        this.outputStream = outputStream;
    }

    public HTTPSession(NanoHTTPD httpd, ITempFileManager tempFileManager, InputStream inputStream, OutputStream outputStream, InetAddress inetAddress) {
        this.httpd = httpd;
        this.tempFileManager = tempFileManager;
        this.inputStream = new BufferedInputStream(inputStream, HTTPSession.BUFSIZE);
        this.outputStream = outputStream;
        this.remoteIp = inetAddress.isLoopbackAddress() || inetAddress.isAnyLocalAddress() ? "127.0.0.1" : inetAddress.getHostAddress().toString();
        this.headers = new HashMap<String, String>();
    }

    /**
     * Decodes the sent headers and loads the data into Key/value pairs
     */
    private void decodeHeader(BufferedReader in, Map<String, String> pre, Map<String, List<String>> parms, Map<String, String> headers) throws ResponseException {
        try {
            // Read the request line
            String inLine = in.readLine();
            if (inLine == null) {
                return;
            }

            StringTokenizer st = new StringTokenizer(inLine);
            if (!st.hasMoreTokens()) {
                throw new ResponseException(Status.BAD_REQUEST, "BAD REQUEST: Syntax error. Usage: GET /example/file.html");
            }

            pre.put("method", st.nextToken());

            if (!st.hasMoreTokens()) {
                throw new ResponseException(Status.BAD_REQUEST, "BAD REQUEST: Missing URI. Usage: GET /example/file.html");
            }

            String uri = st.nextToken();

            // Decode parameters from the URI
            int qmi = uri.indexOf('?');
            if (qmi >= 0) {
                decodeParms(uri.substring(qmi + 1), parms);
                uri = NanoHTTPD.decodePercent(uri.substring(0, qmi));
            } else {
                uri = NanoHTTPD.decodePercent(uri);
            }

            // If there's another token, its protocol version,
            // followed by HTTP headers.
            // NOTE: this now forces header names lower case since they are
            // case insensitive and vary by client.
            if (st.hasMoreTokens()) {
                protocolVersion = st.nextToken();
            } else {
                protocolVersion = "HTTP/1.1";
                NanoHTTPD.LOG.log(Level.FINE, "no protocol version specified, strange. Assuming HTTP/1.1.");
            }
            String line = in.readLine();
            while (line != null && !line.trim().isEmpty()) {
                int p = line.indexOf(':');
                if (p >= 0) {
                    headers.put(line.substring(0, p).trim().toLowerCase(Locale.US), line.substring(p + 1).trim());
                }
                line = in.readLine();
            }

            pre.put("uri", uri);
        } catch (IOException ioe) {
            throw new ResponseException(Status.INTERNAL_ERROR, "SERVER INTERNAL ERROR: IOException: " + ioe.getMessage(), ioe);
        }
    }

    /**
     * Decodes the Multipart Body data and put it into Key/Value pairs.
     */
    private void decodeMultipartFormData(ContentType contentType, ByteBuffer fbuf, Map<String, List<String>> parms, Map<String, String> files) throws ResponseException {
        int pcount = 0;
        try {
            int[] boundaryIdxs = getBoundaryPositions(fbuf, contentType.getBoundary().getBytes());
            if (boundaryIdxs.length < 2) {
                throw new ResponseException(Status.BAD_REQUEST, "BAD REQUEST: Content type is multipart/form-data but contains less than two boundary strings.");
            }

            byte[] partHeaderBuff = new byte[MAX_HEADER_SIZE];
            for (int boundaryIdx = 0; boundaryIdx < boundaryIdxs.length - 1; boundaryIdx++) {
                fbuf.position(boundaryIdxs[boundaryIdx]);
                int len = (fbuf.remaining() < MAX_HEADER_SIZE) ? fbuf.remaining() : MAX_HEADER_SIZE;
                fbuf.get(partHeaderBuff, 0, len);
                BufferedReader in =
                        new BufferedReader(new InputStreamReader(new ByteArrayInputStream(partHeaderBuff, 0, len), Charset.forName(contentType.getEncoding())), len);

                int headerLines = 0;
                // First line is boundary string
                String mpline = in.readLine();
                headerLines++;
                if (mpline == null || !mpline.contains(contentType.getBoundary())) {
                    throw new ResponseException(Status.BAD_REQUEST, "BAD REQUEST: Content type is multipart/form-data but chunk does not start with boundary.");
                }

                String partName = null, fileName = null, partContentType = null;
                // Parse the reset of the header lines
                mpline = in.readLine();
                headerLines++;
                while (mpline != null && mpline.trim().length() > 0) {
                    Matcher matcher = NanoHTTPD.CONTENT_DISPOSITION_PATTERN.matcher(mpline);
                    if (matcher.matches()) {
                        String attributeString = matcher.group(2);
                        matcher = NanoHTTPD.CONTENT_DISPOSITION_ATTRIBUTE_PATTERN.matcher(attributeString);
                        while (matcher.find()) {
                            String key = matcher.group(1);
                            if ("name".equalsIgnoreCase(key)) {
                                partName = matcher.group(2);
                            } else if ("filename".equalsIgnoreCase(key)) {
                                fileName = matcher.group(2);
                                // add these two line to support multiple
                                // files uploaded using the same field Id
                                if (!fileName.isEmpty()) {
                                    if (pcount > 0)
                                        partName = partName + String.valueOf(pcount++);
                                    else
                                        pcount++;
                                }
                            }
                        }
                    }
                    matcher = NanoHTTPD.CONTENT_TYPE_PATTERN.matcher(mpline);
                    if (matcher.matches()) {
                        partContentType = matcher.group(2).trim();
                    }
                    mpline = in.readLine();
                    headerLines++;
                }
                int partHeaderLength = 0;
                while (headerLines-- > 0) {
                    partHeaderLength = scipOverNewLine(partHeaderBuff, partHeaderLength);
                }
                // Read the part data
                if (partHeaderLength >= len - 4) {
                    throw new ResponseException(Status.INTERNAL_ERROR, "Multipart header size exceeds MAX_HEADER_SIZE.");
                }
                int partDataStart = boundaryIdxs[boundaryIdx] + partHeaderLength;
                int partDataEnd = boundaryIdxs[boundaryIdx + 1] - 4;

                fbuf.position(partDataStart);

                List<String> values = parms.get(partName);
                if (values == null) {
                    values = new ArrayList<String>();
                    parms.put(partName, values);
                }

                if (partContentType == null) {
                    // Read the part into a string
                    byte[] data_bytes = new byte[partDataEnd - partDataStart];
                    fbuf.get(data_bytes);

                    values.add(new String(data_bytes, contentType.getEncoding()));
                } else {
                    // Read it into a file
                    String path = saveTmpFile(fbuf, partDataStart, partDataEnd - partDataStart, fileName);
                    if (!files.containsKey(partName)) {
                        files.put(partName, path);
                    } else {
                        int count = 2;
                        while (files.containsKey(partName + count)) {
                            count++;
                        }
                        files.put(partName + count, path);
                    }
                    values.add(fileName);
                }
            }
        } catch (ResponseException re) {
            throw re;
        } catch (Exception e) {
            throw new ResponseException(Status.INTERNAL_ERROR, e.toString());
        }
    }

    private int scipOverNewLine(byte[] partHeaderBuff, int index) {
        while (partHeaderBuff[index] != '\n') {
            index++;
        }
        return ++index;
    }

    /**
     * Decodes parameters in percent-encoded URI-format ( e.g.
     * "name=Jack%20Daniels&pass=Single%20Malt" ) and adds them to given Map.
     */
    private void decodeParms(String parms, Map<String, List<String>> p) {
        if (parms == null) {
            this.queryParameterString = "";
            return;
        }

        this.queryParameterString = parms;
        StringTokenizer st = new StringTokenizer(parms, "&");
        while (st.hasMoreTokens()) {
            String e = st.nextToken();
            int sep = e.indexOf('=');
            String key = null;
            String value = null;

            if (sep >= 0) {
                key = NanoHTTPD.decodePercent(e.substring(0, sep)).trim();
                value = NanoHTTPD.decodePercent(e.substring(sep + 1));
            } else {
                key = NanoHTTPD.decodePercent(e).trim();
                value = "";
            }

            List<String> values = p.get(key);
            if (values == null) {
                values = new ArrayList<String>();
                p.put(key, values);
            }

            values.add(value);
        }
    }

    @Override
    public void execute() throws IOException {
        Response r = null;
        try {
            // Read the first 8192 bytes.
            // The full header should fit in here.
            // Apache's default header limit is 8KB.
            // Do NOT assume that a single read will get the entire header
            // at once!
            byte[] buf = new byte[HTTPSession.BUFSIZE];
            this.splitbyte = 0;
            this.rlen = 0;

            int read = -1;
            this.inputStream.mark(HTTPSession.BUFSIZE);
            try {
                read = this.inputStream.read(buf, 0, HTTPSession.BUFSIZE);
            } catch (SSLException e) {
                throw e;
            } catch (IOException e) {
                NanoHTTPD.safeClose(this.inputStream);
                NanoHTTPD.safeClose(this.outputStream);
                throw new SocketException("NanoHttpd Shutdown");
            }
            if (read == -1) {
                // socket was been closed
                NanoHTTPD.safeClose(this.inputStream);
                NanoHTTPD.safeClose(this.outputStream);
                throw new SocketException("NanoHttpd Shutdown");
            }
            while (read > 0) {
                this.rlen += read;
                this.splitbyte = findHeaderEnd(buf, this.rlen);
                if (this.splitbyte > 0) {
                    break;
                }
                read = this.inputStream.read(buf, this.rlen, HTTPSession.BUFSIZE - this.rlen);
            }

            if (this.splitbyte < this.rlen) {
                this.inputStream.reset();
                this.inputStream.skip(this.splitbyte);
            }

            this.parms = new HashMap<String, List<String>>();
            if (null == this.headers) {
                this.headers = new HashMap<String, String>();
            } else {
                this.headers.clear();
            }

            // Create a BufferedReader for parsing the header.
            BufferedReader hin = new BufferedReader(new InputStreamReader(new ByteArrayInputStream(buf, 0, this.rlen)));

            // Decode the header into parms and header java properties
            Map<String, String> pre = new HashMap<String, String>();
            decodeHeader(hin, pre, this.parms, this.headers);

            if (null != this.remoteIp) {
                this.headers.put("remote-addr", this.remoteIp);
                this.headers.put("http-client-ip", this.remoteIp);
            }

            this.method = Method.lookup(pre.get("method"));
            if (this.method == null) {
                throw new ResponseException(Status.BAD_REQUEST, "BAD REQUEST: Syntax error. HTTP verb " + pre.get("method") + " unhandled.");
            }

            this.uri = pre.get("uri");

            this.cookies = new CookieHandler(this.headers);

            String connection = this.headers.get("connection");
            boolean keepAlive = "HTTP/1.1".equals(protocolVersion) && (connection == null || !connection.matches("(?i).*close.*"));

            // Ok, now do the serve()

            // TODO: long body_size = getBodySize();
            // TODO: long pos_before_serve = this.inputStream.totalRead()
            // (requires implementation for totalRead())
            r = httpd.handle(this);
            // TODO: this.inputStream.skip(body_size -
            // (this.inputStream.totalRead() - pos_before_serve))

            if (r == null) {
                throw new ResponseException(Status.INTERNAL_ERROR, "SERVER INTERNAL ERROR: Serve() returned a null response.");
            } else {
                String acceptEncoding = this.headers.get("accept-encoding");
                this.cookies.unloadQueue(r);
                r.setRequestMethod(this.method);
                if (acceptEncoding == null || !acceptEncoding.contains("gzip")) {
                    r.setUseGzip(false);
                }
                r.setKeepAlive(keepAlive);
                r.send(this.outputStream);
            }
            if (!keepAlive || r.isCloseConnection()) {
                throw new SocketException("NanoHttpd Shutdown");
            }
        } catch (SocketException e) {
            // throw it out to close socket object (finalAccept)
            throw e;
        } catch (SocketTimeoutException ste) {
            // treat socket timeouts the same way we treat socket exceptions
            // i.e. close the stream & finalAccept object by throwing the
            // exception up the call stack.
            throw ste;
        } catch (SSLException ssle) {
            Response resp = Response.newFixedLengthResponse(Status.INTERNAL_ERROR, NanoHTTPD.MIME_PLAINTEXT, "SSL PROTOCOL FAILURE: " + ssle.getMessage());
            resp.send(this.outputStream);
            NanoHTTPD.safeClose(this.outputStream);
        } catch (IOException ioe) {
            Response resp = Response.newFixedLengthResponse(Status.INTERNAL_ERROR, NanoHTTPD.MIME_PLAINTEXT, "SERVER INTERNAL ERROR: IOException: " + ioe.getMessage());
            resp.send(this.outputStream);
            NanoHTTPD.safeClose(this.outputStream);
        } catch (ResponseException re) {
            Response resp = Response.newFixedLengthResponse(re.getStatus(), NanoHTTPD.MIME_PLAINTEXT, re.getMessage());
            resp.send(this.outputStream);
            NanoHTTPD.safeClose(this.outputStream);
        } finally {
            NanoHTTPD.safeClose(r);
            this.tempFileManager.clear();
        }
    }

    /**
     * Find byte index separating header from body. It must be the last byte of
     * the first two sequential new lines.
     */
    private int findHeaderEnd(final byte[] buf, int rlen) {
        int splitbyte = 0;
        while (splitbyte + 1 < rlen) {

            // RFC2616
            if (buf[splitbyte] == '\r' && buf[splitbyte + 1] == '\n' && splitbyte + 3 < rlen && buf[splitbyte + 2] == '\r' && buf[splitbyte + 3] == '\n') {
                return splitbyte + 4;
            }

            // tolerance
            if (buf[splitbyte] == '\n' && buf[splitbyte + 1] == '\n') {
                return splitbyte + 2;
            }
            splitbyte++;
        }
        return 0;
    }

    /**
     * Find the byte positions where multipart boundaries start. This reads a
     * large block at a time and uses a temporary buffer to optimize (memory
     * mapped) file access.
     */
    private int[] getBoundaryPositions(ByteBuffer b, byte[] boundary) {
        int[] res = new int[0];
        if (b.remaining() < boundary.length) {
            return res;
        }

        int search_window_pos = 0;
        byte[] search_window = new byte[4 * 1024 + boundary.length];

        int first_fill = (b.remaining() < search_window.length) ? b.remaining() : search_window.length;
        b.get(search_window, 0, first_fill);
        int new_bytes = first_fill - boundary.length;

        do {
            // Search the search_window
            for (int j = 0; j < new_bytes; j++) {
                for (int i = 0; i < boundary.length; i++) {
                    if (search_window[j + i] != boundary[i])
                        break;
                    if (i == boundary.length - 1) {
                        // Match found, add it to results
                        int[] new_res = new int[res.length + 1];
                        System.arraycopy(res, 0, new_res, 0, res.length);
                        new_res[res.length] = search_window_pos + j;
                        res = new_res;
                    }
                }
            }
            search_window_pos += new_bytes;

            // Copy the end of the buffer to the start
            System.arraycopy(search_window, search_window.length - boundary.length, search_window, 0, boundary.length);

            // Refill search_window
            new_bytes = search_window.length - boundary.length;
            new_bytes = (b.remaining() < new_bytes) ? b.remaining() : new_bytes;
            b.get(search_window, boundary.length, new_bytes);
        } while (new_bytes > 0);
        return res;
    }

    @Override
    public CookieHandler getCookies() {
        return this.cookies;
    }

    @Override
    public final Map<String, String> getHeaders() {
        return this.headers;
    }

    @Override
    public final InputStream getInputStream() {
        return this.inputStream;
    }

    @Override
    public final Method getMethod() {
        return this.method;
    }

    /**
     * @deprecated use {@link #getParameters()} instead.
     */
    @Override
    @Deprecated
    public final Map<String, String> getParms() {
        Map<String, String> result = new HashMap<String, String>();
        for (String key : this.parms.keySet()) {
            result.put(key, this.parms.get(key).get(0));
        }

        return result;
    }

    @Override
    public final Map<String, List<String>> getParameters() {
        return this.parms;
    }

    @Override
    public String getQueryParameterString() {
        return this.queryParameterString;
    }

    private RandomAccessFile getTmpBucket() {
        try {
            ITempFile tempFile = this.tempFileManager.createTempFile(null);
            return new RandomAccessFile(tempFile.getName(), "rw");
        } catch (Exception e) {
            throw new Error(e); // we won't recover, so throw an error
        }
    }

    @Override
    public final String getUri() {
        return this.uri;
    }

    /**
     * Deduce body length in bytes. Either from "content-length" header or read
     * bytes.
     */
    public long getBodySize() {
        if (this.headers.containsKey("content-length")) {
            return Long.parseLong(this.headers.get("content-length"));
        } else if (this.splitbyte < this.rlen) {
            return this.rlen - this.splitbyte;
        }
        return 0;
    }

    @Override
    public void parseBody(Map<String, String> files) throws IOException, ResponseException {
        RandomAccessFile randomAccessFile = null;
        try {
            long size = getBodySize();
            ByteArrayOutputStream baos = null;
            DataOutput requestDataOutput = null;

            // Store the request in memory or a file, depending on size
            if (size < MEMORY_STORE_LIMIT) {
                baos = new ByteArrayOutputStream();
                requestDataOutput = new DataOutputStream(baos);
            } else {
                randomAccessFile = getTmpBucket();
                requestDataOutput = randomAccessFile;
            }

            // Read all the body and write it to request_data_output
            byte[] buf = new byte[REQUEST_BUFFER_LEN];
            while (this.rlen >= 0 && size > 0) {
                this.rlen = this.inputStream.read(buf, 0, (int) Math.min(size, REQUEST_BUFFER_LEN));
                size -= this.rlen;
                if (this.rlen > 0) {
                    requestDataOutput.write(buf, 0, this.rlen);
                }
            }

            ByteBuffer fbuf = null;
            if (baos != null) {
                fbuf = ByteBuffer.wrap(baos.toByteArray(), 0, baos.size());
            } else {
                fbuf = randomAccessFile.getChannel().map(FileChannel.MapMode.READ_ONLY, 0, randomAccessFile.length());
                randomAccessFile.seek(0);
            }

            // If the method is POST, there may be parameters
            // in data section, too, read it:
            if (Method.POST.equals(this.method)) {
                ContentType contentType = new ContentType(this.headers.get("content-type"));
                if (contentType.isMultipart()) {
                    String boundary = contentType.getBoundary();
                    if (boundary == null) {
                        throw new ResponseException(Status.BAD_REQUEST, "BAD REQUEST: Content type is multipart/form-data but boundary missing. Usage: GET /example/file.html");
                    }
                    decodeMultipartFormData(contentType, fbuf, this.parms, files);
                } else {
                    byte[] postBytes = new byte[fbuf.remaining()];
                    fbuf.get(postBytes);
                    String postLine = new String(postBytes, contentType.getEncoding()).trim();
                    // Handle application/x-www-form-urlencoded
                    if ("application/x-www-form-urlencoded".equalsIgnoreCase(contentType.getContentType())) {
                        decodeParms(postLine, this.parms);
                    } else if (postLine.length() != 0) {
                        // Special case for raw POST data => create a
                        // special files entry "postData" with raw content
                        // data
                        files.put(POST_DATA, postLine);
                    }
                }
            } else if (Method.PUT.equals(this.method)) {
                files.put("content", saveTmpFile(fbuf, 0, fbuf.limit(), null));
            }
        } finally {
            NanoHTTPD.safeClose(randomAccessFile);
        }
    }

    /**
     * Retrieves the content of a sent file and saves it to a temporary file.
     * The full path to the saved file is returned.
     */
    private String saveTmpFile(ByteBuffer b, int offset, int len, String filename_hint) {
        String path = "";
        if (len > 0) {
            FileOutputStream fileOutputStream = null;
            try {
                ITempFile tempFile = this.tempFileManager.createTempFile(filename_hint);
                ByteBuffer src = b.duplicate();
                fileOutputStream = new FileOutputStream(tempFile.getName());
                FileChannel dest = fileOutputStream.getChannel();
                src.position(offset).limit(offset + len);
                dest.write(src.slice());
                path = tempFile.getName();
            } catch (Exception e) { // Catch exception if any
                throw new Error(e); // we won't recover, so throw an error
            } finally {
                NanoHTTPD.safeClose(fileOutputStream);
            }
        }
        return path;
    }

    @Override
    public String getRemoteIpAddress() {
        return this.remoteIp;
    }
}
