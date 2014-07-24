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

package com.facebook;

import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.util.Pair;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class NativeAppCallContentProviderTest extends FacebookTestCase {
    private static final String APP_ID = "12345";
    private static final UUID CALL_ID = UUID.randomUUID();

    private NativeAppCallContentProvider providerUnderTest;
    private StubAttachmentStore stubAttachmentStore;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        stubAttachmentStore = new StubAttachmentStore();
        providerUnderTest = new NativeAppCallContentProvider(stubAttachmentStore);
    }

    public void testGetAttachmentUrl() {
        String url = NativeAppCallContentProvider.getAttachmentUrl(APP_ID, CALL_ID, "foo");
        assertEquals("content://com.facebook.app.NativeAppCallContentProvider" + APP_ID + "/" + CALL_ID + "/foo", url);
    }

    public void testOnCreate() throws Exception {
        assertTrue(providerUnderTest.onCreate());
    }

    public void testQuery() throws Exception {
        assertNull(providerUnderTest.query(null, null, null, null, null));
    }

    public void testGetType() throws Exception {
        assertNull(providerUnderTest.getType(null));
    }

    public void testInsert() throws Exception {
        assertNull(providerUnderTest.insert(null, null));
    }

    public void testDelete() throws Exception {
        assertEquals(0, providerUnderTest.delete(null, null, null));
    }

    public void testUpdate() throws Exception {
        assertEquals(0, providerUnderTest.update(null, null, null, null));
    }

    @SuppressWarnings("unused")
    public void testOpenFileWithNullUri() throws Exception {
        try {
            ParcelFileDescriptor pfd = providerUnderTest.openFile(null, "r");
            fail("expected FileNotFoundException");
        } catch (FileNotFoundException e) {
        }
    }

    @SuppressWarnings("unused")
    public void testOpenFileWithBadPath() throws Exception {
        try {
            ParcelFileDescriptor pfd = providerUnderTest.openFile(Uri.parse("/"), "r");
            fail("expected FileNotFoundException");
        } catch (FileNotFoundException e) {
        }
    }

    @SuppressWarnings("unused")
    public void testOpenFileWithoutCallIdAndAttachment() throws Exception {
        try {
            ParcelFileDescriptor pfd = providerUnderTest.openFile(Uri.parse("/foo"), "r");
            fail("expected FileNotFoundException");
        } catch (FileNotFoundException e) {
        }
    }

    @SuppressWarnings("unused")
    public void testOpenFileWithBadCallID() throws Exception {
        try {
            ParcelFileDescriptor pfd = providerUnderTest.openFile(Uri.parse("/foo/bar"), "r");
            fail("expected FileNotFoundException");
        } catch (FileNotFoundException e) {
        }
    }

    public void testOpenFileWithUnknownUri() throws Exception {
        try {
            String callId = UUID.randomUUID().toString();
            ParcelFileDescriptor pfd = providerUnderTest.openFile(Uri.parse("/" + callId + "/bar"), "r");
            fail("expected FileNotFoundException");
        } catch (FileNotFoundException e) {
        }
    }

    public void testOpenFileWithKnownUri() throws Exception {
        String attachmentName = "hi";

        stubAttachmentStore.addAttachment(CALL_ID, attachmentName);
        Uri uri = Uri.parse(NativeAppCallContentProvider.getAttachmentUrl(APP_ID, CALL_ID, attachmentName));

        ParcelFileDescriptor pfd = providerUnderTest.openFile(uri, "r");

        assertNotNull(pfd);
        pfd.close();
    }

    class StubAttachmentStore implements NativeAppCallContentProvider.AttachmentDataSource {
        private List<Pair<UUID, String>> attachments = new ArrayList<Pair<UUID, String>>();
        private static final String DUMMY_FILE_NAME = "dummyfile";

        public void addAttachment(UUID callId, String attachmentName) {
            attachments.add(new Pair<UUID, String>(callId, attachmentName));
        }

        @Override
        public File openAttachment(UUID callId, String attachmentName) throws FileNotFoundException {
            if (attachments.contains(new Pair<UUID, String>(callId, attachmentName))) {
                File cacheDir = getActivity().getCacheDir();
                File dummyFile = new File(cacheDir, DUMMY_FILE_NAME);
                if (!dummyFile.exists()) {
                    try {
                        dummyFile.createNewFile();
                    } catch (IOException e) {
                    }
                }

                return dummyFile;
            }
            throw new FileNotFoundException();
        }
    }
}
