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

package com.facebook.samples.rps;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.database.MatrixCursor;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.SimpleCursorAdapter;
import com.facebook.*;
import com.facebook.model.GraphMultiResult;
import com.facebook.model.GraphUser;
import com.facebook.widget.FriendPickerFragment;
import com.facebook.widget.PickerFragment;
import com.facebook.widget.WebDialog;

import java.text.SimpleDateFormat;
import java.util.*;

import static com.facebook.samples.rps.OpenGraphUtils.*;

public class FriendActivity extends FragmentActivity {
    private static final String TAG = FriendActivity.class.getName();
    private static final String INSTALLED = "installed";

    private FriendPickerFragment friendPickerFragment;
    private SimpleCursorAdapter friendActivityAdapter;
    private ProgressBar friendActivityProgressBar;
    private List<ActionRow> friendActionList;
    private Request pendingRequest;
    private String friendId;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.friends_activity);

        FragmentManager fragmentManager = getSupportFragmentManager();
        friendPickerFragment = (FriendPickerFragment) fragmentManager.findFragmentById(R.id.friend_fragment);
        friendPickerFragment.setShowTitleBar(false);

        ListView friendActivityList = (ListView) findViewById(R.id.friend_activity_list);
        String[] mapColumnNames = {"date", "action"};
        int[] mapViewIds = {R.id.friend_action_date, R.id.friend_game_result};
        friendActivityAdapter = new SimpleCursorAdapter(this, R.layout.friend_activity_row,
                createEmptyCursor(), mapColumnNames, mapViewIds);
        friendActivityList.setAdapter(friendActivityAdapter);
        friendActivityProgressBar = (ProgressBar) findViewById(R.id.friend_activity_progress_bar);

        friendPickerFragment.setOnErrorListener(new PickerFragment.OnErrorListener() {
            @Override
            public void onError(PickerFragment<?> fragment, FacebookException error) {
                FriendActivity.this.onError(error);
            }
        });
        friendPickerFragment.setUserId("me");
        friendPickerFragment.setMultiSelect(false);
        friendPickerFragment.setOnSelectionChangedListener(new PickerFragment.OnSelectionChangedListener() {
            @Override
            public void onSelectionChanged(PickerFragment<?> fragment) {
                FriendActivity.this.onFriendSelectionChanged();
            }
        });
        friendPickerFragment.setExtraFields(Arrays.asList(INSTALLED));
        friendPickerFragment.setFilter(new PickerFragment.GraphObjectFilter<GraphUser>() {
            @Override
            public boolean includeItem(GraphUser graphObject) {
                Boolean installed = graphObject.cast(GraphUserWithInstalled.class).getInstalled();
                return (installed != null) && installed.booleanValue();
            }
        });

        Button inviteButton = (Button) findViewById(R.id.invite_button);
        inviteButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                WebDialog.RequestsDialogBuilder builder =
                        new WebDialog.RequestsDialogBuilder(FriendActivity.this, Session.getActiveSession())
                                .setTitle(getString(R.string.invite_dialog_title))
                                .setMessage(getString(R.string.invite_dialog_message))
                                .setOnCompleteListener(new WebDialog.OnCompleteListener() {
                                    @Override
                                    public void onComplete(Bundle values, FacebookException error) {
                                        if (error != null) {
                                            Log.w(TAG, "Web dialog encountered an error.", error);
                                        } else {
                                            Log.i(TAG, "Web dialog complete: " + values);
                                        }
                                    }
                                });
                if (friendId != null) {
                    builder.setTo(friendId);
                }
                builder.build().show();
            }
        });
    }

    @Override
    public void onResume() {
        super.onResume();
        Session activeSession = Session.getActiveSession();
        if (activeSession == null || !activeSession.isOpened()) {
            new AlertDialog.Builder(this)
                    .setTitle(R.string.feature_requires_login_title)
                    .setMessage(R.string.feature_requires_login_message)
                    .setPositiveButton(R.string.error_ok_button, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            finish();
                        }
                    })
                    .show();
        } else {
            friendPickerFragment.loadData(false);
        }
    }

    private void onError(Exception error) {
        new AlertDialog.Builder(this)
                .setTitle(R.string.error_dialog_title)
                .setMessage(error.getLocalizedMessage())
                .setPositiveButton(R.string.error_ok_button, null)
                .show();
    }

    private <T> T chooseOne(List<T> ts) {
        for (T t : ts) {
            return t;
        }

        return null;
    }

    private void onFriendSelectionChanged() {
        GraphUser user = chooseOne(friendPickerFragment.getSelection());
        if (user != null) {
            friendId = user.getId();
            onChooseFriend();
        } else {
            friendActivityAdapter.changeCursor(createEmptyCursor());
        }
    }

    private void onChooseFriend() {
        friendActivityProgressBar.setVisibility(View.VISIBLE);

        String throwPath = String.format("%s/%s", friendId, ThrowAction.TYPE);
        pendingRequest = new Request(Session.getActiveSession(),
                throwPath,
                null,
                HttpMethod.GET,
                new Request.Callback() {
                    @Override
                    public void onCompleted(Response response) {
                        if (response.getRequest().equals(pendingRequest)) {
                            FriendActivity.this.onPostExecute(response);
                        }
                    }
                });
        pendingRequest.executeAsync();
    }

    private void onPostExecute(Response response) {
        friendActivityProgressBar.setVisibility(View.GONE);

        friendActionList = createActionRows(response);
        updateCursor(friendActionList);
    }

    private List<ActionRow> createActionRows(Response response) {
        ArrayList<ActionRow> publishedItems = new ArrayList<ActionRow>();

        if (response.getError() != null) {
            return Collections.emptyList();
        }

        GraphMultiResult list = response.getGraphObjectAs(GraphMultiResult.class);
        List<PublishedThrowAction> listData = list.getData().castToListOf(PublishedThrowAction.class);

        for (PublishedThrowAction action : listData) {
            publishedItems.add(createActionRow(action));
        }

        Collections.sort(publishedItems);
        return publishedItems;
    }

    private void updateCursor(Iterable<ActionRow> publishedItems) {
        MatrixCursor cursor = createEmptyCursor();
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd", Locale.getDefault());

        int id = 0;
        for (ActionRow item : publishedItems) {
            Object[] row = new Object[3];
            row[0] = id++;
            row[1] = dateFormat.format(item.publishDate);
            row[2] = item.actionText;
            cursor.addRow(row);
        }

        friendActivityAdapter.changeCursor(cursor);
        friendActivityAdapter.notifyDataSetChanged();
    }

    private MatrixCursor createEmptyCursor() {
        String[] cursorColumns = {"_ID", "date", "action"};
        return new MatrixCursor(cursorColumns);
    }

    private ActionRow createActionRow(PublishedThrowAction action) {
        String actionText = getActionText(action);
        Date publishDate = action.getPublishTime();

        return new ActionRow(actionText, publishDate);
    }

    private String getActionText(PublishedThrowAction action) {
        ThrowAction actionData = action.getData();
        if (actionData == null) {
            return "";
        }

        GestureGraphObject playerGesture = actionData.getGesture();
        GestureGraphObject opponentGesture = actionData.getOpposingGesture();

        if ((playerGesture == null) || (opponentGesture == null)) {
            return "";
        }

        String format = getString(R.string.action_display_format);
        return String.format(format, playerGesture.getTitle(), opponentGesture.getTitle());
    }

    private static class ActionRow implements Comparable<ActionRow>, Parcelable {
        final String actionText;
        final Date publishDate;

        ActionRow(String actionText, Date publishDate) {
            this.actionText = actionText;
            this.publishDate = publishDate;
        }

        @Override
        public int compareTo(ActionRow other) {
            if (other == null) {
                return 1;
            } else {
                return publishDate.compareTo(other.publishDate);
            }
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel parcel, int flags) {
            parcel.writeString(actionText);
            parcel.writeLong(publishDate.getTime());
        }

        @SuppressWarnings("unused")
        public final Creator<ActionRow> CREATOR = new Creator<ActionRow>() {
            @Override
            public ActionRow createFromParcel(Parcel parcel) {
                String actionText = parcel.readString();
                Date publishDate = new Date(parcel.readLong());
                return new ActionRow(actionText, publishDate);
            }

            @Override
            public ActionRow[] newArray(int size) {
                return new ActionRow[size];
            }
        };
    }


}
