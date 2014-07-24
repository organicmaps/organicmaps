package com.facebook;

import android.graphics.Bitmap;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class NativeAppCallAttachmentStoreTest extends FacebookTestCase {
    private static final UUID CALL_ID = UUID.randomUUID();
    private static final String ATTACHMENT_NAME = "hello";

    private NativeAppCallAttachmentStore storeUnderTest;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        storeUnderTest = new NativeAppCallAttachmentStore();
    }

    private Bitmap createBitmap() {
        Bitmap bitmap = Bitmap.createBitmap(20, 20, Bitmap.Config.ALPHA_8);
        return bitmap;
    }

    private Map<String, Bitmap> createValidAttachment() {
        String attachmentId = UUID.randomUUID().toString();
        Bitmap bitmap = createBitmap();

        Map<String, Bitmap> bitmaps = new HashMap<String, Bitmap>();
        bitmaps.put(attachmentId, bitmap);

        return bitmaps;
    }

    public void testAddAttachmentsForCallWithNullContext() throws Exception {
        try {
            Map<String, Bitmap> attachments = createValidAttachment();
            storeUnderTest.addAttachmentsForCall(null, CALL_ID, attachments);
            fail("expected exception");
        } catch (NullPointerException ex) {
            assertTrue(ex.getMessage().contains("context"));
        }
    }

    public void testAddAttachmentsForCallWithNullCallId() throws Exception {
        try {
            Map<String, Bitmap> attachments = createValidAttachment();
            storeUnderTest.addAttachmentsForCall(getActivity(), null, attachments);
            fail("expected exception");
        } catch (NullPointerException ex) {
            assertTrue(ex.getMessage().contains("callId"));
        }
    }

    public void testAddAttachmentsForCallWithNullBitmap() throws Exception {
        try {
            Map<String, Bitmap> attachments = new HashMap<String, Bitmap>();
            attachments.put(ATTACHMENT_NAME, null);

            storeUnderTest.addAttachmentsForCall(getActivity(), CALL_ID, attachments);
            fail("expected exception");
        } catch (NullPointerException ex) {
            assertTrue(ex.getMessage().contains("imageAttachments"));
        }
    }

    public void testAddAttachmentsForCallWithEmptyAttachmentName() throws Exception {
        try {
            Map<String, Bitmap> attachments = new HashMap<String, Bitmap>();
            attachments.put("", createBitmap());

            storeUnderTest.addAttachmentsForCall(getActivity(), CALL_ID, attachments);
            fail("expected exception");
        } catch (IllegalArgumentException ex) {
            assertTrue(ex.getMessage().contains("imageAttachments"));
        }
    }

    public void testAddAttachmentsForCall() throws Exception {

    }

    public void testCleanupAttachmentsForCall() throws Exception {

    }

    public void testGetAttachmentsDirectory() throws Exception {
        File dir = NativeAppCallAttachmentStore.getAttachmentsDirectory(getActivity());
        assertNotNull(dir);
        assertTrue(dir.getAbsolutePath().contains(NativeAppCallAttachmentStore.ATTACHMENTS_DIR_NAME));
    }

    public void testGetAttachmentsDirectoryForCall() throws Exception {
        storeUnderTest.ensureAttachmentsDirectoryExists(getActivity());
        File dir = storeUnderTest.getAttachmentsDirectoryForCall(CALL_ID, false);
        assertNotNull(dir);
        assertTrue(dir.getAbsolutePath().contains(NativeAppCallAttachmentStore.ATTACHMENTS_DIR_NAME));
        assertTrue(dir.getAbsolutePath().contains(CALL_ID.toString()));
    }

    public void testGetAttachmentFile() throws Exception {
        storeUnderTest.ensureAttachmentsDirectoryExists(getActivity());
        File dir = storeUnderTest.getAttachmentFile(CALL_ID, ATTACHMENT_NAME, false);
        assertNotNull(dir);
        assertTrue(dir.getAbsolutePath().contains(NativeAppCallAttachmentStore.ATTACHMENTS_DIR_NAME));
        assertTrue(dir.getAbsolutePath().contains(CALL_ID.toString()));
        assertTrue(dir.getAbsolutePath().contains(ATTACHMENT_NAME.toString()));
    }
}
