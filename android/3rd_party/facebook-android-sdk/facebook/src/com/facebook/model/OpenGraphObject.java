package com.facebook.model;

import com.facebook.internal.NativeProtocol;

import java.util.Arrays;
import java.util.Date;
import java.util.List;

/**
 * Provides a strongly-typed representation of an Open Graph Object.
 * For more documentation of OG Objects, see: https://developers.facebook.com/docs/opengraph/using-object-api/
 *
 * Note that this interface is intended to be used with GraphObject.Factory or OpenGraphObject.Factory
 * and not implemented directly.
 */
public interface OpenGraphObject extends GraphObject {
    /**
     * Gets the ID of the object.
     * @return the ID
     */
    String getId();

    /**
     * Sets the ID of the object.
     * @param id the ID
     */
    void setId(String id);

    /**
     * Gets the type of the object, which is a string in the form "mynamespace:mytype".
     * @return the type
     */
    String getType();

    /**
     * Sets the type of the object, which is a string in the form "mynamespace:mytype".
     * @param type the type
     */
    void setType(String type);

    /**
     * Gets the URL associated with the Open Graph object.
     * @return the URL
     */
    String getUrl();

    /**
     * Sets the URL associated with the Open Graph object.
     * @param url the URL
     */
    void setUrl(String url);

    /**
     * Gets the title of the Open Graph object.
     * @return the title
     */
    String getTitle();

    /**
     * Sets the title of the Open Graph object.
     * @param title the title
     */
    void setTitle(String title);


    /**
     * Gets the description of the Open Graph object.
     * @return the description
     */
    String getDescription();

    /**
     * Sets the description of the Open Graph Object
     * @param description the description
     */
    void setDescription(String description);

    /**
     * Gets the images associated with the Open Graph object.
     * @return the images
     */
    GraphObjectList<GraphObject> getImage();

    /**
     * Sets the images associated with the Open Graph object.
     * @param images the images
     */
    void setImage(GraphObjectList<GraphObject> images);

    /**
     * Sets the images associated with the Open Graph object by specifying their URLs. This is a helper
     * that will create GraphObjects with the correct URLs and populate the property with those objects.
     * @param urls the URLs
     */
    @CreateGraphObject("url")
    @PropertyName("image")
    void setImageUrls(List<String> urls);

    /**
     * Gets the videos associated with the Open Graph object.
     * @return the videos
     */
    GraphObjectList<GraphObject> getVideo();

    /**
     * Sets the videos associated with the Open Graph object.
     * @param videos the videos
     */
    void setVideo(GraphObjectList<GraphObject> videos);

    /**
     * Gets the audio associated with the Open Graph object.
     * @return the audio
     */
    GraphObjectList<GraphObject> getAudio();

    /**
     * Sets the audio associated with the Open Graph object.
     * @param audios the audio
     */
    void setAudio(GraphObjectList<GraphObject> audios);

    /**
     * Gets the "determiner" for the Open Graph object. This is the word such as "a", "an", or "the" that will
     * appear before the title of the object.
     * @return the determiner string
     */
    String getDeterminer();

    /**
     * Sets the "determiner" for the Open Graph object. This is the word such as "a", "an", or "the" that will
     * appear before the title of the object.
     * @param determiner the determiner string
     */
    void setDeterminer(String determiner);

    /**
     * Gets the list of related resources for the Open Graph object.
     * @return a list of URLs of related resources
     */
    List<String> getSeeAlso();

    /**
     * Sets the list of related resources for the Open Graph object.
     * @param seeAlso a list of URLs of related resources
     */
    void setSeeAlso(List<String> seeAlso);

    /**
     * Gets the name of the site hosting the Open Graph object, if any.
     * @return the name of the site
     */
    String getSiteName();

    /**
     * Sets the name of the site hosting the Open Graph object.
     * @param siteName the name of the site
     */
    void setSiteName(String siteName);

    /**
     * Gets the date and time the Open Graph object was created.
     * @return the creation time
     */
    Date getCreatedTime();

    /**
     * Sets the date and time the Open Graph object was created.
     * @param createdTime the creation time
     */
    void setCreatedTime(Date createdTime);

    /**
     * Gets the date and time the Open Graph object was last updated.
     * @return the update time
     */
    Date getUpdatedTime();

    /**
     * Sets the date and time the Open Graph object was last updated.
     * @param updatedTime the update time
     */
    void setUpdatedTime(Date updatedTime);

    /**
     * Gets the application that created this object.
     * @return the application
     */
    GraphObject getApplication();

    /**
     * Sets the application that created this object.
     * @param application the application
     */
    void setApplication(GraphObject application);

