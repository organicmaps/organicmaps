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

package com.facebook;

/**
 * Predefined event and parameter names for logging events common to many apps. Logging occurs through the
 * {@link AppEventsLogger#logEvent(String, android.os.Bundle)} family of methods.
 *
 * @beta
 */
public class AppEventsConstants {
    // Event names

    // General purpose

    /** Log this event when an app is being activated. */
    public static final String EVENT_NAME_ACTIVATED_APP = "fb_mobile_activate_app";

    /** Log this event when a user has completed registration with the app. */
    public static final String EVENT_NAME_COMPLETED_REGISTRATION = "fb_mobile_complete_registration";

    /** Log this event when a user has viewed a form of content in the app. */
    public static final String EVENT_NAME_VIEWED_CONTENT = "fb_mobile_content_view";

    /** Log this event when a user has performed a search within the app. */
    public static final String EVENT_NAME_SEARCHED = "fb_mobile_search";

    /**
     * Log this event when the user has rated an item in the app.
     * The valueToSum passed to logEvent should be the numeric rating.
     */
    public static final String EVENT_NAME_RATED = "fb_mobile_rate";

    /** Log this event when the user has completed a tutorial in the app. */
    public static final String EVENT_NAME_COMPLETED_TUTORIAL = "fb_mobile_tutorial_completion";

    // Ecommerce related

    /**
     * Log this event when the user has added an item to their cart.
     * The valueToSum passed to logEvent should be the item's price.
     */
    public static final String EVENT_NAME_ADDED_TO_CART = "fb_mobile_add_to_cart";

    /**
     * Log this event when the user has added an item to their wishlist.
     * The valueToSum passed to logEvent should be the item's price.
     */
    public static final String EVENT_NAME_ADDED_TO_WISHLIST = "fb_mobile_add_to_wishlist";

    /**
     * Log this event when the user has entered the checkout process.
     * The valueToSum passed to logEvent should be the total price in the cart.
     */
    public static final String EVENT_NAME_INITIATED_CHECKOUT = "fb_mobile_initiated_checkout";

    /** Log this event when the user has entered their payment info. */
    public static final String EVENT_NAME_ADDED_PAYMENT_INFO = "fb_mobile_add_payment_info";

    /**
     * Log this event when the user has completed a purchase.
     * The {@link AppEventsLogger#logPurchase(java.math.BigDecimal, java.util.Currency)} method is a shortcut for
     * logging this event.
     */
    public static final String EVENT_NAME_PURCHASED = "fb_mobile_purchase";

    // Gaming related

    /** Log this event when the user has achieved a level in the app. */
    public static final String EVENT_NAME_ACHIEVED_LEVEL = "fb_mobile_level_achieved";

    /** Log this event when the user has unlocked an achievement in the app. */
    public static final String EVENT_NAME_UNLOCKED_ACHIEVEMENT = "fb_mobile_achievement_unlocked";

    /**
     * Log this event when the user has spent app credits.
     * The valueToSum passed to logEvent should be the number of credits spent.
     */
    public static final String EVENT_NAME_SPENT_CREDITS = "fb_mobile_spent_credits";




    // Event parameters

    /**
     * Parameter key used to specify currency used with logged event.  E.g. "USD", "EUR", "GBP".
     * See ISO-4217 for specific values.  One reference for these is <http://en.wikipedia.org/wiki/ISO_4217>.
     */
    public static final String EVENT_PARAM_CURRENCY = "fb_currency";

    /**
     * Parameter key used to specify method user has used to register for the app, e.g., "Facebook", "email",
     * "Twitter", etc.
     */
    public static final String EVENT_PARAM_REGISTRATION_METHOD = "fb_registration_method";

    /**
     * Parameter key used to specify a generic content type/family for the logged event, e.g. "music", "photo",
     * "video".  Options to use will vary based upon what the app is all about.
     */
    public static final String EVENT_PARAM_CONTENT_TYPE = "fb_content_type";

    /**
     * Parameter key used to specify an ID for the specific piece of content being logged about.
     * Could be an EAN, article identifier, etc., depending on the nature of the app.
     */
    public static final String EVENT_PARAM_CONTENT_ID = "fb_content_id";

    /** Parameter key used to specify the string provided by the user for a search operation. */
    public static final String EVENT_PARAM_SEARCH_STRING = "fb_search_string";

    /**
     * Parameter key used to specify whether the activity being logged about was successful or not.
     * EVENT_PARAM_VALUE_YES and EVENT_PARAM_VALUE_NO are good canonical values to use for this parameter.
     */
    public static final String EVENT_PARAM_SUCCESS = "fb_success";

    /**
     * Parameter key used to specify the maximum rating available for the EVENT_NAME_RATE event.
     * E.g., "5" or "10".
     */
    public static final String EVENT_PARAM_MAX_RATING_VALUE = "fb_max_rating_value";

    /**
     * Parameter key used to specify whether payment info is available for the EVENT_NAME_INITIATED_CHECKOUT event.
     * EVENT_PARAM_VALUE_YES and EVENT_PARAM_VALUE_NO are good canonical values to use for this parameter.
     */
    public static final String EVENT_PARAM_PAYMENT_INFO_AVAILABLE = "fb_payment_info_available";

    /**
     * Parameter key used to specify how many items are being processed for an EVENT_NAME_INITIATED_CHECKOUT
     * or EVENT_NAME_PURCHASE event.
     */
    public static final String EVENT_PARAM_NUM_ITEMS = "fb_num_items";

    /** Parameter key used to specify the level achieved in a EVENT_NAME_LEVEL_ACHIEVED event. */
    public static final String EVENT_PARAM_LEVEL = "fb_level";

    /**
     * Parameter key used to specify a description appropriate to the event being logged.
     * E.g., the name of the achievement unlocked in the EVENT_NAME_ACHIEVEMENT_UNLOCKED event.
     */
    public static final String EVENT_PARAM_DESCRIPTION = "fb_description";



    // Parameter values

    /** Yes-valued parameter value to be used with parameter keys that need a Yes/No value */
    public static final String EVENT_PARAM_VALUE_YES = "1";

    /** No-valued parameter value to be used with parameter keys that need a Yes/No value */
    public static final String EVENT_PARAM_VALUE_NO = "0";

}
