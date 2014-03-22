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

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.*;
import android.widget.TextView;
import com.facebook.model.GraphUser;
import com.facebook.widget.ProfilePictureView;

public class ProfileFragment extends Fragment {

    public static final String TAG = "ProfileFragment";

    private TextView userNameView;
    private ProfilePictureView profilePictureView;
    private OnOptionsItemSelectedListener onOptionsItemSelectedListener;

    private GraphUser pendingUpdateForUser;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        inflater.inflate(R.menu.options_profile, menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        boolean handled = false;
        OnOptionsItemSelectedListener listener = onOptionsItemSelectedListener;
        if (listener != null) {
            handled = listener.onOptionsItemSelected(item);
        }

        if (!handled) {
            handled = super.onOptionsItemSelected(item);
        }

        return handled;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup parent, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_profile, parent, false);

        userNameView = (TextView)v.findViewById(R.id.profileUserName);
        profilePictureView = (ProfilePictureView)v.findViewById(R.id.profilePic);

        if (pendingUpdateForUser != null) {
            updateViewForUser(pendingUpdateForUser);
            pendingUpdateForUser = null;
        }

        return v;
    }

    public void setOnOptionsItemSelectedListener(OnOptionsItemSelectedListener listener) {
        this.onOptionsItemSelectedListener = listener;
    }

    public void updateViewForUser(GraphUser user) {
        if (userNameView == null || profilePictureView == null || !isAdded()) {
            // Fragment not yet added to the view. So let's store which user was intended
            // for display.
            pendingUpdateForUser = user;
            return;
        }

        if (user == null) {
            profilePictureView.setProfileId(null);
            userNameView.setText(R.string.greeting_no_user);
        } else {
            profilePictureView.setProfileId(user.getId());
            userNameView.setText(
                    String.format(getString(R.string.greeting_format), user.getFirstName()));
        }
    }

    public interface OnOptionsItemSelectedListener {
        boolean onOptionsItemSelected(MenuItem item);
    }
}
