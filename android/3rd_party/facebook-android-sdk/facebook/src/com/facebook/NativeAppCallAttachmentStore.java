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

import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;
import com.facebook.internal.Utility;
import com.facebook.internal.Validate;

import java.io.*;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * <p>This class works in conjunction with {@link NativeAppCallContentProvider} to allow apps to attach binary
 * attachments (e.g., images) to native dialogs launched via the {@link com.facebook.widget.FacebookDialog}
 * class. It stores attachments in temporary files and allows the Facebook application to retrieve them via
 * the content provider.</p>
 *
 * <p>Callers are generally not expected to need to use this class directly;
 * see {@link com.facebook.widget.FacebookDialog.OpenGraphActionDialogBuilder#setImageAttachmentsForObject(String,
 * java.util.List) OpenGraphActionDialogBuilder.setImageAttachmentsForObject} for an example of a function
 * that will accept attachments, attach them to the native dialog call, and add them to the content provider
 * automatically.</p>
 **/
public final class NativeAppCallAttachmentStore implements NativeAppCallContentProvider.AttachmentDataSource {
    private static final String TAG = NativeAppCallAttachmentStore.class.getName();
    static final String ATTACHMENTS_DIR_NAME = "com.facebook.NativeAppCallAttachmentStore.files";
    private static File attachmentsDirectory;

    /**
     * Adds a number of bitmap attachments associated with a native app call. The attachments will be
     * served via {@link NativeAppCallContentProvider#openFile(android.net.Uri, String) openFile}.
     *
     * @param context the Context the call is being made from
     * @param callId the unique ID of the call
     * @param imageAttachments a Map of attachment names to Bitmaps; the attachment names will be part of
     *                         the URI processed by openFile
     * @throws java.io.IOException
     */
    public void addAttachmentsForCall(Context context, UUID callId, Map<String, Bitmap> imageAttachments) {
        Validate.notNull(context, "context");
        Validate.notNull(callId, "callId");
        Validate.containsNoNulls(imageAttachments.values(), "imageAttachments");
        Validate.containsNoNullOrEmpty(imageAttachments.keySet(), "imageAttachments");

        addAttachments(context, callId, imageAttachments, new ProcessAttachment<Bitmap>() {
            @Override
            public void processAttachment(Bitmap attachment, File outputFile) throws IOException {
                FileOutputStream outputStream = new FileOutputStream(outputFile);
                try {
                    attachment.compress(Bitmap.CompressFormat.JPEG, 100, outputStream);
                } finally {
                    Utility.closeQuietly(outputStream);
                }
            }
        });
    }

    /**
     * Adds a number of bitmap attachment files associated with a native app call. The attachments will be
     * served via {@link NativeAppCallContentProvider#openFile(android.net.Uri, String) openFile}.
     *
     * @param context the Context the call is being made from
     * @param callId the unique ID of the call
     * @param imageAttachments a Map of attachment names to Files containing the bitmaps; the attachment names will be
     *                         part of the URI processed by openFile
     * @throws java.io.IOException
     */
    public void addAttachmentFilesForCall(Context context, UUID callId, Map<String, File> imageAttachmentFiles) {
        Validate.notNull(context, "context");
        Validate.notNull(callId, "callId");
        Validate.containsNoNulls(imageAttachmentFiles.values(), "imageAttachmentFiles");
        Validate.containsNoNullOrEmpty(imageAttachmentFiles.keySet(), "imageAttachmentFiles");

        addAttachments(context, callId, imageAttachmentFiles, new ProcessAttachment<File>() {
            @Override
            public void processAttachment(File attachment, File outputFile) throws IOException {
                FileOutputStream outputStream = new FileOutputStream(outputFile);
                FileInputStream inputStream = null;
                try {
                    inputStream = new FileInputStream(attachment);

                    byte[] buffer = new byte[1024];
                    int len;
                    while ((len = inputStream.read(buffer)) > 0) {
                        outputStream.write(buffer, 0, len);
                    }
                } finally {
                    Utility.closeQuietly(outputStream);
                    Utility.closeQuietly(inputStream);
                }
            }
        });
    }

    private <T> void addAttachments(Context context, UUID callId, Map<String, T> attachments,
            ProcessAttachment<T> processor) {
        if (attachments.size() == 0) {
            return;
        }

        // If this is the first time we've been instantiated, clean up any existing attachments.
        if (attachmentsDirectory == null) {
            cleanupAllAttachments(context);
        }

        ensureAttachmentsDirectoryExists(context);

        List<File> filesToCleanup = new ArrayList<File>();

        try {
            for (Map.Entry<String, T> entry : attachments.entrySet()) {
                String attachmentName = entry.getKey();
                T attachment = entry.getValue();

                File file = getAttachmentFile(callId, attachmentName, true);
                filesToCleanup.add(file);

                processor.processAttachment(attachment, file);
            }
        } catch (IOException exception) {
            Log.e(TAG, "Got unexpected exception:" + exception);
            for (File file : filesToCleanup) {
                try {
                    file.delete();
                } catch (Exception e) {
                    // Always try to delete other files.
                }
            }
            throw new FacebookException(exception);
        }

    }

    interface ProcessAttachment<T> {
        void processAttachment(T attachment, File outputFile) throws IOException;
    }

    /**
     * Removes any temporary files associated with a particular native app call.
     *
     * @param context the Context the call is being made from
     * @param callId the unique ID of the call
     */
    public void cleanupAttachmentsForCall(Context context, UUID callId) {
        File dir = getAttachmentsDirectoryForCall(callId, false);
        Utility.deleteDirectory(dir);
    }

    @Override
    public File openAttachment(UUID callId, String attachmentName) throws FileNotFoundException {
        if (Utility.isNullOrEmpty(attachmentName) ||
                callId == null) {
            throw new FileNotFoundException();
        }

        try {
            return getAttachmentFile(callId, attachmentName, false);
        } catch (IOException e) {
            // We don't try to create the file, so we shouldn't get any IOExceptions. But if we do, just
            // act like the file wasn't found.
            throw new FileNotFoundException();
        }
    }

    synchronized static File getAttachmentsDirectory(Context context) {
        if (attachmentsDirectory == null) {
            attachmentsDirectory = new File(context.getCacheDir(), ATTACHMENTS_DIR_NAME);
        }
        return attachmentsDirectory;
    }

    File ensureAttachmentsDirectoryExists(Context context) {
        File dir = getAttachmentsDirectory(context);
        dir.mkdirs();
        return dir;
    }

    File getAttachmentsDirectoryForCall(UUID callId, boolean create) {
        if (attachmentsDirectory == null) {
            return null;
        }

        File dir = new File(attachmentsDirectory, callId.toString());
        if (create && !dir.exists()) {
            dir.mkdirs();
        }
        return dir;
    }

    File getAttachmentFile(UUID callId, String attachmentName, boolean createDirs) throws IOException {
        File dir = getAttachmentsDirectoryForCall(callId, createDirs);
        if (dir == null) {
            return null;
        }

        try {
            return new File(dir, URLEncoder.encode(attachmentName, "UTF-8"));
        } catch (UnsupportedEncodingException e) {
            return null;
        }
    }

    void cleanupAllAttachments(Context context) {
        // Attachments directory may or may not exist; we won't create it if not, since we are just going to delete it.
        File dir = getAttachmentsDirectory(context);
        Utility.deleteDirectory(dir);
    }
}
