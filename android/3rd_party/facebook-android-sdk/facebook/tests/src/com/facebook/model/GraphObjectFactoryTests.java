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

package com.facebook.model;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.FacebookGraphObjectException;
import junit.framework.Assert;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.*;
import java.util.Map.Entry;

public final class GraphObjectFactoryTests extends AndroidTestCase {

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreateEmptyGraphObject() {
        GraphObject graphObject = GraphObject.Factory.create();
        assertTrue(graphObject != null);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanTreatAsMap() {
        GraphObject graphObject = GraphObject.Factory.create();

        graphObject.setProperty("hello", "world");
        assertEquals("world", (String) graphObject.asMap().get("hello"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanTreatAsGraphPlace() {
        GraphPlace graphPlace = GraphObject.Factory.create(GraphPlace.class);

        graphPlace.setName("hello");
        assertEquals("hello", graphPlace.getName());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanTreatAsGraphUser() {
        GraphUser graphUser = GraphObject.Factory.create(GraphUser.class);

        graphUser.setFirstName("Michael");
        assertEquals("Michael", graphUser.getFirstName());
        assertEquals("Michael", graphUser.getProperty("first_name"));
        assertEquals("Michael", graphUser.asMap().get("first_name"));

        graphUser.setProperty("last_name", "Scott");
        assertEquals("Scott", graphUser.getProperty("last_name"));
        assertEquals("Scott", graphUser.getLastName());
        assertEquals("Scott", graphUser.asMap().get("last_name"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanCastBetweenGraphObjectTypes() {
        GraphObject graphObject = GraphObject.Factory.create();

        graphObject.setProperty("first_name", "Mickey");

        GraphUser graphUser = graphObject.cast(GraphUser.class);
        assertTrue(graphUser != null);
        // Should see the name we set earlier as a GraphObject.
        assertEquals("Mickey", graphUser.getFirstName());

        // Changes to GraphUser should be reflected in GraphObject version.
        graphUser.setLastName("Mouse");
        assertEquals("Mouse", graphObject.getProperty("last_name"));
    }

    interface Base extends GraphObject {
    }

    interface Derived extends Base {
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCastingToSameTypeGivesSameObject() {
        Base base = GraphObject.Factory.create(Base.class);

        Base cast = base.cast(Base.class);

        assertTrue(base == cast);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCastingToBaseTypeGivesSameObject() {
        Derived derived = GraphObject.Factory.create(Derived.class);

        Base cast = derived.cast(Base.class);
        assertTrue(derived == cast);

        cast = cast.cast(Derived.class);
        assertTrue(derived == cast);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanSetComplexTypes() {
        GraphLocation graphLocation = GraphObject.Factory.create(GraphLocation.class);
        graphLocation.setCity("Seattle");

        GraphPlace graphPlace = GraphObject.Factory.create(GraphPlace.class);
        graphPlace.setLocation(graphLocation);

        assertEquals(graphLocation, graphPlace.getLocation());
        assertEquals("Seattle", graphPlace.getLocation().getCity());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanConvertFromJSON() throws JSONException {
        JSONObject jsonLocation = new JSONObject();
        jsonLocation.put("city", "Paris");
        jsonLocation.put("country", "France");

        JSONObject jsonPlace = new JSONObject();
        jsonPlace.put("location", jsonLocation);
        jsonPlace.put("name", "Eiffel Tower");

        GraphPlace graphPlace = GraphObject.Factory.create(jsonPlace, GraphPlace.class);
        GraphLocation graphLocation = graphPlace.getLocation();
        assertEquals("Paris", graphLocation.getCity());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanConvertFromGraphObject() throws JSONException {
        GraphObject graphObject = GraphObject.Factory.create();
        graphObject.setProperty("city", "Paris");
        graphObject.setProperty("country", "France");

        JSONObject jsonPlace = new JSONObject();
        jsonPlace.put("location", graphObject);
        jsonPlace.put("name", "Eiffel Tower");

        GraphPlace graphPlace = GraphObject.Factory.create(jsonPlace, GraphPlace.class);
        GraphLocation graphLocation = graphPlace.getLocation();
        assertEquals("Paris", graphLocation.getCity());
    }

    private abstract class GraphObjectClass implements GraphObject {
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanConvertNumbers() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("double_as_string", 3.14159);
        jsonObject.put("int_as_string", 42);

        GraphMetric metric = GraphObject.Factory.create(jsonObject, GraphMetric.class);
        assertEquals("42", metric.getIntAsString());
        assertNotNull(metric.getDoubleAsString());
        assertTrue(metric.getDoubleAsString().startsWith("3.14159"));
    }

    private interface GraphMetric extends GraphObject {
        String getIntAsString();
        String getDoubleAsString();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapNonInterface() {
        try {
            GraphObject.Factory.create(GraphObjectClass.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadNoParameterMethodNameGraphObject extends GraphObject {
        Object floppityFlee();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadZeroParameterMethodName() {
        try {
            GraphObject.Factory.create(BadNoParameterMethodNameGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadSingleParameterMethodNameGraphObject extends GraphObject {
        void floppityFlee(Object obj);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadSingleParameterMethodName() {
        try {
            GraphObject.Factory.create(BadSingleParameterMethodNameGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadGetterNameGraphObject extends GraphObject {
        void get();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadGetterName() {
        try {
            GraphObject.Factory.create(BadGetterNameGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadGetterParamsGraphObject extends GraphObject {
        Object getFoo(Object obj);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadGetterParams() {
        try {
            GraphObject.Factory.create(BadGetterParamsGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadGetterReturnTypeGraphObject extends GraphObject {
        void getFoo();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadGetterReturnType() {
        try {
            GraphObject.Factory.create(BadGetterReturnTypeGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadSetterNameGraphObject extends GraphObject {
        void set();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadSetterName() {
        try {
            GraphObject.Factory.create(BadSetterNameGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadSetterParamsGraphObject extends GraphObject {
        void setFoo();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadSetterParams() {
        try {
            GraphObject.Factory.create(BadSetterParamsGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadSetterReturnTypeGraphObject extends GraphObject {
        Object setFoo(Object obj);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadSetterReturnType() {
        try {
            GraphObject.Factory.create(BadSetterReturnTypeGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface BadBaseInterfaceGraphObject extends BadSetterReturnTypeGraphObject {
        void setBar(Object obj);

        Object getBar();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadBaseInterface() {
        try {
            GraphObject.Factory.create(BadBaseInterfaceGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    private interface GoodPropertyOverrideInterfaceGraphObject extends GraphObject {
        void setDefaultName(String s);

        // No annotation to ensure that the right property is being set.
        String getAnotherDefaultName();

        @PropertyName("another_default_name")
        void putSomething(String s);

        @PropertyName("default_name")
        String retrieveSomething();

        @PropertyName("MixedCase")
        void setMixedCase(String s);

        @PropertyName("MixedCase")
        String getMixedCase();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanOverrideGraphPropertyNames() {
        GoodPropertyOverrideInterfaceGraphObject graphObject =
                GraphObject.Factory.create(GoodPropertyOverrideInterfaceGraphObject.class);

        String testValue = "flu-blah";
        graphObject.setDefaultName(testValue);
        Assert.assertEquals(testValue, graphObject.retrieveSomething());

        testValue = testValue + "1";
        graphObject.putSomething(testValue);
        Assert.assertEquals(testValue, graphObject.getAnotherDefaultName());

        testValue = testValue + "2";
        graphObject.setMixedCase(testValue);
        Assert.assertEquals(testValue, graphObject.getMixedCase());
    }

    private interface BadPropertyOverrideInterfaceGraphObject extends GraphObject {
        @PropertyName("")
        void setMissingProperty(Object value);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantWrapBadPropertyNameOverrides() {
        try {
            GraphObject.Factory.create(BadPropertyOverrideInterfaceGraphObject.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testObjectEquals() {
        GraphObject graphObject = GraphObject.Factory.create();
        graphObject.setProperty("aKey", "aValue");

        assertTrue(graphObject.equals(graphObject));

        GraphPlace graphPlace = graphObject.cast(GraphPlace.class);
        assertTrue(graphObject.equals(graphPlace));
        assertTrue(graphPlace.equals(graphObject));

        GraphObject aDifferentGraphObject = GraphObject.Factory.create();
        aDifferentGraphObject.setProperty("aKey", "aDifferentValue");
        assertFalse(graphObject.equals(aDifferentGraphObject));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testGetProperty() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);
        assertEquals("world", graphObject.getProperty("hello"));
        assertTrue(graphObject.getProperty("fred") == null);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetProperty() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        graphObject.setProperty("hello", "world");
        graphObject.setProperty("don't imagine", "purple elephants");

        assertEquals("world", jsonObject.getString("hello"));
        assertEquals("purple elephants", jsonObject.getString("don't imagine"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetPropertyWithGraphObject() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        GraphObject nestedObject = GraphObject.Factory.create();
        graphObject.setProperty("foo", nestedObject);

        JSONObject nestedJsonObject = jsonObject.getJSONObject("foo");
        assertNotNull(nestedJsonObject);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetPropertyWithGraphObjectList() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        GraphObjectList<GraphObject> nestedList = GraphObject.Factory.createList(GraphObject.class);
        graphObject.setProperty("foo", nestedList);

        JSONArray nestedJsonArray = jsonObject.getJSONArray("foo");
        assertNotNull(nestedJsonArray);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetPropertyWithList() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        List<GraphObject> nestedList = new ArrayList<GraphObject>();
        graphObject.setProperty("foo", nestedList);

        JSONArray nestedJsonArray = jsonObject.getJSONArray("foo");
        assertNotNull(nestedJsonArray);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testRemoveProperty() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("whirled", "peas");
        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        graphObject.setProperty("hello", "world");
        graphObject.setProperty("don't imagine", "purple elephants");

        assertEquals("world", jsonObject.getString("hello"));
        assertEquals("purple elephants", jsonObject.getString("don't imagine"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapClear() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        assertEquals(1, jsonObject.length());

        graphObject.asMap().clear();

        assertEquals(0, jsonObject.length());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapContainsKey() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        assertTrue(graphObject.asMap().containsKey("hello"));
        assertFalse(graphObject.asMap().containsKey("hocus"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapContainsValue() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        assertTrue(graphObject.asMap().containsValue("world"));
        assertFalse(graphObject.asMap().containsValue("pocus"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapEntrySet() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        Set<Entry<String, Object>> entrySet = graphObject.asMap().entrySet();
        assertEquals(2, entrySet.size());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapGet() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);
        assertEquals("world", graphObject.asMap().get("hello"));
        assertTrue(graphObject.getProperty("fred") == null);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapGetReturnsNullForMissingProperty() throws JSONException {
        GraphUser graphUser = GraphObject.Factory.create(GraphUser.class);
        assertNull(graphUser.getBirthday());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapIsEmpty() throws JSONException {
        JSONObject jsonObject = new JSONObject();

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);
        assertTrue(graphObject.asMap().isEmpty());

        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");
        assertFalse(graphObject.asMap().isEmpty());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapKeySet() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        Set<String> keySet = graphObject.asMap().keySet();
        assertEquals(2, keySet.size());
        assertTrue(keySet.contains("hello"));
        assertTrue(keySet.contains("hocus"));
        assertFalse(keySet.contains("world"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapPut() throws JSONException {
        JSONObject jsonObject = new JSONObject();

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);
        graphObject.setProperty("hello", "world");
        graphObject.setProperty("hocus", "pocus");

        assertEquals("pocus", jsonObject.get("hocus"));
        assertEquals(2, jsonObject.length());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapPutOfWrapperPutsJSONObject() throws JSONException {
        JSONObject jsonObject = new JSONObject();

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);
        graphObject.setProperty("hello", "world");
        graphObject.setProperty("hocus", "pocus");

        GraphObject parentObject = GraphObject.Factory.create();
        parentObject.setProperty("key", graphObject);

        JSONObject jsonParent = parentObject.getInnerJSONObject();
        Object obj = jsonParent.opt("key");

        assertNotNull(obj);
        assertEquals(jsonObject, obj);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapPutOfWrapperPutsJSONArray() throws JSONException {
        JSONArray jsonArray = new JSONArray();

        GraphObjectList<String> graphObjectList = GraphObject.Factory
                .createList(jsonArray, String.class);
        graphObjectList.add("hello");
        graphObjectList.add("world");

        GraphObject parentObject = GraphObject.Factory.create();
        parentObject.setProperty("key", graphObjectList);

        JSONObject jsonParent = parentObject.getInnerJSONObject();
        Object obj = jsonParent.opt("key");

        assertNotNull(obj);
        assertEquals(jsonArray, obj);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapPutAll() throws JSONException {
        HashMap<String, Object> map = new HashMap<String, Object>();
        map.put("hello", "world");
        map.put("hocus", "pocus");

        JSONObject jsonObject = new JSONObject();
        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        graphObject.asMap().putAll(map);
        assertEquals("pocus", jsonObject.get("hocus"));
        assertEquals(2, jsonObject.length());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapRemove() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);
        graphObject.removeProperty("hello");

        assertEquals(1, jsonObject.length());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapSize() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        assertEquals(2, graphObject.asMap().size());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testMapValues() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        Collection<Object> values = graphObject.asMap().values();

        assertEquals(2, values.size());
        assertTrue(values.contains("world"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testGetInnerJSONObject() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("hello", "world");
        jsonObject.put("hocus", "pocus");

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);

        assertEquals(jsonObject, graphObject.getInnerJSONObject());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSettingGraphObjectProxyStoresJSONObject() throws JSONException {
        GraphPlace graphPlace = GraphObject.Factory.create(GraphPlace.class);
        GraphLocation graphLocation = GraphObject.Factory.create(GraphLocation.class);

        graphPlace.setLocation(graphLocation);

        assertEquals(graphLocation.getInnerJSONObject(), graphPlace.getInnerJSONObject().get("location"));

    }

    private interface DateGraphObject extends GraphObject {
        Date getDate1();

        Date getDate2();

        Date getDate3();

        Date getDate4();
        void setDate4(Date date);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testGetStringsAsDates() {
        DateGraphObject dates = GraphObject.Factory.create(DateGraphObject.class);
        dates.setProperty("date1", "2012-07-04");
        dates.setProperty("date2", "2012-07-04T19:30:50");
        dates.setProperty("date3", "2012-07-04T19:20:40-0400");

        // Dates without a time zone should be assumed to be in the current timezone.
        Calendar cal = new GregorianCalendar();
        cal.set(Calendar.MILLISECOND, 0);

        cal.set(2012, 6, 4, 0, 0, 0);
        Date expectedDate1 = cal.getTime();
        Date date1 = dates.getDate1();
        assertEquals(expectedDate1, date1);

        cal.set(2012, 6, 4, 19, 30, 50);
        Date expectedDate2 = cal.getTime();
        Date date2 = dates.getDate2();
        assertEquals(expectedDate2, date2);

        // Dates with an explicit time zone should take that timezone into account.
        cal = new GregorianCalendar(TimeZone.getTimeZone("GMT"));
        cal.set(Calendar.MILLISECOND, 0);
        cal.set(2012, 6, 4, 23, 20, 40);

        Date expectedDate3 = cal.getTime();
        Date date3 = dates.getDate3();
        assertEquals(expectedDate3, date3);

        cal.set(2012, 9, 28, 9, 53, 0);
        Date expectedDate4 = cal.getTime();
        dates.setDate4(expectedDate4);
        Date date4 = dates.getDate4();
        assertEquals(expectedDate4, date4);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionAdd() throws JSONException {
        JSONArray array = new JSONArray();

        Collection<Integer> collection = GraphObject.Factory.createList(array, Integer.class);
        collection.add(5);

        assertTrue(array.length() == 1);
        assertTrue(array.optInt(0) == 5);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionAddAll() throws JSONException {
        JSONArray array = new JSONArray();

        Collection<Integer> collectionToAdd = Arrays.asList(5, -1);

        Collection<Integer> collection = GraphObject.Factory.createList(array, Integer.class);
        collection.addAll(collectionToAdd);

        assertTrue(array.length() == 2);
        assertTrue(array.optInt(0) == 5);
        assertTrue(array.optInt(1) == -1);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionContains() throws JSONException {
        JSONArray array = new JSONArray();
        array.put(5);

        Collection<Integer> collection = GraphObject.Factory.createList(array, Integer.class);
        assertTrue(collection.contains(5));
        assertFalse(collection.contains(6));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionContainsAll() throws JSONException {
        JSONArray array = new JSONArray();
        array.put(5);
        array.put(-1);

        Collection<Integer> collection = GraphObject.Factory.createList(array, Integer.class);
        assertTrue(collection.containsAll(Arrays.asList(5)));
        assertTrue(collection.containsAll(Arrays.asList(5, -1)));
        assertFalse(collection.containsAll(Arrays.asList(5, -1, 2)));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionIsEmpty() throws JSONException {
        JSONArray array = new JSONArray();

        Collection<Integer> collection = GraphObject.Factory.createList(array, Integer.class);
        assertTrue(collection.isEmpty());

        array.put(5);
        assertFalse(collection.isEmpty());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionIterator() throws JSONException {
        JSONArray array = new JSONArray();
        array.put(5);
        array.put(-1);

        Collection<Integer> collection = GraphObject.Factory.createList(array, Integer.class);
        Iterator<Integer> iter = collection.iterator();
        assertTrue(iter.hasNext());
        assertTrue(iter.next() == 5);
        assertTrue(iter.hasNext());
        assertTrue(iter.next() == -1);
        assertFalse(iter.hasNext());

        for (Integer i : collection) {
            assertNotSame(0, i);
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionIteratorOfGraphObject() throws JSONException {
        Collection<GraphLocation> collection = GraphObject.Factory.createList(GraphLocation.class);

        GraphLocation seattle = GraphObject.Factory.create(GraphLocation.class);
        seattle.setCity("Seattle");
        collection.add(seattle);
        GraphLocation paris = GraphObject.Factory.create(GraphLocation.class);
        paris.setCity("Paris");
        collection.add(paris);

        Iterator<GraphLocation> iter = collection.iterator();
        assertTrue(iter.hasNext());
        assertEquals(seattle, iter.next());
        assertTrue(iter.hasNext());
        assertEquals(paris, iter.next());
        assertFalse(iter.hasNext());

        for (GraphLocation location : collection) {
            assertTrue(location != null);
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionSize() throws JSONException {
        JSONArray array = new JSONArray();

        Collection<Integer> collection = GraphObject.Factory.createList(array, Integer.class);
        assertEquals(0, collection.size());

        array.put(5);
        assertEquals(1, collection.size());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionClearThrows() throws JSONException {
        try {
            Collection<Integer> collection = GraphObject.Factory.createList(Integer.class);
            collection.clear();
            fail("Expected exception");
        } catch (UnsupportedOperationException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionRemoveThrows() throws JSONException {
        try {
            Collection<Integer> collection = GraphObject.Factory.createList(Integer.class);
            collection.remove(5);
            fail("Expected exception");
        } catch (UnsupportedOperationException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionRemoveAllThrows() throws JSONException {
        try {
            Collection<Integer> collection = GraphObject.Factory.createList(Integer.class);
            collection.removeAll(Arrays.asList());
            fail("Expected exception");
        } catch (UnsupportedOperationException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionRetainAllThrows() throws JSONException {
        try {
            Collection<Integer> collection = GraphObject.Factory.createList(Integer.class);
            collection.retainAll(Arrays.asList());
            fail("Expected exception");
        } catch (UnsupportedOperationException exception) {
        }
    }

    private interface Locations extends GraphObject {
        Collection<GraphLocation> getLocations();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testObjectWrapsJSONCollection() throws JSONException {
        JSONObject jsonLocation = new JSONObject();
        jsonLocation.put("city", "Seattle");

        JSONArray jsonArray = new JSONArray();
        jsonArray.put(jsonLocation);

        JSONObject jsonLocations = new JSONObject();
        jsonLocations.put("locations", jsonArray);

        Locations locations = GraphObject.Factory.create(jsonLocations, Locations.class);
        Collection<GraphLocation> locationsGraphObjectCollection = locations.getLocations();
        assertTrue(locationsGraphObjectCollection != null);

        GraphLocation graphLocation = locationsGraphObjectCollection.iterator().next();
        assertTrue(graphLocation != null);
        assertEquals("Seattle", graphLocation.getCity());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testObjectWrapsIterable() throws JSONException {
        GraphUser user = GraphObject.Factory.create(GraphUser.class);
        user.setFirstName("Foo");
        user.setLastName("Bar");

        List<GraphUser> users = new ArrayList<GraphUser>();
        users.add(user);

        OpenGraphAction action = GraphObject.Factory.create(OpenGraphAction.class);
        action.setTags(users);

        String json = action.getInnerJSONObject().toString();

        assertTrue("JSON string should contain last_name", json.contains("last_name"));

        Object tags = action.getInnerJSONObject().get("tags");
        assertNotNull("tags should not be null", tags);
        assertTrue("tags should be JSONArray", tags instanceof JSONArray);

        List<GraphObject> retrievedUsers = action.getTags();
        assertEquals("Size should be 1", 1, retrievedUsers.size());
        GraphUser retrievedUser = retrievedUsers.get(0).cast(GraphUser.class);
        assertEquals("First name should be Foo", "Foo", retrievedUser.getFirstName());
        assertEquals("Last name should be Bar", "Bar", retrievedUser.getLastName());

    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionWrapsJSONObject() throws JSONException {
        JSONObject jsonLocation = new JSONObject();
        jsonLocation.put("city", "Seattle");

        JSONArray jsonArray = new JSONArray();
        jsonArray.put(jsonLocation);
        Collection<GraphLocation> locationsGraphObjectCollection = GraphObject.Factory
                .createList(jsonArray,
                        GraphLocation.class);
        assertTrue(locationsGraphObjectCollection != null);

        GraphLocation graphLocation = locationsGraphObjectCollection.iterator().next();
        assertTrue(graphLocation != null);
        assertEquals("Seattle", graphLocation.getCity());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCannotCastCollectionOfNonGraphObjects() throws JSONException {
        try {
            GraphObjectList<Integer> collection = GraphObject.Factory.createList(Integer.class);
            collection.castToListOf(GraphLocation.class);
            fail("Expected exception");
        } catch (FacebookGraphObjectException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanCastCollectionOfGraphObjects() throws JSONException {
        JSONObject jsonSeattle = new JSONObject();
        jsonSeattle.put("city", "Seattle");

        JSONArray jsonArray = new JSONArray();
        jsonArray.put(jsonSeattle);

        GraphObjectList<GraphObject> collection = GraphObject.Factory
                .createList(jsonArray, GraphObject.class);

        GraphObjectList<GraphLocation> locationCollection = collection.castToListOf(GraphLocation.class);
        assertTrue(locationCollection != null);

        GraphLocation seattle = locationCollection.iterator().next();
        assertTrue(seattle != null);
        assertEquals("Seattle", seattle.getCity());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCastingCollectionToSameTypeGivesSameObject() {
        GraphObjectList<Base> base = GraphObject.Factory.createList(Base.class);

        GraphObjectList<Base> cast = base.castToListOf(Base.class);

        assertTrue(base == cast);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCastingCollectionToBaseTypeGivesSameObject() {
        GraphObjectList<Derived> derived = GraphObject.Factory.createList(Derived.class);

        GraphObjectList<Base> cast = derived.castToListOf(Base.class);

        assertTrue((GraphObjectList<?>)derived == (GraphObjectList<?>)cast);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanGetInnerJSONArray() throws JSONException {
        JSONArray jsonArray = new JSONArray();

        GraphObjectList<GraphObject> collection = GraphObject.Factory
                .createList(jsonArray, GraphObject.class);

        assertEquals(jsonArray, collection.getInnerJSONArray());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanGetRandomAccess() throws JSONException {
        JSONArray jsonArray = new JSONArray();
        jsonArray.put("Seattle");
        jsonArray.put("Menlo Park");

        GraphObjectList<String> collection = GraphObject.Factory
                .createList(jsonArray, String.class);

        assertEquals("Menlo Park", collection.get(1));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanSetRandomAccess() throws JSONException {
        JSONArray jsonArray = new JSONArray();

        GraphObjectList<String> collection = GraphObject.Factory
                .createList(jsonArray, String.class);

        collection.add("Seattle");
        collection.add("Menlo Park");

        collection.set(1, "Ann Arbor");
        assertEquals("Ann Arbor", collection.get(1));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCollectionPutOfWrapperPutsJSONObject() throws JSONException {
        JSONObject jsonObject = new JSONObject();

        GraphObject graphObject = GraphObject.Factory.create(jsonObject);
        graphObject.setProperty("hello", "world");
        graphObject.setProperty("hocus", "pocus");

        GraphObjectList<GraphObject> parentList = GraphObject.Factory
                .createList(GraphObject.class);
        parentList.add(graphObject);

        JSONArray jsonArray = parentList.getInnerJSONArray();

        Object obj = jsonArray.opt(0);

        assertNotNull(obj);
        assertEquals(jsonObject, obj);

        parentList.set(0, graphObject);

        obj = jsonArray.opt(0);

        assertNotNull(obj);
        assertEquals(jsonObject, obj);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCamelCaseToLowercase() {
        assertEquals("hello_world", GraphObject.Factory
                .convertCamelCaseToLowercaseWithUnderscores("HelloWorld"));
        assertEquals("hello_world", GraphObject.Factory
                .convertCamelCaseToLowercaseWithUnderscores("helloWorld"));
    }

    interface NestedObject extends GraphObject {
        String getId();
        void setId(String id);

        String getUrl();
        void setUrl(String url);
    }

    interface ObjectWithNestedObject extends GraphObject {
        // Single-object version
        NestedObject getNestedObject();
        void setNestedObject(NestedObject nestedObject);

        @PropertyName("nested_object")
        @CreateGraphObject("id")
        void setNestedObjectById(String id);
        @PropertyName("nested_object")
        @CreateGraphObject("url")
        void setNestedObjectByUrl(String url);

        // Test overloaded name
        @CreateGraphObject("id")
        void setNestedObject(String id);

        // List version
        GraphObjectList<NestedObject> getNestedObjects();
        void setNestedObjects(List<NestedObject> nestedObjects);

        @PropertyName("nested_objects")
        @CreateGraphObject("id")
        void setNestedObjectsById(List<String> id);
        @PropertyName("nested_objects")
        @CreateGraphObject("url")
        void setNestedObjectsByUrl(List<String> url);

    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testGetPropertyAs() throws JSONException {
        JSONObject nestedObject = new JSONObject();
        nestedObject.put("id", "55");

        GraphObject containingObject = GraphObject.Factory.create();
        containingObject.setProperty("nested", nestedObject);

        NestedObject nestedGraphObject = containingObject.getPropertyAs("nested", NestedObject.class);
        assertNotNull(nestedGraphObject);
        assertEquals("55", nestedGraphObject.getId());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testGetPropertyAsList() throws JSONException {
        JSONObject nestedObject = new JSONObject();
        nestedObject.put("id", "55");

        JSONArray nestedArray = new JSONArray(Arrays.asList(new JSONObject[]{nestedObject}));
        GraphObject containingObject = GraphObject.Factory.create();
        containingObject.setProperty("nested", nestedArray);

        GraphObjectList<NestedObject> nestedGraphObjects = containingObject.getPropertyAsList("nested",
                NestedObject.class);
        assertNotNull(nestedGraphObjects);
        assertEquals("55", nestedGraphObjects.get(0).getId());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetNestedObject() {
        ObjectWithNestedObject object = GraphObject.Factory.create(ObjectWithNestedObject.class);
        object.setNestedObjectById("77");

        NestedObject nestedObject = object.getNestedObject();
        assertNotNull(nestedObject);
        assertEquals("77", nestedObject.getId());

        object.setNestedObjectByUrl("http://www.example.com");

        nestedObject = object.getNestedObject();
        assertNotNull(nestedObject);
        assertEquals("http://www.example.com", nestedObject.getUrl());

        // Overloaded method
        object.setNestedObject("77");

        nestedObject = object.getNestedObject();
        assertNotNull(nestedObject);
        assertEquals("77", nestedObject.getId());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetNestedObjects() {
        ObjectWithNestedObject object = GraphObject.Factory.create(ObjectWithNestedObject.class);
        object.setNestedObjectsById(Arrays.asList("77", "88"));

        GraphObjectList<NestedObject> nestedObjects = object.getNestedObjects();
        assertNotNull(nestedObjects);
        assertEquals("77", nestedObjects.get(0).getId());
        assertEquals("88", nestedObjects.get(1).getId());

        object.setNestedObjectsByUrl(Arrays.asList("http://www.example.com/1", "http://www.example.com/2"));

        nestedObjects = object.getNestedObjects();
        assertNotNull(nestedObjects);
        assertEquals("http://www.example.com/1", nestedObjects.get(0).getUrl());
        assertEquals("http://www.example.com/2", nestedObjects.get(1).getUrl());
    }

    interface GraphObjectWithPrimitives extends GraphObject {
        boolean getBoolean();
        void setBoolean(boolean value);

        int getInt();
        void setInt(int value);

        char getChar();
        void setChar(char value);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetBooleanProperty() {
        GraphObjectWithPrimitives graphObject = GraphObject.Factory.create(GraphObjectWithPrimitives.class);

        graphObject.setBoolean(true);
        assertEquals(true, graphObject.getBoolean());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testBooleanPropertyDefaultsToFalse() {
        GraphObjectWithPrimitives graphObject = GraphObject.Factory.create(GraphObjectWithPrimitives.class);

        assertEquals(false, graphObject.getBoolean());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetNumericProperty() {
        GraphObjectWithPrimitives graphObject = GraphObject.Factory.create(GraphObjectWithPrimitives.class);

        graphObject.setInt(5);
        assertEquals(5, graphObject.getInt());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNumericPropertyDefaultsToZero() {
        GraphObjectWithPrimitives graphObject = GraphObject.Factory.create(GraphObjectWithPrimitives.class);

        assertEquals(0, graphObject.getInt());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetCharProperty() {
        GraphObjectWithPrimitives graphObject = GraphObject.Factory.create(GraphObjectWithPrimitives.class);

        graphObject.setChar('z');
        assertEquals('z', graphObject.getChar());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCharPropertyDefaultsToZero() {
        GraphObjectWithPrimitives graphObject = GraphObject.Factory.create(GraphObjectWithPrimitives.class);

        assertEquals(0, graphObject.getChar());
    }

}
