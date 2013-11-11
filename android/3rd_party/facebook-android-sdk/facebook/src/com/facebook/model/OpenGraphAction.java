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

import org.json.JSONObject;

import java.util.Date;
import java.util.List;

/**
 * Provides a strongly-typed representation of an Open Graph Action.
 * For more documentation of OG Actions, see: https://developers.facebook.com/docs/opengraph/actions/
 *
 * Note that this interface is intended to be used with GraphObject.Factory or OpenGraphAction.Factory
 * and not implemented directly.
 */
public interface OpenGraphAction extends GraphObject {
    /**
     * Gets the ID of the action.
     * @return the ID
     */
    String getId();

    /**
     * Sets the ID of the action.
     * @param id the ID
     */
    void setId(String id);

    /**
     * Gets the type of the action, which is a string in the form "mynamespace:mytype".
     * @return the type
     */
    String getType();

    /**
     * Sets the type of the action, which is a string in the form "mynamespace:mytype".
     * @param type the type
     */
    void setType(String type);

    /**
     * Gets the start time of the action.
     * @return the start time
     */
    Date getStartTime();

    /**
     * Sets the start time of the action.
     * @param startTime the start time
     */
    void setStartTime(Date startTime);

    /**
     * Gets the end time of the action.
     * @return the end time
     */
    Date getEndTime();

    /**
     * Sets the end time of the action.
     * @param endTime the end time
     */
    void setEndTime(Date endTime);

    /**
     * Gets the time the action was published, if any.
     * @return the publish time
     */
    Date getPublishTime();

    /**
     * Sets the time the action was published.
     * @param publishTime the publish time
     */
    void setPublishTime(Date publishTime);

    /**
     * Gets the time the action was created.
     * @return the creation time
     */
    public Date getCreatedTime();

    /**
     * Sets the time the action was created.
     * @param createdTime the creation time
     */
    public void setCreatedTime(Date createdTime);

    /**
     * Gets the time the action expires at.
     * @return the expiration time
     */
    public Date getExpiresTime();

    /**
     * Sets the time the action expires at.
     * @param expiresTime the expiration time
     */
    public void setExpiresTime(Date expiresTime);

    /**
     * Gets the unique string which will be passed to the OG Action owner's website
     * when a user clicks through this action on Facebook.
     * @return the ref string
     */
    String getRef();

    /**
     * Sets the unique string which will be passed to the OG Action owner's website
     * when a user clicks through this action on Facebook.
     * @param ref the ref string
     */
    void setRef(String ref);

    /**
     * Gets the message assoicated with the action.
     * @return the message
     */
    String getMessage();

    /**
     * Sets the message associated with the action.
     * @param message the message
     */
    void setMessage(String message);

    /**
     * Gets the place where the action took place.
     * @return the place
     */
    GraphPlace getPlace();

    /**
     * Sets the place where the action took place.
     * @param place the place
     */
    void setPlace(GraphPlace place);

    /**
     * Gets the list of profiles that were tagged in the action.
     * @return the profiles that were tagged in the action
     */
    GraphObjectList<GraphObject> getTags();

    /**
     * Sets the list of profiles that were tagged in the action.
     * @param tags the profiles that were tagged in the action
     */
    void setTags(List<? extends GraphObject> tags);

    /**
     * Gets the images that were associated with the action.
     * @return the images
     */
    List<JSONObject> getImage();

    /**
     * Sets the images that were associated with the action.
     * @param image the images
     */
    void setImage(List<JSONObject> image);

    /**
     * Sets the images associated with the Open Graph action by specifying their URLs. This is a helper
     * that will create GraphObjects with the correct URLs and populate the property with those objects.
     * @param urls the URLs
     */
    @CreateGraphObject("url")
    @PropertyName("image")
    void setImageUrls(List<String> urls);

    /**
     * Gets the from-user associated with the action.
     * @return the user
     */
    GraphUser getFrom();

    /**
     * Sets the from-user associated with the action.
     * @param from the from-user
     */
    void setFrom(GraphUser from);

    /**
     * Gets the 'likes' that have been performed on this action.
     * @return the likes
     */
    public JSONObject getLikes();

    /**
     * Sets the 'likes' that have been performed on this action.
     * @param likes the likes
     */
    public void setLikes(JSONObject likes);

    /**
     * Gets the application that created this action.
     * @return the application
     */
    GraphObject getApplication();

    /**
     * Sets the application that created this action.
     * @param application the application
     */
    void setApplication(GraphObject application);

    /**
     * Gets the comments that have been made on this action.
     * @return the comments
     */
    public JSONObject getComments();

    /**
     * Sets the comments that have been made on this action.
     * @param comments the comments
     */
    void setComments(JSONObject comments);

    /**
     * Gets the type-specific data for this action; for instance, any properties
     * referencing Open Graph objects will appear under here.
     * @return a GraphObject representing the type-specific data
     */
    GraphObject getData();

    /**
     * Sets the type-specific data for this action.
     * @param data a GraphObject representing the type-specific data
     */
    void setData(GraphObject data);


    /**
     * Gets whether the action has been explicitly shared by the user. See
     * <a href="https://developers.facebook.com/docs/opengraph/guides/explicit-sharing/">Explicit Sharing</a> for
     * more information.
     * @return true if this action was explicitly shared
     */
    @PropertyName("fb:explicitly_shared")
    boolean getExplicitlyShared();

    /**
     * Sets whether the action has been explicitly shared by the user. See
     * <a href="https://developers.facebook.com/docs/opengraph/guides/explicit-sharing/">Explicit Sharing</a> for
     * more information. You should only specify this property if explicit sharing has been enabled for an
     * Open Graph action type.
     * @param explicitlyShared true if this action was explicitly shared
     */
    @PropertyName("fb:explicitly_shared")
    void setExplicitlyShared(boolean explicitlyShared);

    /**
     * Exposes helpers for creating instances of OpenGraphAction.
     */
    final class Factory {
        /**
         * Creates an OpenGraphAction suitable for posting via, e.g., a native Share dialog.
         * @return an OpenGraphAction
         */
        @Deprecated
        public static OpenGraphAction createForPost() {
            return createForPost(OpenGraphAction.class, null);
        }

        /**
         * Creates an OpenGraphAction suitable for posting via, e.g., a native Share dialog.
         * @param type the Open Graph action type for the action, or null if it will be specified later
         * @return an OpenGraphAction
         */
        public static OpenGraphAction createForPost(String type) {
            return createForPost(OpenGraphAction.class, type);
        }

        /**
         * Creates an OpenGraphAction suitable for posting via, e.g., a native Share dialog.
         * @param type the Open Graph action type for the action, or null if it will be specified later
         * @param graphObjectClass the OpenGraphAction-derived type to return
         * @return an OpenGraphAction
         */
        public static <T extends OpenGraphAction> T createForPost(Class<T> graphObjectClass, String type) {
            T object = GraphObject.Factory.create(graphObjectClass);

            if (type != null) {
                object.setType(type);
            }

            return object;
        }
    }
}
