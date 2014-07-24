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

package com.facebook.widget;

import android.graphics.Bitmap;
import com.facebook.FacebookException;
import com.facebook.FacebookTestCase;
import com.facebook.model.GraphObject;
import com.facebook.model.OpenGraphAction;
import com.facebook.model.OpenGraphObject;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class FacebookDialogTests extends FacebookTestCase {

    private String getAttachmentNameFromContentUri(String contentUri) {
        int lastSlash = contentUri.lastIndexOf("/");
        return contentUri.substring(lastSlash + 1);
    }

    public void testCantSetAttachmentsWithNullBitmaps() {
        try {
            OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
            action.setProperty("foo", "bar");

            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "foo");

            builder.setImageAttachmentsForAction(Arrays.asList((Bitmap)null));
            fail("expected exception");
        } catch (NullPointerException exception) {
        }
    }

    public void testOpenGraphActionImageAttachments() throws JSONException {
        OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
        action.setProperty("foo", "bar");

        FacebookDialog.OpenGraphActionDialogBuilder builder =
                new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "foo");

        Bitmap bitmap = Bitmap.createBitmap(20, 20, Bitmap.Config.ALPHA_8);

        builder.setImageAttachmentsForAction(Arrays.asList(bitmap));

        List<JSONObject> images = action.getImage();
        assertNotNull(images);
        assertTrue(images.size() == 1);

        List<String> attachmentNames = builder.getImageAttachmentNames();
        assertNotNull(attachmentNames);
        assertTrue(attachmentNames.size() == 1);

        String attachmentName = getAttachmentNameFromContentUri(images.get(0).getString("url"));
        assertEquals(attachmentNames.get(0), attachmentName);
    }

    public void testCantSetObjectAttachmentsWithoutAction() {
        try {
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), null, "foo");
            builder.setImageAttachmentsForObject("foo", new ArrayList<Bitmap>());
            fail("expected exception");
        } catch (NullPointerException exception) {
        }
    }

    public void testCantSetObjectAttachmentsWithoutObjectProperty() {
        try {
            OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "foo");

            builder.setImageAttachmentsForObject("foo", new ArrayList<Bitmap>());
            fail("expected exception");
        } catch (IllegalArgumentException exception) {
        }
    }

    public void testCantSetObjectAttachmentsWithNonGraphObjectProperty() {
        try {
            OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "foo");

            action.setProperty("foo", "bar");

            builder.setImageAttachmentsForObject("foo", new ArrayList<Bitmap>());
            fail("expected exception");
        } catch (IllegalArgumentException exception) {
        }
    }

    public void testCantSetObjectAttachmentsWithNullBitmaps() {
        try {
            OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
            action.setProperty("foo", OpenGraphObject.Factory.createForPost("bar"));

            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "foo");

            builder.setImageAttachmentsForObject("foo", Arrays.asList((Bitmap)null));
            fail("expected exception");
        } catch (NullPointerException exception) {
        }
    }

    public void testOpenGraphObjectImageAttachments() throws JSONException {
        OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
        OpenGraphObject object = OpenGraphObject.Factory.createForPost("bar");
        action.setProperty("foo", object);

        FacebookDialog.OpenGraphActionDialogBuilder builder =
                new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "foo");

        Bitmap bitmap = Bitmap.createBitmap(20, 20, Bitmap.Config.ALPHA_8);

        builder.setImageAttachmentsForObject("foo", Arrays.asList(bitmap));

        List<GraphObject> images = object.getImage();
        assertNotNull(images);
        assertTrue(images.size() == 1);

        List<String> attachmentNames = builder.getImageAttachmentNames();
        assertNotNull(attachmentNames);
        assertTrue(attachmentNames.size() == 1);

        String attachmentName = getAttachmentNameFromContentUri((String) images.get(0).getProperty("url"));
        assertEquals(attachmentNames.get(0), attachmentName);
    }

    public void testOpenGraphActionAndObjectImageAttachments() throws JSONException {
        OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
        OpenGraphObject object = OpenGraphObject.Factory.createForPost("bar");
        action.setProperty("foo", object);

        FacebookDialog.OpenGraphActionDialogBuilder builder =
                new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "foo");

        Bitmap bitmap = Bitmap.createBitmap(20, 20, Bitmap.Config.ALPHA_8);

        builder.setImageAttachmentsForAction(Arrays.asList(bitmap));
        builder.setImageAttachmentsForObject("foo", Arrays.asList(bitmap));

        List<String> attachmentNames = builder.getImageAttachmentNames();
        assertNotNull(attachmentNames);
        assertTrue(attachmentNames.size() == 2);

        List<GraphObject> objectImages = object.getImage();
        assertNotNull(objectImages);
        assertTrue(objectImages.size() == 1);

        String attachmentName = getAttachmentNameFromContentUri((String) objectImages.get(0).getProperty("url"));
        assertTrue(attachmentNames.contains(attachmentName));

        List<JSONObject> actionImages = action.getImage();
        assertNotNull(actionImages);
        assertTrue(actionImages.size() == 1);

        attachmentName = getAttachmentNameFromContentUri((String) actionImages.get(0).getString("url"));
        assertTrue(attachmentNames.contains(attachmentName));
    }

    public void testOpenGraphDialogBuilderRequiresAction() {
        try {
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), null, "foo");

            builder.build();
            fail("expected exception");
        } catch (NullPointerException exception) {
        }
    }

    public void testOpenGraphDialogBuilderRequiresActionType() {
        try {
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(),
                            OpenGraphAction.Factory.createForPost(null), "foo");

            builder.build();
            fail("expected exception");
        } catch (IllegalArgumentException exception) {
        }
    }

    public void testOpenGraphDialogBuilderRequiresPreviewPropertyName() {
        try {
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(),
                            OpenGraphAction.Factory.createForPost("foo"), null);

            builder.build();
            fail("expected exception");
        } catch (IllegalArgumentException exception) {
        }
    }

    public void testOpenGraphDialogBuilderRequiresPreviewPropertyToExist() {
        try {
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(),
                            OpenGraphAction.Factory.createForPost("foo"), "nosuchproperty");

            builder.build();
            fail("expected exception");
        } catch (IllegalArgumentException exception) {
        }
    }

    @SuppressWarnings("deprecation")
    public void testOpenGraphDialogBuilderDeprecatedConstructorRequiresActionType() {
        try {
            OpenGraphAction action = OpenGraphAction.Factory.createForPost();
            OpenGraphObject object = OpenGraphObject.Factory.createForPost("bar");
            action.setProperty("object", object);
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "", "object");

            builder.build();
            fail("expected exception");
        } catch (IllegalArgumentException exception) {
        }

    }

    @SuppressWarnings("deprecation")
    public void testOpenGraphDialogBuilderDeprecatedConstructorRequiresActionTypeMatches() {
        try {
            OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
            OpenGraphObject object = OpenGraphObject.Factory.createForPost("bar");
            action.setProperty("object", object);
            FacebookDialog.OpenGraphActionDialogBuilder builder =
                    new FacebookDialog.OpenGraphActionDialogBuilder(getActivity(), action, "notfoo", "object");

            builder.build();
            fail("expected exception");
        } catch (IllegalArgumentException exception) {
        }
    }
}
