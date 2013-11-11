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

package com.facebook.internal;

import android.content.Context;
import android.util.Log;
import com.facebook.LoggingBehavior;
import com.facebook.Settings;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.io.*;
import java.security.InvalidParameterException;
import java.util.Date;
import java.util.PriorityQueue;
import java.util.concurrent.atomic.AtomicLong;

// This class is intended to be thread-safe.
//
// There are two classes of files:  buffer files and cache files:
// - A buffer file is in the process of being written, and there is an open stream on the file.  These files are
//   named as "bufferN" where N is an incrementing integer.  On startup, we delete all existing files of this form.
//   Once the stream is closed, we rename the buffer file to a cache file or attempt to delete if this fails.  We
//   do not otherwise ever attempt to delete these files.
// - A cache file is a non-changing file that is named by the md5 hash of the cache key.  We monitor the size of
//   these files in aggregate and remove the oldest one(s) to stay under quota.  This process does not block threads
//   calling into this class, so theoretically we could go arbitrarily over quota but in practice this should not
//   happen because deleting files should be much cheaper than downloading new file content.
//
// Since there can only ever be one thread accessing a particular buffer file, we do not synchronize access to these.
// We do assume that file rename is atomic when converting a buffer file to a cache file, and that if multiple files
// are renamed to a single target that exactly one of them continues to exist.
//
// Standard POSIX file semantics guarantee being able to continue to use a file handle even after the
// corresponding file has been deleted.  Given this and that cache files never change other than deleting in trim()
// or clear(),  we only have to ensure that there is at most one trim() or clear() process deleting files at any
// given time.

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
public final class FileLruCache {
    static final String TAG = FileLruCache.class.getSimpleName();
    private static final String HEADER_CACHEKEY_KEY = "key";
    private static final String HEADER_CACHE_CONTENT_TAG_KEY = "tag";

    private static final AtomicLong bufferIndex = new AtomicLong();

    private final String tag;
    private final Limits limits;
    private final File directory;
    private boolean isTrimPending;
    private final Object lock;
    private AtomicLong lastClearCacheTime = new AtomicLong(0);

    // The value of tag should be a final String that works as a directory name.
    public FileLruCache(Context context, String tag, Limits limits) {
        this.tag = tag;
        this.limits = limits;
        this.directory = new File(context.getCacheDir(), tag);
        this.lock = new Object();

        // Ensure the cache dir exists
        if (this.directory.mkdirs() || this.directory.isDirectory()) {
            // Remove any stale partially-written files from a previous run
            BufferFile.deleteAll(this.directory);
        }
    }

    // This is not robust to files changing dynamically underneath it and should therefore only be used
    // for test code.  If we ever need this for product code we need to think through synchronization.
    // See the threading notes at the top of this class.
    //
    // Also, since trim() runs asynchronously now, this blocks until any pending trim has completed.
    long sizeInBytesForTest() {
        synchronized (lock) {
            while (isTrimPending) {
                try {
                    lock.wait();
                } catch (InterruptedException e) {
                    // intentional no-op
                }
            }
        }

        File[] files = this.directory.listFiles();
        long total = 0;
        if (files != null) {
            for (File file : files) {
                total += file.length();
            }
        }
        return total;
    }

    public InputStream get(String key) throws IOException {
        return get(key, null);
    }

    public InputStream get(String key, String contentTag) throws IOException {
        File file = new File(this.directory, Utility.md5hash(key));

        FileInputStream input = null;
        try {
            input = new FileInputStream(file);
        } catch (IOException e) {
            return null;
        }

        BufferedInputStream buffered = new BufferedInputStream(input, Utility.DEFAULT_STREAM_BUFFER_SIZE);
        boolean success = false;

        try {
            JSONObject header = StreamHeader.readHeader(buffered);
            if (header == null) {
                return null;
            }

            String foundKey = header.optString(HEADER_CACHEKEY_KEY);
            if ((foundKey == null) || !foundKey.equals(key)) {
                return null;
            }

            String headerContentTag = header.optString(HEADER_CACHE_CONTENT_TAG_KEY, null);

            if ((contentTag == null && headerContentTag != null) ||
                    (contentTag != null && !contentTag.equals(headerContentTag))) {
                return null;
            }

            long accessTime = new Date().getTime();
            Logger.log(LoggingBehavior.CACHE, TAG, "Setting lastModified to " + Long.valueOf(accessTime) + " for "
                    + file.getName());
            file.setLastModified(accessTime);

            success = true;
            return buffered;
        } finally {
            if (!success) {
                buffered.close();
            }
        }
    }

