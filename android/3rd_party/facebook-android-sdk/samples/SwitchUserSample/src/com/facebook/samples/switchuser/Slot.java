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

package com.facebook.samples.switchuser;

import android.content.Context;
import android.os.Bundle;
import com.facebook.*;
import com.facebook.model.GraphUser;

public class Slot {

    private static final String CACHE_NAME_FORMAT = "TokenCache%d";
    private static final String CACHE_USER_ID_KEY = "SwitchUserSampleUserId";
    private static final String CACHE_USER_NAME_KEY = "SwitchUserSampleUserName";

    private String tokenCacheName;
    private String userName;
    private String userId;
    private SharedPreferencesTokenCachingStrategy tokenCache;
    private SessionLoginBehavior loginBehavior;

    public Slot(Context context, int slotNumber, SessionLoginBehavior loginBehavior) {
        this.loginBehavior = loginBehavior;
        this.tokenCacheName = String.format(CACHE_NAME_FORMAT, slotNumber);
        this.tokenCache = new SharedPreferencesTokenCachingStrategy(
                context,
                tokenCacheName);

        restore();
    }

    public String getTokenCacheName() {
        return tokenCacheName;
    }

    public String getUserName() {
        return userName;
    }

    public String getUserId() {
        return userId;
    }

    public SessionLoginBehavior getLoginBehavior() {
        return loginBehavior;
    }

    public SharedPreferencesTokenCachingStrategy getTokenCache() {
        return tokenCache;
    }

    public void update(GraphUser user) {
        if (user == null) {
            return;
        }

        userId = user.getId();
        userName = user.getName();

        Bundle userInfo = tokenCache.load();
        userInfo.putString(CACHE_USER_ID_KEY, userId);
        userInfo.putString(CACHE_USER_NAME_KEY, userName);

        tokenCache.save(userInfo);
    }

    public void clear() {
        tokenCache.clear();
        restore();
    }

    private void restore() {
        Bundle userInfo = tokenCache.load();
        userId = userInfo.getString(CACHE_USER_ID_KEY);
        userName = userInfo.getString(CACHE_USER_NAME_KEY);
    }
}
