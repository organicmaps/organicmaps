/*
 * Author: Andreas Linde <mail@andreaslinde.de>
 *
 * Copyright (c) 2013-2014 HockeyApp, Bit Stadium GmbH.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HockeySDK_HockeySDKFeatureConfig_h
#define HockeySDK_HockeySDKFeatureConfig_h


/**
 * If true, include support for handling crash reports
 *
 * _Default_: Enabled
 */
#ifndef HOCKEYSDK_FEATURE_CRASH_REPORTER
#    define HOCKEYSDK_FEATURE_CRASH_REPORTER 1
#endif /* HOCKEYSDK_FEATURE_CRASH_REPORTER */


/**
 * If true, include support for managing user feedback
 *
 * _Default_: Enabled
 */
#ifndef HOCKEYSDK_FEATURE_FEEDBACK
#    define HOCKEYSDK_FEATURE_FEEDBACK 1
#endif /* HOCKEYSDK_FEATURE_FEEDBACK */


/**
 * If true, include support for informing the user about new updates pending in the App Store
 *
 * _Default_: Enabled
 */
#ifndef HOCKEYSDK_FEATURE_STORE_UPDATES
#    define HOCKEYSDK_FEATURE_STORE_UPDATES 1
#endif /* HOCKEYSDK_FEATURE_STORE_UPDATES */


/**
 * If true, include support for authentication installations for Ad-Hoc and Enterprise builds
 *
 * _Default_: Enabled
 */
#ifndef HOCKEYSDK_FEATURE_AUTHENTICATOR
#    define HOCKEYSDK_FEATURE_AUTHENTICATOR 1
#endif /* HOCKEYSDK_FEATURE_AUTHENTICATOR */


/**
 * If true, include support for handling in-app updates for Ad-Hoc and Enterprise builds
 *
 * _Default_: Enabled
 */
#ifndef HOCKEYSDK_FEATURE_UPDATES
#    define HOCKEYSDK_FEATURE_UPDATES 1
#endif /* HOCKEYSDK_FEATURE_UPDATES */


#endif /* HockeySDK_HockeySDKFeatureConfig_h */
