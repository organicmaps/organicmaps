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
 * Use this annotation on getters and setters in an interface that derives from
 * GraphObject, if you wish to override the default property name that is inferred
 * from the name of the method.
 *
 * If this annotation is specified on a method, it must contain a non-empty String
 * value that represents the name of the property that the method is a getter or setter
 * for.
 */
@Retention(RetentionPolicy.RUNTIME)
public @interface PropertyName {
    String value();
}