    OutputStream openPutStream(final String key) throws IOException {
        return openPutStream(key, null);
    }

    public OutputStream openPutStream(final String key, String contentTag) throws IOException {
        final File buffer = BufferFile.newFile(this.directory);
        buffer.delete();
        if (!buffer.createNewFile()) {
            throw new IOException("Could not create file at " + buffer.getAbsolutePath());
        }

        FileOutputStream file = null;
        try {
            file = new FileOutputStream(buffer);
        } catch (FileNotFoundException e) {
            Logger.log(LoggingBehavior.CACHE, Log.WARN, TAG, "Error creating buffer output stream: " + e);
            throw new IOException(e.getMessage());
        }

        final long bufferFileCreateTime = System.currentTimeMillis();
        StreamCloseCallback renameToTargetCallback = new StreamCloseCallback() {
            @Override
            public void onClose() {
                // if the buffer file was created before the cache was cleared, then the buffer file
                // should be deleted rather than renamed and saved.
                if (bufferFileCreateTime < lastClearCacheTime.get()) {
                    buffer.delete();
                } else {
                    renameToTargetAndTrim(key, buffer);
                }
            }
        };

        CloseCallbackOutputStream cleanup = new CloseCallbackOutputStream(file, renameToTargetCallback);
        BufferedOutputStream buffered = new BufferedOutputStream(cleanup, Utility.DEFAULT_STREAM_BUFFER_SIZE);
        boolean success = false;

        try {
            // Prefix the stream with the actual key, since there could be collisions
            JSONObject header = new JSONObject();
            header.put(HEADER_CACHEKEY_KEY, key);
            if (!Utility.isNullOrEmpty(contentTag)) {
                header.put(HEADER_CACHE_CONTENT_TAG_KEY, contentTag);
            }

            StreamHeader.writeHeader(buffered, header);

            success = true;
            return buffered;
        } catch (JSONException e) {
            // JSON is an implementation detail of the cache, so don't let JSON exceptions out.
            Logger.log(LoggingBehavior.CACHE, Log.WARN, TAG, "Error creating JSON header for cache file: " + e);
            throw new IOException(e.getMessage());
        } finally {
            if (!success) {
                buffered.close();
            }
        }
    }

    public void clearCache() {
        // get the current directory listing of files to delete
        final File[] filesToDelete = directory.listFiles(BufferFile.excludeBufferFiles());
        lastClearCacheTime.set(System.currentTimeMillis());
        if (filesToDelete != null) {
            Settings.getExecutor().execute(new Runnable() {
                @Override
                public void run() {
                    for (File file : filesToDelete) {
                        file.delete();
                    }
                }
            });
        }
    }

    private void renameToTargetAndTrim(String key, File buffer) {
        final File target = new File(directory, Utility.md5hash(key));

        // This is triggered by close().  By the time close() returns, the file should be cached, so this needs to
        // happen synchronously on this thread.
        //
        // However, it does not need to be synchronized, since in the race we will just start an unnecesary trim
        // operation.  Avoiding the cost of holding the lock across the file operation seems worth this cost.
        if (!buffer.renameTo(target)) {
            buffer.delete();
        }

        postTrim();
    }

    // Opens an output stream for the key, and creates an input stream wrapper to copy
    // the contents of input into the new output stream.  The effect is to store a
    // copy of input, and associate that data with key.
    public InputStream interceptAndPut(String key, InputStream input) throws IOException {
        OutputStream output = openPutStream(key);
        return new CopyingInputStream(input, output);
    }

    public String toString() {
        return "{FileLruCache:" + " tag:" + this.tag + " file:" + this.directory.getName() + "}";
    }

    private void postTrim() {
        synchronized (lock) {
            if (!isTrimPending) {
                isTrimPending = true;
                Settings.getExecutor().execute(new Runnable() {
                    @Override
                    public void run() {
                        trim();
                    }
                });
            }
        }
    }

    private void trim() {
        try {
            Logger.log(LoggingBehavior.CACHE, TAG, "trim started");
            PriorityQueue<ModifiedFile> heap = new PriorityQueue<ModifiedFile>();
            long size = 0;
            long count = 0;
            File[] filesToTrim =this.directory.listFiles(BufferFile.excludeBufferFiles());
            if (filesToTrim != null) {
                for (File file : filesToTrim) {
                    ModifiedFile modified = new ModifiedFile(file);
                    heap.add(modified);
                    Logger.log(LoggingBehavior.CACHE, TAG, "  trim considering time=" + Long.valueOf(modified.getModified())
                            + " name=" + modified.getFile().getName());

                    size += file.length();
                    count++;
                }
            }

            while ((size > limits.getByteCount()) || (count > limits.getFileCount())) {
                File file = heap.remove().getFile();
                Logger.log(LoggingBehavior.CACHE, TAG, "  trim removing " + file.getName());
                size -= file.length();
                count--;
                file.delete();
            }
        } finally {
            synchronized (lock) {
                isTrimPending = false;
                lock.notifyAll();
            }
        }
    }

