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
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import com.facebook.TestUtils;
import com.facebook.internal.Utility;
import com.facebook.internal.ImageResponseCache;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URI;
import java.nio.ByteBuffer;
import java.util.Arrays;

public final class ImageResponseCacheTests extends AndroidTestCase {

    @MediumTest @LargeTest
    public void testImageCaching() throws Exception {
        // In unit test, since we need verify first access the image is not in cache
        // we need clear the cache first
        TestUtils.clearFileLruCache(ImageResponseCache.getCache(safeGetContext()));
        String imgUrl = "http://profile.ak.fbcdn.net/hprofile-ak-frc1/369438_100003049100322_615834658_n.jpg";
        
        Bitmap bmp1 = readImage(imgUrl, false);
        Bitmap bmp2 = readImage(imgUrl, true);
        compareImages(bmp1, bmp2);
    }
    
    @MediumTest @LargeTest
    public void testImageNotCaching() throws IOException {
        
        String imgUrl = "http://graph.facebook.com/ryanseacrest/picture?type=large";
        
        Bitmap bmp1 = readImage(imgUrl, false);
        Bitmap bmp2 = readImage(imgUrl, false);
        compareImages(bmp1, bmp2);
    }

    private Bitmap readImage(String uri, boolean expectedFromCache) {
        Bitmap bmp = null;
        InputStream istream = null;
        try
        {
            URI url = new URI(uri);
            // Check if the cache contains value for this url
            boolean isInCache = (ImageResponseCache.getCache(safeGetContext()).get(url.toString()) != null);
            assertTrue(isInCache == expectedFromCache);
            // Read the image
            istream = ImageResponseCache.getCachedImageStream(url, safeGetContext());
            if (istream == null) {
                HttpURLConnection connection = (HttpURLConnection)url.toURL().openConnection();
                istream = ImageResponseCache.interceptAndCacheImageStream(safeGetContext(), connection);
            }
            assertTrue(istream != null);
            bmp = BitmapFactory.decodeStream(istream);
            assertTrue(bmp != null);
        } catch (Exception e) {
             assertNull(e);
        } finally {
            Utility.closeQuietly(istream);
        }
        return bmp;
    }
    
    private static void compareImages(Bitmap bmp1, Bitmap bmp2) {
        assertTrue(bmp1.getHeight() == bmp2.getHeight());
        assertTrue(bmp1.getWidth() == bmp1.getWidth());
        ByteBuffer buffer1 = ByteBuffer.allocate(bmp1.getHeight() * bmp1.getRowBytes());
        bmp1.copyPixelsToBuffer(buffer1);

        ByteBuffer buffer2 = ByteBuffer.allocate(bmp2.getHeight() * bmp2.getRowBytes());
        bmp2.copyPixelsToBuffer(buffer2);

        assertTrue(Arrays.equals(buffer1.array(), buffer2.array()));
    }

    private Context safeGetContext() {
        for (;;) {
            if ((getContext() != null) && (getContext().getApplicationContext() != null)) {
                return getContext();
            }
            try {
                Thread.sleep(25);
            } catch (InterruptedException e) {
            }
        }
    }
}
