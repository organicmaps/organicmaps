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

import android.content.Context;
import android.os.Bundle;
import com.facebook.internal.Logger;

import java.math.BigDecimal;
import java.util.Currency;

/**
 * This class is deprecated. Please use {@link AppEventsLogger} instead.
 */
@Deprecated
public class InsightsLogger {
    private static final String EVENT_PARAMETER_PIXEL_ID         = "fb_offsite_pixel_id";
    private static final String EVENT_PARAMETER_PIXEL_VALUE      = "fb_offsite_pixel_value";

    private static final String EVENT_NAME_LOG_CONVERSION_PIXEL  = "fb_log_offsite_pixel";

    private AppEventsLogger appEventsLogger;

    private InsightsLogger(Context context, String applicationId, Session session) {
        appEventsLogger = AppEventsLogger.newLogger(context, applicationId, session);
    }

    /**
     * Deprecated. Please use {@link AppEventsLogger} instead.
     */
    public static InsightsLogger newLogger(Context context, String clientToken) {
        return new InsightsLogger(context, null, null);
    }

    /**
     * Deprecated. Please use {@link AppEventsLogger} instead.
     */
    public static InsightsLogger newLogger(Context context, String clientToken, String applicationId) {
        return new InsightsLogger(context, applicationId, null);
    }

    /**
     * Deprecated. Please use {@link AppEventsLogger} instead.
     */
    public static InsightsLogger newLogger(Context context, String clientToken, String applicationId, Session session) {
        return new InsightsLogger(context, applicationId, session);
    }

    /**
     * Deprecated. Please use {@link AppEventsLogger} instead.
     */
    public void logPurchase(BigDecimal purchaseAmount, Currency currency) {
        logPurchase(purchaseAmount, currency, null);
    }

    /**
     * Deprecated. Please use {@link AppEventsLogger} instead.
     */
    public void logPurchase(BigDecimal purchaseAmount, Currency currency, Bundle parameters) {
        appEventsLogger.logPurchase(purchaseAmount, currency, parameters);
    }

    /**
     * Deprecated. Please use {@link AppEventsLogger} instead.
     */
    public void logConversionPixel(String pixelId, double valueOfPixel) {

        if (pixelId == null) {
            Logger.log(LoggingBehavior.DEVELOPER_ERRORS, "Insights", "pixelID cannot be null");
            return;
        }

        Bundle parameters = new Bundle();
        parameters.putString(EVENT_PARAMETER_PIXEL_ID, pixelId);
        parameters.putDouble(EVENT_PARAMETER_PIXEL_VALUE, valueOfPixel);

        appEventsLogger.logEvent(EVENT_NAME_LOG_CONVERSION_PIXEL, valueOfPixel, parameters);
        AppEventsLogger.eagerFlush();
    }
}
