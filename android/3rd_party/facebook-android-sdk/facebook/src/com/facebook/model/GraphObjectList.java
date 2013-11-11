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

import org.json.JSONArray;

import java.util.List;

/**
 * GraphObjectList is the primary representation of a collection of graph objects in the Facebook SDK for Android.
 * It is not implemented by any concrete classes, but rather by a proxy (see the {@link com.facebook.model.GraphObject.Factory Factory}
 * class). A GraphObjectList can actually contain elements of any type, not just graph objects, but its principal
 * use in the SDK is to contain types derived from GraphObject.
 * <br/>
 *
 * @param <T> the type of elements in the list
 */
public interface GraphObjectList<T> extends List<T> {
    // cast method is only supported if T extends GraphObject
    /**
     * If T is derived from GraphObject, returns a new GraphObjectList exposing the same underlying data as a new
     * GraphObject-derived type.
     * @param graphObjectClass the GraphObject-derived type to return a list of
     * @return a list representing the same underlying data, exposed as the new GraphObject-derived type
     * @throws com.facebook.FacebookGraphObjectException if T does not derive from GraphObject
     */
    public <U extends GraphObject> GraphObjectList<U> castToListOf(Class<U> graphObjectClass);
    /**
     * Gets the underlying JSONArray representation of the data.
     * @return the underlying JSONArray representation of the data
     */
    public JSONArray getInnerJSONArray();
}
