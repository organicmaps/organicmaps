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

package com.facebook.scrumptious;

import android.app.Application;
import com.facebook.model.GraphPlace;
import com.facebook.model.GraphUser;

import java.util.List;

/**
 * Use a custom Application class to pass state data between Activities.
 */
public class ScrumptiousApplication extends Application {

    private List<GraphUser> selectedUsers;
    private GraphPlace selectedPlace;

    public List<GraphUser> getSelectedUsers() {
        return selectedUsers;
    }

    public void setSelectedUsers(List<GraphUser> users) {
        selectedUsers = users;
    }

    public GraphPlace getSelectedPlace() {
        return selectedPlace;
    }

    public void setSelectedPlace(GraphPlace place) {
        this.selectedPlace = place;
    }
}
