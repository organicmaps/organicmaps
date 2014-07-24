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

/**
 * Provides a strongly-typed representation of a User as defined by the Graph API.
 *
 * Note that this interface is intended to be used with GraphObject.Factory
 * and not implemented directly.
 */
public interface GraphUser extends GraphObject {
    /**
     * Returns the ID of the user.
     * @return the ID of the user
     */
    public String getId();
    /**
     * Sets the ID of the user.
     * @param id the ID of the user
     */
    public void setId(String id);

    /**
     * Returns the name of the user.
     * @return the name of the user
     */
    public String getName();
    /**
     * Sets the name of the user.
     * @param name the name of the user
     */
    public void setName(String name);

    /**
     * Returns the first name of the user.
     * @return the first name of the user
     */
    public String getFirstName();
    /**
     * Sets the first name of the user.
     * @param firstName the first name of the user
     */
    public void setFirstName(String firstName);

    /**
     * Returns the middle name of the user.
     * @return the middle name of the user
     */
    public String getMiddleName();
    /**
     * Sets the middle name of the user.
     * @param middleName the middle name of the user
     */
    public void setMiddleName(String middleName);

    /**
     * Returns the last name of the user.
     * @return the last name of the user
     */
    public String getLastName();
    /**
     * Sets the last name of the user.
     * @param lastName the last name of the user
     */
    public void setLastName(String lastName);

    /**
     * Returns the Facebook URL of the user.
     * @return the Facebook URL of the user
     */
    public String getLink();
    /**
     * Sets the Facebook URL of the user.
     * @param link the Facebook URL of the user
     */
    public void setLink(String link);

    /**
     * Returns the Facebook username of the user.
     * @return the Facebook username of the user
     */
    public String getUsername();
    /**
     * Sets the Facebook username of the user.
     * @param username the Facebook username of the user
     */
    public void setUsername(String username);

    /**
     * Returns the birthday of the user.
     * @return the birthday of the user
     */
    public String getBirthday();
    /**
     * Sets the birthday of the user.
     * @param birthday the birthday of the user
     */
    public void setBirthday(String birthday);

    /**
     * Returns the current place of the user.
     * @return the current place of the user
     */
    public GraphPlace getLocation();

    /**
     * Sets the current place of the user.
     * @param location the current place of the user
     */
    public void setLocation(GraphPlace location);
}
