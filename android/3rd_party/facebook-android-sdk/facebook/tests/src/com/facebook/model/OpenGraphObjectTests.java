package com.facebook.model;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.Date;

public final class OpenGraphObjectTests extends AndroidTestCase {
    private static String OBJECT_JSON = "{" +
            "  \"id\": \"509\"," +
            "  \"url\": \"http://www.example.com/100\"," +
            "  \"type\": \"awesome:thing\"," +
            "  \"title\": \"awesome title\"," +
            "  \"data\": {" +
            "    \"color\": \"blue\"" +
            "  }," +
            "  \"image\": [" +
            "    {" +
            "      \"url\": \"http://www.example.com/images/81\"" +
            "    }" +
            "  ]," +
            "  \"video\": [" +
            "    {" +
            "      \"url\": \"http://www.example.com/videos/18\"" +
            "    }" +
            "  ]," +
            "  \"audio\": [" +
            "    {" +
            "      \"url\": \"http://www.example.com/audio/98\"" +
            "    }" +
            "  ]," +
            "  \"description\": \"a description\"," +
            "  \"see_also\": [" +
            "    \"http://www.example.com/101\"" +
            "  ]," +
            "  \"site_name\": \"Awesome Site\"," +
            "  \"updated_time\": \"2013-04-30T18:18:17+0000\"," +
            "  \"created_time\": \"2013-04-30T18:18:17+0000\"," +
            "  \"application\": {" +
            "    \"id\": \"55\"," +
            "    \"name\": \"Awesome App\"," +
            "    \"url\": \"https://www.facebook.com/apps/application.php?id=55\"" +
            "  }," +
            "  \"is_scraped\": false," +
            "  \"post_action_id\": \"1234\"" +
            "}";

    interface TestOpenGraphObjectData extends GraphObject {
        String getColor();
    }

    interface TestOpenGraphObject extends GraphObject {
        TestOpenGraphObjectData getData();
    }

    public OpenGraphObject getParsedObject() throws JSONException {
        JSONObject jsonObject = new JSONObject(OBJECT_JSON);
        return GraphObject.Factory.create(jsonObject, OpenGraphObject.class);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedId() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals("509", parsedObject.getId());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedType() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals("awesome:thing", parsedObject.getType());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedUrl() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals("http://www.example.com/100", parsedObject.getUrl());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedTitle() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals("awesome title", parsedObject.getTitle());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedDescription() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals("a description", parsedObject.getDescription());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedImage() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        GraphObjectList<GraphObject> images = parsedObject.getImage();
        assertEquals("http://www.example.com/images/81", images.get(0).getProperty("url"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedVideo() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        GraphObjectList<GraphObject> videos = parsedObject.getVideo();
        assertEquals("http://www.example.com/videos/18", videos.get(0).getProperty("url"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedAudio() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        GraphObjectList<GraphObject> audios = parsedObject.getAudio();
        assertEquals("http://www.example.com/audio/98", audios.get(0).getProperty("url"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedSeeAlso() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals("http://www.example.com/101", parsedObject.getSeeAlso().get(0));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedSiteName() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals("Awesome Site", parsedObject.getSiteName());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedTimes() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals(new Date(1367345897000L), parsedObject.getCreatedTime());
        assertEquals(new Date(1367345897000L), parsedObject.getUpdatedTime());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedApplication() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        GraphObject application = parsedObject.getApplication();
        assertEquals("Awesome App", application.getProperty("name"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedIsScraped() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals(false, parsedObject.getIsScraped());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedPostActionId() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        assertEquals("1234", parsedObject.getPostActionId());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedData() throws JSONException {
        OpenGraphObject parsedObject = getParsedObject();
        GraphObject data = parsedObject.getData();

        assertEquals("blue", data.getProperty("color"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedDataWithTypedObject() throws JSONException {
        TestOpenGraphObject parsedObject = getParsedObject().cast(TestOpenGraphObject.class);
        TestOpenGraphObjectData data = parsedObject.getData();

        assertEquals("blue", data.getColor());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetImagesByUrl() throws JSONException {
        OpenGraphObject object = GraphObject.Factory.create(OpenGraphObject.class);

        object.setImageUrls(Arrays.asList("http://www.example.com/1", "http://www.example.com/2"));

        GraphObjectList<GraphObject> images = object.getImage();
        assertNotNull(images);
        assertEquals(2, images.size());
        assertEquals("http://www.example.com/1", images.get(0).getProperty("url"));
        assertEquals("http://www.example.com/2", images.get(1).getProperty("url"));
    }
}
