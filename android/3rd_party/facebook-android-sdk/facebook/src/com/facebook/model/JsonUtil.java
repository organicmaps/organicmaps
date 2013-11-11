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

import android.annotation.SuppressLint;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.*;

class JsonUtil {
    static void jsonObjectClear(JSONObject jsonObject) {
        @SuppressWarnings("unchecked")
        Iterator<String> keys = (Iterator<String>) jsonObject.keys();
        while (keys.hasNext()) {
            keys.next();
            keys.remove();
        }
    }

    static boolean jsonObjectContainsValue(JSONObject jsonObject, Object value) {
        @SuppressWarnings("unchecked")
        Iterator<String> keys = (Iterator<String>) jsonObject.keys();
        while (keys.hasNext()) {
            Object thisValue = jsonObject.opt(keys.next());
            if (thisValue != null && thisValue.equals(value)) {
                return true;
            }
        }
        return false;
    }

    private final static class JSONObjectEntry implements Map.Entry<String, Object> {
        private final String key;
        private final Object value;

        JSONObjectEntry(String key, Object value) {
            this.key = key;
            this.value = value;
        }

        @SuppressLint("FieldGetter")
        @Override
        public String getKey() {
            return this.key;
        }

        @Override
        public Object getValue() {
            return this.value;
        }

        @Override
        public Object setValue(Object object) {
            throw new UnsupportedOperationException("JSONObjectEntry is immutable");
        }

    }

    static Set<Map.Entry<String, Object>> jsonObjectEntrySet(JSONObject jsonObject) {
        HashSet<Map.Entry<String, Object>> result = new HashSet<Map.Entry<String, Object>>();

        @SuppressWarnings("unchecked")
        Iterator<String> keys = (Iterator<String>) jsonObject.keys();
        while (keys.hasNext()) {
            String key = keys.next();
            Object value = jsonObject.opt(key);
            result.add(new JSONObjectEntry(key, value));
        }

        return result;
    }

    static Set<String> jsonObjectKeySet(JSONObject jsonObject) {
        HashSet<String> result = new HashSet<String>();

        @SuppressWarnings("unchecked")
        Iterator<String> keys = (Iterator<String>) jsonObject.keys();
        while (keys.hasNext()) {
            result.add(keys.next());
        }

        return result;
    }

    static void jsonObjectPutAll(JSONObject jsonObject, Map<String, Object> map) {
        Set<Map.Entry<String, Object>> entrySet = map.entrySet();
        for (Map.Entry<String, Object> entry : entrySet) {
            try {
                jsonObject.putOpt(entry.getKey(), entry.getValue());
            } catch (JSONException e) {
                throw new IllegalArgumentException(e);
            }
        }
    }

    static Collection<Object> jsonObjectValues(JSONObject jsonObject) {
        ArrayList<Object> result = new ArrayList<Object>();

        @SuppressWarnings("unchecked")
        Iterator<String> keys = (Iterator<String>) jsonObject.keys();
        while (keys.hasNext()) {
            result.add(jsonObject.opt(keys.next()));
        }

        return result;
    }
}
