package com.facebook.model;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class OpenGraphActionTests extends AndroidTestCase {
    private static String ACTION_JSON = "{" +
            "  \"id\": \"99\", " +
            "  \"from\": {" +
            "    \"name\": \"A User\", " +
            "    \"id\": \"42\"" +
            "  }, " +
            "  \"start_time\": \"2013-04-11T02:05:17+0000\", " +
            "  \"end_time\": \"2013-04-11T03:05:17+0000\", " +
            "  \"publish_time\": \"2013-04-11T02:05:17+0000\", " +
            "  \"ref\": \"hello!\", " +
            "  \"message\": \"a message!\", " +
            "  \"place\": {" +
            "    \"id\": \"9999\", " +
            "    \"name\": \"Some Place\", " +
            "    \"location\": {" +
            "      \"latitude\": 37.786130951058, " +
            "      \"longitude\": -122.40886171765, " +
            "      \"city\": \"San Francisco\", " +
            "      \"country\": \"United States\", " +
            "      \"id\": \"2421836\", " +
            "      \"zip\": \"94102-2118\", " +
            "      \"address\": \"5 Any Street\", " +
            "      \"region\": \"CA\"" +
            "    } " +
            "  }, " +
            "  \"tags\": [" +
            "    {" +
            "      \"id\": \"4321\", " +
            "      \"name\": \"Jim Bob\"" +
            "    }" +
            "  ], " +
            "  \"application\": {" +
            "    \"name\": \"Awesome App\", " +
            "    \"namespace\": \"awesome\", " +
            "    \"id\": \"55\"" +
            "  }, " +
            "  \"data\": {" +
            "    \"thing\": {" +
            "      \"id\": \"509\", " +
            "      \"url\": \"http://www.example.com/100\", " +
            "      \"type\": \"awesome:thing\", " +
            "      \"title\": \"A thing!\"" +
            "    }" +
            "  }, " +
            "  \"type\": \"awesome:action\", " +
            "  \"likes\": {" +
            "    \"count\": 7, " +
            "    \"can_like\": true, " +
            "    \"user_likes\": false" +
            "  }, " +
            "  \"comments\": {" +
            "    \"data\": [" +
            "      {" +
            "        \"id\": \"2_3\", " +
            "        \"from\": {" +
            "          \"name\": \"A Yooser\", " +
            "          \"id\": \"1001\"" +
            "        }, " +
            "        \"message\": \"Here's a comment.\", " +
            "        \"can_remove\": true, " +
            "        \"created_time\": \"2013-04-26T23:38:19+0000\", " +
            "        \"like_count\": 3, " +
            "        \"user_likes\": false" +
            "      }" +
            "    ], " +
            "    \"paging\": {" +
            "      \"cursors\": {" +
            "        \"after\": \"x\", " +
            "        \"before\": \"x\"" +
            "      }" +
            "    }, " +
            "    \"count\": 1, " +
            "    \"can_comment\": true, " +
            "    \"comment_order\": \"chronological\"" +
            "  }," +
            "  \"likes\": {" +
            "    \"data\": [" +
            "      {" +
            "        \"id\": \"422\", " +
            "        \"name\": \"Another User\"" +
            "      }" +
            "    ], " +
            "    \"paging\": {" +
            "      \"next\": \"https://graph.facebook.com/blah\"" +
            "    }, " +
            "    \"count\": 1, " +
            "    \"can_like\": true, " +
            "    \"user_likes\": true" +
            "  }" +
            "}";

    interface TestOpenGraphActionData extends GraphObject {
        GraphObject getThing();
    }

    interface TestOpenGraphAction extends OpenGraphAction {
        TestOpenGraphActionData getData();
    }

    private OpenGraphAction parsedAction;

    public void setUp() throws JSONException {
        JSONObject jsonObject = new JSONObject(ACTION_JSON);
        parsedAction = GraphObject.Factory.create(jsonObject, OpenGraphAction.class);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedId() {
        assertEquals("99", parsedAction.getId());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedType() {
        assertEquals("awesome:action", parsedAction.getType());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedDates() {
        assertEquals(new Date(1365645917000L), parsedAction.getStartTime());
        assertEquals(new Date(1365649517000L), parsedAction.getEndTime());
        assertEquals(new Date(1365645917000L), parsedAction.getPublishTime());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedRef() {
        assertEquals("hello!", parsedAction.getRef());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedMessage() {
        assertEquals("a message!", parsedAction.getMessage());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedPlace() {
        GraphPlace place = parsedAction.getPlace();
        assertEquals("9999", place.getId());
        assertEquals("94102-2118", place.getLocation().getZip());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedTags() {
        List<GraphObject> tags = parsedAction.getTags();
        assertEquals(1, tags.size());
        GraphUser tag = tags.get(0).cast(GraphUser.class);
        assertEquals("4321", tag.getId());
        assertEquals("Jim Bob", tag.getName());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedFrom() {
        GraphUser from = parsedAction.getFrom();
        assertEquals("42", from.getId());
        assertEquals("A User", from.getName());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedLikes() {
        JSONObject likes = parsedAction.getLikes();
        assertEquals(1, likes.optInt("count"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedApplication() {
        GraphObject application = parsedAction.getApplication();
        assertEquals("Awesome App", application.getProperty("name"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedComments() {
        JSONObject comments = parsedAction.getComments();
        assertEquals(1, comments.optInt("count"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedData() {
        GraphObject data = parsedAction.getData();
        assertNotNull(data);

        GraphObject thing = data.getPropertyAs("thing", GraphObject.class);
        assertNotNull(thing);
        assertEquals("509", thing.getProperty("id"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedTypedData() {
        TestOpenGraphActionData data = parsedAction.getPropertyAs("data", TestOpenGraphActionData.class);
        assertNotNull(data);

        GraphObject thing = data.getThing();
        assertNotNull(thing);
        assertEquals("509", thing.getProperty("id"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testParsedDataWithTypedAction() {
        TestOpenGraphAction typedAction = parsedAction.cast(TestOpenGraphAction.class);
        TestOpenGraphActionData data = typedAction.getData();
        assertNotNull(data);

        GraphObject thing = data.getThing();
        assertNotNull(thing);
        assertEquals("509", thing.getProperty("id"));
    }

    public void testSetGetTags() {
        OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");

        GraphObject tag = GraphObject.Factory.create();
        tag.setProperty("id", "123");

        List<GraphObject> tags = new ArrayList<GraphObject>();
        tags.add(tag);

        action.setTags(tags);

        GraphObjectList<GraphObject> retrievedTags = action.getTags();
        assertNotNull(retrievedTags);
        assertEquals(1, retrievedTags.size());
        assertEquals("123", retrievedTags.get(0).getProperty("id"));
    }
}