    /**
     * Gets whether the Open Graph object was created by scraping a Web resource or not.
     * @return true if the Open Graph object was created by scraping the Web, false if not
     */
    boolean getIsScraped();

    /**
     * Sets whether the Open Graph object was created by scraping a Web resource or not.
     * @param isScraped true if the Open Graph object was created by scraping the Web, false if not
     */
    void setIsScraped(boolean isScraped);

    /**
     * Gets the Open Graph action which was created when this Open Graph action was posted, if it is a user-owned
     * object, otherwise null. The post action controls the privacy of this object.
     * @return the ID of the post action, if any, or null
     */
    String getPostActionId();

    /**
     * Sets the Open Graph action which was created when this Open Graph action was posted, if it is a user-owned
     * object, otherwise null. The post action controls the privacy of this object.
     * @param postActionId the ID of the post action, if any, or null
     */
    void setPostActionId(String postActionId);

    /**
     * Gets the type-specific properties of the Open Graph object, if any. Any custom properties that are defined on an
     * application-defined Open Graph object type will appear here.
     * @return a GraphObject containing the type-specific properties
     */
    GraphObject getData();

    /**
     * Sets the type-specific properties of the Open Graph object, if any. Any custom properties that are defined on an
     * application-defined Open Graph object type will appear here.
     * @param data a GraphObject containing the type-specific properties
     */
    void setData(GraphObject data);

    /**
     * Gets whether the object represents a new object that should be created as part of publishing via, e.g., the
     * native Share dialog. This flag has no effect on explicit publishing of an action via, e.g., a POST to the
     * '/me/objects/object_type' endpoint.
     * @return true if the native Share dialog should create the object as part of publishing an action, false if not
     */
    @PropertyName(NativeProtocol.OPEN_GRAPH_CREATE_OBJECT_KEY)
    boolean getCreateObject();

    /**
     * Sets whether the object represents a new object that should be created as part of publishing via, e.g., the
     * native Share dialog. This flag has no effect on explicit publishing of an action via, e.g., a POST to the
     * '/me/objects/object_type' endpoint.
     * @param createObject true if the native Share dialog should create the object as part of publishing an action,
     *                     false if not
     */
    @PropertyName(NativeProtocol.OPEN_GRAPH_CREATE_OBJECT_KEY)
    void setCreateObject(boolean createObject);

    /**
     * Exposes helpers for creating instances of OpenGraphObject.
     */
    final class Factory {
        /**
         * Creates an OpenGraphObject suitable for posting via, e.g., a native Share dialog. The object will have
         * no properties other than a 'create_object' and 'data' property, ready to be populated.
         * @param type the Open Graph object type for the object, or null if it will be specified later
         * @return an OpenGraphObject
         */
        public static OpenGraphObject createForPost(String type) {
            return createForPost(OpenGraphObject.class, type);
        }

        /**
         * Creates an OpenGraphObject suitable for posting via, e.g., a native Share dialog. The object will have
         * no properties other than a 'create_object' and 'data' property, ready to be populated.
         * @param graphObjectClass the OpenGraphObject-derived type to return
         * @param type the Open Graph object type for the object, or null if it will be specified later
         * @return an OpenGraphObject
         */
        public static <T extends OpenGraphObject> T createForPost(Class<T> graphObjectClass, String type) {
            return createForPost(graphObjectClass, type, null, null, null, null);
        }

        /**
         * Creates an OpenGraphObject suitable for posting via, e.g., a native Share dialog. The object will have
         * the specified properties, plus a 'create_object' and 'data' property, ready to be populated.
         * @param graphObjectClass the OpenGraphObject-derived type to return
         * @param type the Open Graph object type for the object, or null if it will be specified later
         * @param title the title of the object, or null if it will be specified later
         * @param imageUrl the URL of an image associated with the object, or null
         * @param url the URL associated with the object, or null
         * @param description the description of the object, or null
         * @return an OpenGraphObject
         */
        public static <T extends OpenGraphObject> T createForPost(Class<T> graphObjectClass, String type, String title,
                String imageUrl, String url, String description) {
            T object = GraphObject.Factory.create(graphObjectClass);

            if (type != null) {
                object.setType(type);
            }
            if (title != null) {
                object.setTitle(title);
            }
            if (imageUrl != null) {
                object.setImageUrls(Arrays.asList(imageUrl));
            }
            if (url != null) {
                object.setUrl(url);
            }
            if (description != null) {
                object.setDescription(description);
            }

            object.setCreateObject(true);
            object.setData(GraphObject.Factory.create());

            return object;
        }
    }
}
