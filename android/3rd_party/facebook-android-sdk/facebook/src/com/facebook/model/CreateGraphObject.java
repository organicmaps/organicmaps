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

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Use this annotation setters in an interface that derives from
 * GraphObject, if you wish to provide a setter that takes a primitive data type (e.g., String)
 * or a List of primitive data types, but actually populates its underlying property with a
 * new GraphObject with a property equal to the specified value (or a List of such GraphObjects).
 * This is useful for providing "helper" setters to avoid requiring callers to instantiate a GraphObject
 * just to set a single property on it (e.g., 'url' or 'id').
 *
 * The String value provided to this annotation should be the name of the property that will be
 * populated on the newly-created GraphObject using the value that was passed to the setter.
 *
 * This annotation has no effect if applied to a getter.
 */
@Retention(RetentionPolicy.RUNTIME)
public @interface CreateGraphObject {
    String value();
}