    private static class BufferFile {
        private static final String FILE_NAME_PREFIX = "buffer";
        private static final FilenameFilter filterExcludeBufferFiles = new FilenameFilter() {
            @Override
            public boolean accept(File dir, String filename) {
                return !filename.startsWith(FILE_NAME_PREFIX);
            }
        };
        private static final FilenameFilter filterExcludeNonBufferFiles = new FilenameFilter() {
            @Override
            public boolean accept(File dir, String filename) {
                return filename.startsWith(FILE_NAME_PREFIX);
            }
        };

        static void deleteAll(final File root) {
            File[] filesToDelete = root.listFiles(excludeNonBufferFiles());
            if (filesToDelete != null) {
                for (File file : filesToDelete) {
                    file.delete();
                }
            }
        }

        static FilenameFilter excludeBufferFiles() {
            return filterExcludeBufferFiles;
        }

        static FilenameFilter excludeNonBufferFiles() {
            return filterExcludeNonBufferFiles;
        }

        static File newFile(final File root) {
            String name = FILE_NAME_PREFIX + Long.valueOf(bufferIndex.incrementAndGet()).toString();
            return new File(root, name);
        }
    }

    // Treats the first part of a stream as a header, reads/writes it as a JSON blob, and
    // leaves the stream positioned exactly after the header.
    //
    // The format is as follows:
    //     byte: meaning
    // ---------------------------------
    //        0: version number
    //      1-3: big-endian JSON header blob size
    // 4-size+4: UTF-8 JSON header blob
    //      ...: stream data
    private static final class StreamHeader {
        private static final int HEADER_VERSION = 0;

        static void writeHeader(OutputStream stream, JSONObject header) throws IOException {
            String headerString = header.toString();
            byte[] headerBytes = headerString.getBytes();

            // Write version number and big-endian header size
            stream.write(HEADER_VERSION);
            stream.write((headerBytes.length >> 16) & 0xff);
            stream.write((headerBytes.length >> 8) & 0xff);
            stream.write((headerBytes.length >> 0) & 0xff);

            stream.write(headerBytes);
        }

        static JSONObject readHeader(InputStream stream) throws IOException {
            int version = stream.read();
            if (version != HEADER_VERSION) {
                return null;
            }

            int headerSize = 0;
            for (int i = 0; i < 3; i++) {
                int b = stream.read();
                if (b == -1) {
                    Logger.log(LoggingBehavior.CACHE, TAG,
                            "readHeader: stream.read returned -1 while reading header size");
                    return null;
                }
                headerSize <<= 8;
                headerSize += b & 0xff;
            }

            byte[] headerBytes = new byte[headerSize];
            int count = 0;
            while (count < headerBytes.length) {
                int readCount = stream.read(headerBytes, count, headerBytes.length - count);
                if (readCount < 1) {
                    Logger.log(LoggingBehavior.CACHE, TAG,
                            "readHeader: stream.read stopped at " + Integer.valueOf(count) + " when expected "
                                    + headerBytes.length);
                    return null;
                }
                count += readCount;
            }

            String headerString = new String(headerBytes);
            JSONObject header = null;
            JSONTokener tokener = new JSONTokener(headerString);
            try {
                Object parsed = tokener.nextValue();
                if (!(parsed instanceof JSONObject)) {
                    Logger.log(LoggingBehavior.CACHE, TAG, "readHeader: expected JSONObject, got " + parsed.getClass().getCanonicalName());
                    return null;
                }
                header = (JSONObject) parsed;
            } catch (JSONException e) {
                throw new IOException(e.getMessage());
            }

            return header;
        }
    }

    private static class CloseCallbackOutputStream extends OutputStream {
        final OutputStream innerStream;
        final StreamCloseCallback callback;

        CloseCallbackOutputStream(OutputStream innerStream, StreamCloseCallback callback) {
            this.innerStream = innerStream;
            this.callback = callback;
        }

        @Override
        public void close() throws IOException {
            try {
                this.innerStream.close();
            } finally {
                this.callback.onClose();
            }
        }

