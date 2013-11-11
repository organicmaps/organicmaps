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
 * Provides a strongly-typed representation of a Location as defined by the Graph API.
 *
 * Note that this interface is intended to be used with GraphObject.Factory
 * and not implemented directly.
 */
public interface GraphLocation extends GraphObject {
    /**
     * Returns the street component of the location.
     * 
     * @return the street component of the location, or null
     */
    public String getStreet();

    /**
     * Sets the street component of the location.
     * 
     * @param street
     *            the street component of the location, or null
     */
    public void setStreet(String street);

    /**
     * Gets the city component of the location.
     * 
     * @return the city component of the location
     */
    public String getCity();

    /**
     * Sets the city component of the location.
     * 
     * @param city
     *            the city component of the location
     */
    public void setCity(String city);

    /**
     * Returns the state component of the location.
     * 
     * @return the state component of the location
     */
    public String getState();

    /**
     * Sets the state component of the location.
     * 
     * @param state
     *            the state component of the location
     */
    public void setState(String state);

    /**
     * Returns the country component of the location.
     * 
     * @return the country component of the location
     */
    public String getCountry();

    /**
     * Sets the country component of the location
     * 
     * @param country
     *            the country component of the location
     */
    public void setCountry(String country);

    /**
     * Returns the postal code component of the location.
     * 
     * @return the postal code component of the location
     */
    public String getZip();

    /**
     * Sets the postal code component of the location.
     * 
     * @param zip
     *            the postal code component of the location
     */
    public void setZip(String zip);

    /**
     * Returns the latitude component of the location.
     * 
     * @return the latitude component of the location
     */
    public double getLatitude();

    /**
     * Sets the latitude component of the location.
     * 
     * @param latitude
     *            the latitude component of the location
     */
    public void setLatitude(double latitude);

    /**
     * Returns the longitude component of the location.
     * 
     * @return the longitude component of the location
     */
    public double getLongitude();

    /**
     * Sets the longitude component of the location.
     * 
     * @param longitude
     *            the longitude component of the location
     */
    public void setLongitude(double longitude);
}