        @Override
        public void flush() throws IOException {
            this.innerStream.flush();
        }

        @Override
        public void write(byte[] buffer, int offset, int count) throws IOException {
            this.innerStream.write(buffer, offset, count);
        }

        @Override
        public void write(byte[] buffer) throws IOException {
            this.innerStream.write(buffer);
        }

        @Override
        public void write(int oneByte) throws IOException {
            this.innerStream.write(oneByte);
        }
    }

    private static final class CopyingInputStream extends InputStream {
        final InputStream input;
        final OutputStream output;

        CopyingInputStream(final InputStream input, final OutputStream output) {
            this.input = input;
            this.output = output;
        }

        @Override
        public int available() throws IOException {
            return input.available();
        }

        @Override
        public void close() throws IOException {
            // According to http://www.cs.cornell.edu/andru/javaspec/11.doc.html:
            //  "If a finally clause is executed because of abrupt completion of a try block and the finally clause
            //   itself completes abruptly, then the reason for the abrupt completion of the try block is discarded
            //   and the new reason for abrupt completion is propagated from there."
            //
            // Android does appear to behave like this.
            try {
                this.input.close();
            } finally {
                this.output.close();
            }
        }

        @Override
        public void mark(int readlimit) {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean markSupported() {
            return false;
        }

        @Override
        public int read(byte[] buffer) throws IOException {
            int count = input.read(buffer);
            if (count > 0) {
                output.write(buffer, 0, count);
            }
            return count;
        }

        @Override
        public int read() throws IOException {
            int b = input.read();
            if (b >= 0) {
                output.write(b);
            }
            return b;
        }

        @Override
        public int read(byte[] buffer, int offset, int length) throws IOException {
            int count = input.read(buffer, offset, length);
            if (count > 0) {
                output.write(buffer, offset, count);
            }
            return count;
        }

        @Override
        public synchronized void reset() {
            throw new UnsupportedOperationException();
        }

        @Override
        public long skip(long byteCount) throws IOException {
            byte[] buffer = new byte[1024];
            long total = 0;
            while (total < byteCount) {
                int count = read(buffer, 0, (int)Math.min(byteCount - total, buffer.length));
                if (count < 0) {
                    return total;
                }
                total += count;
            }
            return total;
        }
    }

    public static final class Limits {
        private int byteCount;
        private int fileCount;

        public Limits() {
            // A Samsung Galaxy Nexus can create 1k files in half a second.  By the time
            // it gets to 5k files it takes 5 seconds.  10k files took 15 seconds.  This
            // continues to slow down as files are added.  This assumes all files are in
            // a single directory.
            //
            // Following a git-like strategy where we partition MD5-named files based on
            // the first 2 characters is slower across the board.
            this.fileCount = 1024;
            this.byteCount = 1024 * 1024;
        }

        int getByteCount() {
            return byteCount;
        }

        int getFileCount() {
            return fileCount;
        }

        void setByteCount(int n) {
            if (n < 0) {
                throw new InvalidParameterException("Cache byte-count limit must be >= 0");
            }
            byteCount = n;
        }

        void setFileCount(int n) {
            if (n < 0) {
                throw new InvalidParameterException("Cache file count limit must be >= 0");
            }
            fileCount = n;
        }
    }

    // Caches the result of lastModified during sort/heap operations
    private final static class ModifiedFile implements Comparable<ModifiedFile> {
        private static final int HASH_SEED = 29; // Some random prime number
        private static final int HASH_MULTIPLIER = 37; // Some random prime number

        private final File file;
        private final long modified;

        ModifiedFile(File file) {
            this.file = file;
            this.modified = file.lastModified();
        }

        File getFile() {
            return file;
        }

        long getModified() {
            return modified;
        }

        @Override
        public int compareTo(ModifiedFile another) {
            if (getModified() < another.getModified()) {
                return -1;
            } else if (getModified() > another.getModified()) {
                return 1;
            } else {
                return getFile().compareTo(another.getFile());
            }
        }

        @Override
        public boolean equals(Object another) {
            return
                    (another instanceof ModifiedFile) &&
                    (compareTo((ModifiedFile)another) == 0);
        }

        @Override
        public int hashCode() {
            int result = HASH_SEED;

            result = (result * HASH_MULTIPLIER) + file.hashCode();
            result = (result * HASH_MULTIPLIER) + (int) (modified % Integer.MAX_VALUE);

            return result;
        }
    }

    private interface StreamCloseCallback {
        void onClose();
    }
}
