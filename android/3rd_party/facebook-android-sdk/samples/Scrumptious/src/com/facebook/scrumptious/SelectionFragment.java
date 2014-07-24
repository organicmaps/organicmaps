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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.*;
import com.facebook.*;
import com.facebook.internal.Utility;
import com.facebook.model.*;
import com.facebook.widget.FacebookDialog;
import com.facebook.widget.ProfilePictureView;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Fragment that represents the main selection screen for Scrumptious.
 */
public class SelectionFragment extends Fragment {

    private static final String TAG = "SelectionFragment";
    private static final String MEAL_OBJECT_TYPE = "fb_sample_scrumps:meal";
    private static final String EAT_ACTION_TYPE = "fb_sample_scrumps:eat";
    private static final String DEFAULT_ACTION_IMAGE_URL =
            "http://facebooksampleapp.com/scrumptious/static/images/logo.png";

    private static final String PENDING_ANNOUNCE_KEY = "pendingAnnounce";
    private static final Uri M_FACEBOOK_URL = Uri.parse("http://m.facebook.com");
    private static final int USER_GENERATED_MIN_SIZE = 480;

    private static final int REAUTH_ACTIVITY_CODE = 100;
    private static final String PERMISSION = "publish_actions";

    private TextView announceButton;
    private TextView messageButton;
    private ListView listView;
    private ProgressDialog progressDialog;
    private List<BaseListElement> listElements;
    private ProfilePictureView profilePictureView;
    private boolean pendingAnnounce;
    private MainActivity activity;
    private Uri photoUri;
    private ImageView photoThumbnail;

    private UiLifecycleHelper uiHelper;
    private Session.StatusCallback sessionCallback = new Session.StatusCallback() {
        @Override
        public void call(final Session session, final SessionState state, final Exception exception) {
            onSessionStateChange(session, state, exception);
        }
    };
    private FacebookDialog.Callback nativeDialogCallback = new FacebookDialog.Callback() {
        @Override
        public void onComplete(FacebookDialog.PendingCall pendingCall, Bundle data) {
            boolean resetSelections = true;
            if (FacebookDialog.getNativeDialogDidComplete(data)) {
                if (FacebookDialog.COMPLETION_GESTURE_CANCEL
                        .equals(FacebookDialog.getNativeDialogCompletionGesture(data))) {
                    // Leave selections alone if user canceled.
                    resetSelections = false;
                    showCancelResponse();
                } else {
                    showSuccessResponse(FacebookDialog.getNativeDialogPostId(data));
                }
            }

            if (resetSelections) {
                init(null);
            }
        }

        @Override
        public void onError(FacebookDialog.PendingCall pendingCall, Exception error, Bundle data) {
            new AlertDialog.Builder(getActivity())
                    .setPositiveButton(R.string.error_dialog_button_text, null)
                    .setTitle(R.string.error_dialog_title)
                    .setMessage(error.getLocalizedMessage())
                    .show();
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        activity = (MainActivity) getActivity();
        uiHelper = new UiLifecycleHelper(getActivity(), sessionCallback);
        uiHelper.onCreate(savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        uiHelper.onResume();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        super.onCreateView(inflater, container, savedInstanceState);
        View view = inflater.inflate(R.layout.selection, container, false);

        profilePictureView = (ProfilePictureView) view.findViewById(R.id.selection_profile_pic);
        profilePictureView.setCropped(true);
        announceButton = (TextView) view.findViewById(R.id.announce_text);
        messageButton = (TextView) view.findViewById(R.id.message_text);
        listView = (ListView) view.findViewById(R.id.selection_list);
        photoThumbnail = (ImageView) view.findViewById(R.id.selected_image);

        if (FacebookDialog.canPresentOpenGraphMessageDialog(activity)) {
            messageButton.setVisibility(View.VISIBLE);
        }

        announceButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                handleAnnounce(false);
            }
        });
        messageButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                handleAnnounce(true);
            }
        });

        init(savedInstanceState);

        return view;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == Activity.RESULT_OK && requestCode >= 0 && requestCode < listElements.size()) {
            listElements.get(requestCode).onActivityResult(data);
        } else {
            uiHelper.onActivityResult(requestCode, resultCode, data, nativeDialogCallback);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle bundle) {
        super.onSaveInstanceState(bundle);
        for (BaseListElement listElement : listElements) {
            listElement.onSaveInstanceState(bundle);
        }
        bundle.putBoolean(PENDING_ANNOUNCE_KEY, pendingAnnounce);
        uiHelper.onSaveInstanceState(bundle);
    }

    @Override
    public void onPause() {
        super.onPause();
        uiHelper.onPause();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        uiHelper.onDestroy();
        activity = null;
    }

    /**
     * Notifies that the session token has been updated.
     */
    private void tokenUpdated() {
        if (pendingAnnounce) {
            handleAnnounce(false);
        }
    }

    private void onSessionStateChange(final Session session, SessionState state, Exception exception) {
        if (session != null && session.isOpened()) {
            if (state.equals(SessionState.OPENED_TOKEN_UPDATED)) {
                tokenUpdated();
            } else {
                makeMeRequest(session);
            }
        } else {
            profilePictureView.setProfileId(null);
        }
    }

    private void makeMeRequest(final Session session) {
        Request request = Request.newMeRequest(session, new Request.GraphUserCallback() {
            @Override
            public void onCompleted(GraphUser user, Response response) {
                if (session == Session.getActiveSession()) {
                    if (user != null) {
                        profilePictureView.setProfileId(user.getId());
                    }
                }
                if (response.getError() != null) {
                    handleError(response.getError());
                }
            }
        });
        request.executeAsync();

    }

    /**
     * Resets the view to the initial defaults.
     */
    private void init(Bundle savedInstanceState) {
        announceButton.setEnabled(false);
        messageButton.setEnabled(false);

        listElements = new ArrayList<BaseListElement>();

        listElements.add(new EatListElement(0));
        listElements.add(new LocationListElement(1));
        listElements.add(new PeopleListElement(2));
        listElements.add(new PhotoListElement(3));

        if (savedInstanceState != null) {
            for (BaseListElement listElement : listElements) {
                listElement.restoreState(savedInstanceState);
            }
            pendingAnnounce = savedInstanceState.getBoolean(PENDING_ANNOUNCE_KEY, false);
        }

        listView.setAdapter(new ActionListAdapter(getActivity(), R.id.selection_list, listElements));

        Session session = Session.getActiveSession();
        if (session != null && session.isOpened()) {
            makeMeRequest(session);
        }
    }

    private void handleAnnounce(boolean isMessage) {
        pendingAnnounce = false;
        Session session = Session.getActiveSession();

        // if we have a session, then use the graph API to directly publish, otherwise use
        // the native open graph share dialog.
        if (session != null && session.isOpened()) {
            handleGraphApiAnnounce();
        } else {
            if (isMessage) {
                handleNativeMessageAnnounce();
            } else {
                handleNativeShareAnnounce();
            }
        }
    }

    private void handleGraphApiAnnounce() {
        Session session = Session.getActiveSession();

        List<String> permissions = session.getPermissions();
        if (!permissions.contains(PERMISSION)) {
            pendingAnnounce = true;
            requestPublishPermissions(session);
            return;
        }

        // Show a progress dialog because sometimes the requests can take a while.
        progressDialog = ProgressDialog.show(getActivity(), "",
                getActivity().getResources().getString(R.string.progress_dialog_text), true);

        // Run this in a background thread so we can process the list of responses and extract errors.
        AsyncTask<Void, Void, List<Response>> task = new AsyncTask<Void, Void, List<Response>>() {

            @Override
            protected List<Response> doInBackground(Void... voids) {
                EatAction eatAction = createEatAction();

                RequestBatch requestBatch = new RequestBatch();

                String photoStagingUri = null;

                if (photoUri != null) {
                    try {
                        Pair<File, Integer> fileAndMinDimemsion = getImageFileAndMinDimension();
                        if (fileAndMinDimemsion != null) {
                            Request photoStagingRequest =
                                    Request.newUploadStagingResourceWithImageRequest(Session.getActiveSession(),
                                            fileAndMinDimemsion.first, null);
                            photoStagingRequest.setBatchEntryName("photoStaging");
                            requestBatch.add(photoStagingRequest);
                            // Facebook SDK * pro-tip *
                            // We can use the result from one request in the batch as the input to another request.
                            // In this case, the result from the staging upload is "uri", which we will use as the
                            // input into the "url" field for images on the open graph action below.
                            photoStagingUri = "{result=photoStaging:$.uri}";
                            eatAction.setImage(getImageListForAction(photoStagingUri,
                                    fileAndMinDimemsion.second >= USER_GENERATED_MIN_SIZE));
                        }
                    } catch (FileNotFoundException e) {
                        // NOOP - if we can't upload the image, just skip it for now
                    }
                }
                MealGraphObject meal = eatAction.getMeal();
                if (meal.getCreateObject()) {
                    Request createObjectRequest =
                            Request.newPostOpenGraphObjectRequest(Session.getActiveSession(), meal, null);
                    createObjectRequest.setBatchEntryName("createObject");
                    requestBatch.add(createObjectRequest);
                    eatAction.setProperty("meal", "{result=createObject:$.id}");
                }

                Request request = Request.newPostOpenGraphActionRequest(Session.getActiveSession(), eatAction, null);
                requestBatch.add(request);

                return requestBatch.executeAndWait();
            }

            @Override
            protected void onPostExecute(List<Response> responses) {
                // We only care about the last response, or the first one with an error.
                Response finalResponse = null;
                for (Response response : responses) {
                    finalResponse = response;
                    if (response != null && response.getError() != null) {
                        break;
                    }
                }
                onPostActionResponse(finalResponse);
            }
        };

        task.execute();
    }

    private void handleNativeShareAnnounce() {
        FacebookDialog.OpenGraphActionDialogBuilder builder = createDialogBuilder();
        if (builder.canPresent()) {
            uiHelper.trackPendingDialogCall(builder.build().present());
        } else {
            // If we can't show the native open graph share dialog because the Facebook app
            // does not support it, then show then settings fragment so the user can log in.
            activity.showSettingsFragment();
        }
    }

    private FacebookDialog.OpenGraphActionDialogBuilder createDialogBuilder() {
        EatAction eatAction = createEatAction();

        boolean userGenerated = false;
        if (photoUri != null) {
            String photoUriString = photoUri.toString();
            Pair<File, Integer> fileAndMinDimemsion = getImageFileAndMinDimension();
            userGenerated = fileAndMinDimemsion.second >= USER_GENERATED_MIN_SIZE;

            // If we have a content: URI, we can just use that URI, otherwise we'll need to add it as an attachment.
            if (fileAndMinDimemsion != null && photoUri.getScheme().startsWith("content")) {
                eatAction.setImage(getImageListForAction(photoUriString, userGenerated));
            }
        }

        FacebookDialog.OpenGraphActionDialogBuilder builder = new FacebookDialog.OpenGraphActionDialogBuilder(
                getActivity(), eatAction, "meal")
                .setFragment(SelectionFragment.this);

        if (photoUri != null && !photoUri.getScheme().startsWith("content")) {
            builder.setImageAttachmentFilesForAction(Arrays.asList(new File(photoUri.getPath())), userGenerated);
        }

        return builder;
    }

    private void handleNativeMessageAnnounce() {
        FacebookDialog.OpenGraphMessageDialogBuilder builder = createMessageDialogBuilder();
        if (builder.canPresent()) {
            uiHelper.trackPendingDialogCall(builder.build().present());
        } else {
            // If we can't show the native open graph share dialog because the Messenger app
            // does not support it, then show then settings fragment so the user can log in.
            activity.showSettingsFragment();
        }
    }

    private FacebookDialog.OpenGraphMessageDialogBuilder createMessageDialogBuilder() {
        EatAction eatAction = createEatAction();

        boolean userGenerated = false;
        if (photoUri != null) {
            String photoUriString = photoUri.toString();
            Pair<File, Integer> fileAndMinDimemsion = getImageFileAndMinDimension();
            userGenerated = fileAndMinDimemsion.second >= USER_GENERATED_MIN_SIZE;

            // If we have a content: URI, we can just use that URI, otherwise we'll need to add it as an attachment.
            if (fileAndMinDimemsion != null && photoUri.getScheme().startsWith("content")) {
                eatAction.setImage(getImageListForAction(photoUriString, userGenerated));
            }
        }

        FacebookDialog.OpenGraphMessageDialogBuilder builder = new FacebookDialog.OpenGraphMessageDialogBuilder(
                getActivity(), eatAction, "meal")
                .setFragment(SelectionFragment.this);

        if (photoUri != null && !photoUri.getScheme().startsWith("content")) {
            builder.setImageAttachmentFilesForAction(Arrays.asList(new File(photoUri.getPath())), userGenerated);
        }

        return builder;
    }

    private Pair<File, Integer> getImageFileAndMinDimension() {
        File photoFile = null;
        String photoUriString = photoUri.toString();
        if (photoUriString.startsWith("file://")) {
            photoFile = new File(photoUri.getPath());
        } else if (photoUriString.startsWith("content://")) {
            String [] filePath = { MediaStore.Images.Media.DATA };
            Cursor cursor = getActivity().getContentResolver().query(photoUri, filePath, null, null, null);
            if (cursor != null) {
                cursor.moveToFirst();
                int columnIndex = cursor.getColumnIndex(filePath[0]);
                String filename = cursor.getString(columnIndex);
                cursor.close();

                photoFile = new File(filename);
            }
        }

        if (photoFile != null) {
            InputStream is = null;
            try {
                is = new FileInputStream(photoFile);

                // We only want to get the bounds of the image, rather than load the whole thing.
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inJustDecodeBounds = true;
                BitmapFactory.decodeStream(is, null, options);

                return new Pair<File, Integer>(photoFile, Math.min(options.outWidth, options.outHeight));
            } catch (Exception e) {
                return null;
            } finally {
                Utility.closeQuietly(is);
            }
        }
        return null;
    }

    /**
     * Creates a GraphObject with the following format:
     * {
     *     url: ${uri},
     *     user_generated: true
     * }
     */
    private GraphObject getImageObject(String uri, boolean userGenerated) {
        GraphObject imageObject = GraphObject.Factory.create();
        imageObject.setProperty("url", uri);
        if (userGenerated) {
            imageObject.setProperty("user_generated", "true");
        }
        return imageObject;
    }

    private List<JSONObject> getImageListForAction(String uri, boolean userGenerated) {
        return Arrays.asList(getImageObject(uri, userGenerated).getInnerJSONObject());
    }

    private EatAction createEatAction() {
        EatAction eatAction = OpenGraphAction.Factory.createForPost(EatAction.class, EAT_ACTION_TYPE);
        for (BaseListElement element : listElements) {
            element.populateOGAction(eatAction);
        }
        return eatAction;
    }

    private void requestPublishPermissions(Session session) {
        if (session != null) {
            Session.NewPermissionsRequest newPermissionsRequest = new Session.NewPermissionsRequest(this, PERMISSION)
                    // demonstrate how to set an audience for the publish permissions,
                    // if none are set, this defaults to FRIENDS
                    .setDefaultAudience(SessionDefaultAudience.FRIENDS)
                    .setRequestCode(REAUTH_ACTIVITY_CODE);
            session.requestNewPublishPermissions(newPermissionsRequest);
        }
    }

    private void onPostActionResponse(Response response) {
        if (progressDialog != null) {
            progressDialog.dismiss();
            progressDialog = null;
        }
        if (getActivity() == null) {
            // if the user removes the app from the website, then a request will
            // have caused the session to close (since the token is no longer valid),
            // which means the splash fragment will be shown rather than this one,
            // causing activity to be null. If the activity is null, then we cannot
            // show any dialogs, so we return.
            return;
        }

        PostResponse postResponse = response.getGraphObjectAs(PostResponse.class);

        if (postResponse != null && postResponse.getId() != null) {
            showSuccessResponse(postResponse.getId());
            init(null);
        } else {
            handleError(response.getError());
        }
    }

    private void showSuccessResponse(String postId) {
        String dialogBody;
        if (postId != null) {
            dialogBody = String.format(getString(R.string.result_dialog_text_with_id), postId);
        } else {
            dialogBody = getString(R.string.result_dialog_text_default);
        }
        showResultDialog(dialogBody);
    }

    private void showCancelResponse() {
        showResultDialog(getString(R.string.result_dialog_text_canceled));
    }

    private void showResultDialog(String dialogBody) {
        new AlertDialog.Builder(getActivity())
                .setPositiveButton(R.string.result_dialog_button_text, null)
                .setTitle(R.string.result_dialog_title)
                .setMessage(dialogBody)
                .show();
    }

    private void handleError(FacebookRequestError error) {
        DialogInterface.OnClickListener listener = null;
        String dialogBody = null;

        if (error == null) {
            dialogBody = getString(R.string.error_dialog_default_text);
        } else {
            switch (error.getCategory()) {
                case AUTHENTICATION_RETRY:
                    // tell the user what happened by getting the message id, and
                    // retry the operation later
                    String userAction = (error.shouldNotifyUser()) ? "" :
                            getString(error.getUserActionMessageId());
                    dialogBody = getString(R.string.error_authentication_retry, userAction);
                    listener = new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            Intent intent = new Intent(Intent.ACTION_VIEW, M_FACEBOOK_URL);
                            startActivity(intent);
                        }
                    };
                    break;

                case AUTHENTICATION_REOPEN_SESSION:
                    // close the session and reopen it.
                    dialogBody = getString(R.string.error_authentication_reopen);
                    listener = new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            Session session = Session.getActiveSession();
                            if (session != null && !session.isClosed()) {
                                session.closeAndClearTokenInformation();
                            }
                        }
                    };
                    break;

                case PERMISSION:
                    // request the publish permission
                    dialogBody = getString(R.string.error_permission);
                    listener = new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            pendingAnnounce = true;
                            requestPublishPermissions(Session.getActiveSession());
                        }
                    };
                    break;

                case SERVER:
                case THROTTLING:
                    // this is usually temporary, don't clear the fields, and
                    // ask the user to try again
                    dialogBody = getString(R.string.error_server);
                    break;

                case BAD_REQUEST:
                    // this is likely a coding error, ask the user to file a bug
                    dialogBody = getString(R.string.error_bad_request, error.getErrorMessage());
                    break;

                case OTHER:
                case CLIENT:
                default:
                    // an unknown issue occurred, this could be a code error, or
                    // a server side issue, log the issue, and either ask the
                    // user to retry, or file a bug
                    dialogBody = getString(R.string.error_unknown, error.getErrorMessage());
                    break;
            }
        }

        new AlertDialog.Builder(getActivity())
                .setPositiveButton(R.string.error_dialog_button_text, listener)
                .setTitle(R.string.error_dialog_title)
                .setMessage(dialogBody)
                .show();
    }

    private void startPickerActivity(Uri data, int requestCode) {
        Intent intent = new Intent();
        intent.setData(data);
        intent.setClass(getActivity(), PickerActivity.class);
        startActivityForResult(intent, requestCode);
    }

    /**
     * Interface representing the Meal Open Graph object.
     */
    private interface MealGraphObject extends OpenGraphObject {
        public String getUrl();
        public void setUrl(String url);

        public String getId();
        public void setId(String id);
    }

    /**
     * Interface representing the Eat action.
     */
    private interface EatAction extends OpenGraphAction {
        public MealGraphObject getMeal();
        public void setMeal(MealGraphObject meal);
    }

    /**
     * Used to inspect the response from posting an action
     */
    private interface PostResponse extends GraphObject {
        String getId();
    }

    private class EatListElement extends BaseListElement {

        private static final String FOOD_KEY = "food";
        private static final String FOOD_URL_KEY = "food_url";

        private final String[] foodChoices;
        private final String[] foodUrls;
        private String foodChoiceUrl = null;
        private String foodChoice = null;

        public EatListElement(int requestCode) {
            super(getActivity().getResources().getDrawable(R.drawable.add_food),
                    getActivity().getResources().getString(R.string.action_eating),
                    null,
                    requestCode);
            foodChoices = getActivity().getResources().getStringArray(R.array.food_types);
            foodUrls = getActivity().getResources().getStringArray(R.array.food_og_urls);
        }

        @Override
        protected View.OnClickListener getOnClickListener() {
            return new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    showMealOptions();
                }
            };
        }

        @Override
        protected void populateOGAction(OpenGraphAction action) {
            if (foodChoice != null && foodChoice.length() > 0) {
                EatAction eatAction = action.cast(EatAction.class);
                eatAction.setImageUrls(Arrays.asList(DEFAULT_ACTION_IMAGE_URL));
                if (foodChoiceUrl != null && foodChoiceUrl.length() > 0) {
                    MealGraphObject meal = GraphObject.Factory.create(MealGraphObject.class);
                    meal.setUrl(foodChoiceUrl);
                    eatAction.setMeal(meal);
                } else {
                    MealGraphObject meal = OpenGraphObject.Factory.createForPost(MealGraphObject.class,
                            MEAL_OBJECT_TYPE);
                    meal.setTitle(foodChoice);
                    eatAction.setMeal(meal);
                }
            }
        }

        @Override
        protected void onSaveInstanceState(Bundle bundle) {
            if (foodChoice != null && foodChoiceUrl != null) {
                bundle.putString(FOOD_KEY, foodChoice);
                bundle.putString(FOOD_URL_KEY, foodChoiceUrl);
            }
        }

        @Override
        protected boolean restoreState(Bundle savedState) {
            String food = savedState.getString(FOOD_KEY);
            String foodUrl = savedState.getString(FOOD_URL_KEY);
            if (food != null && foodUrl != null) {
                foodChoice = food;
                foodChoiceUrl = foodUrl;
                setFoodText();
                return true;
            }
            return false;
        }

        private void showMealOptions() {
            String title = getActivity().getResources().getString(R.string.select_meal);
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            builder.setTitle(title).
                    setCancelable(true).
                    setItems(foodChoices, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            foodChoiceUrl = foodUrls[i];
                            if (foodChoiceUrl.length() == 0) {
                                getCustomFood();
                            } else {
                                foodChoice = foodChoices[i];
                                setFoodText();
                                notifyDataChanged();
                            }
                        }
                    });
            builder.show();
        }

        private void getCustomFood() {
            String title = getActivity().getResources().getString(R.string.enter_meal);
            final EditText input = new EditText(getActivity());

            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            builder.setTitle(title)
                    .setCancelable(true)
                    .setView(input)
                    .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            foodChoice = input.getText().toString();
                            setFoodText();
                            notifyDataChanged();
                        }
                    })
                    .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                        }
                    });
            AlertDialog dialog = builder.create();
            // always popup the keyboard when the alert dialog shows
            dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
            dialog.show();
        }

        private void setFoodText() {
            if (foodChoice != null && foodChoice.length() > 0) {
                setText2(foodChoice);
                announceButton.setEnabled(true);
                messageButton.setEnabled(true);
            } else {
                setText2(getActivity().getResources().getString(R.string.action_eating_default));
                announceButton.setEnabled(false);
                messageButton.setEnabled(false);
            }
        }
    }

    private class PeopleListElement extends BaseListElement {

        private static final String FRIENDS_KEY = "friends";

        private List<GraphUser> selectedUsers;

        public PeopleListElement(int requestCode) {
            super(getActivity().getResources().getDrawable(R.drawable.add_friends),
                    getActivity().getResources().getString(R.string.action_people),
                    null,
                    requestCode);
        }

        @Override
        protected View.OnClickListener getOnClickListener() {
            return new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (Session.getActiveSession() != null &&
                            Session.getActiveSession().isOpened()) {
                        startPickerActivity(PickerActivity.FRIEND_PICKER, getRequestCode());
                    } else {
                        activity.showSettingsFragment();
                    }
                }
            };
        }

        @Override
        protected void onActivityResult(Intent data) {
            selectedUsers = ((ScrumptiousApplication) getActivity().getApplication()).getSelectedUsers();
            setUsersText();
            notifyDataChanged();
        }

        @Override
        protected void populateOGAction(OpenGraphAction action) {
            if (selectedUsers != null) {
                action.setTags(selectedUsers);
            }
        }

        @Override
        protected void onSaveInstanceState(Bundle bundle) {
            if (selectedUsers != null) {
                bundle.putByteArray(FRIENDS_KEY, getByteArray(selectedUsers));
            }
        }

        @Override
        protected boolean restoreState(Bundle savedState) {
            byte[] bytes = savedState.getByteArray(FRIENDS_KEY);
            if (bytes != null) {
                selectedUsers = restoreByteArray(bytes);
                setUsersText();
                return true;
            }
            return false;
        }

        private void setUsersText() {
            String text = null;
            if (selectedUsers != null) {
                if (selectedUsers.size() == 1) {
                    text = String.format(getResources().getString(R.string.single_user_selected),
                            selectedUsers.get(0).getName());
                } else if (selectedUsers.size() == 2) {
                    text = String.format(getResources().getString(R.string.two_users_selected),
                            selectedUsers.get(0).getName(), selectedUsers.get(1).getName());
                } else if (selectedUsers.size() > 2) {
                    text = String.format(getResources().getString(R.string.multiple_users_selected),
                            selectedUsers.get(0).getName(), (selectedUsers.size() - 1));
                }
            }
            if (text == null) {
                text = getResources().getString(R.string.action_people_default);
            }
            setText2(text);
        }

        private byte[] getByteArray(List<GraphUser> users) {
            // convert the list of GraphUsers to a list of String where each element is
            // the JSON representation of the GraphUser so it can be stored in a Bundle
            List<String> usersAsString = new ArrayList<String>(users.size());

            for (GraphUser user : users) {
                usersAsString.add(user.getInnerJSONObject().toString());
            }
            try {
                ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                new ObjectOutputStream(outputStream).writeObject(usersAsString);
                return outputStream.toByteArray();
            } catch (IOException e) {
                Log.e(TAG, "Unable to serialize users.", e);
            }
            return null;
        }

        private List<GraphUser> restoreByteArray(byte[] bytes) {
            try {
                @SuppressWarnings("unchecked")
                List<String> usersAsString =
                        (List<String>) (new ObjectInputStream(new ByteArrayInputStream(bytes))).readObject();
                if (usersAsString != null) {
                    List<GraphUser> users = new ArrayList<GraphUser>(usersAsString.size());
                    for (String user : usersAsString) {
                        GraphUser graphUser = GraphObject.Factory
                                .create(new JSONObject(user), GraphUser.class);
                        users.add(graphUser);
                    }
                    return users;
                }
            } catch (ClassNotFoundException e) {
                Log.e(TAG, "Unable to deserialize users.", e);
            } catch (IOException e) {
                Log.e(TAG, "Unable to deserialize users.", e);
            } catch (JSONException e) {
                Log.e(TAG, "Unable to deserialize users.", e);
            }
            return null;
        }
    }

    private class LocationListElement extends BaseListElement {

        private static final String PLACE_KEY = "place";

        private GraphPlace selectedPlace = null;

        public LocationListElement(int requestCode) {
            super(getActivity().getResources().getDrawable(R.drawable.add_location),
                    getActivity().getResources().getString(R.string.action_location),
                    null,
                    requestCode);
        }

        @Override
        protected View.OnClickListener getOnClickListener() {
            return new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (Session.getActiveSession() != null &&
                            Session.getActiveSession().isOpened()) {
                        startPickerActivity(PickerActivity.PLACE_PICKER, getRequestCode());
                    } else {
                        activity.showSettingsFragment();
                    }
                }
            };
        }

        @Override
        protected void onActivityResult(Intent data) {
            selectedPlace = ((ScrumptiousApplication) getActivity().getApplication()).getSelectedPlace();
            setPlaceText();
            notifyDataChanged();
        }

        @Override
        protected void populateOGAction(OpenGraphAction action) {
            if (selectedPlace != null) {
                action.setPlace(selectedPlace);
            }
        }

        @Override
        protected void onSaveInstanceState(Bundle bundle) {
            if (selectedPlace != null) {
                bundle.putString(PLACE_KEY, selectedPlace.getInnerJSONObject().toString());
            }
        }

        @Override
        protected boolean restoreState(Bundle savedState) {
            String place = savedState.getString(PLACE_KEY);
            if (place != null) {
                try {
                    selectedPlace = GraphObject.Factory
                            .create(new JSONObject(place), GraphPlace.class);
                    setPlaceText();
                    return true;
                } catch (JSONException e) {
                    Log.e(TAG, "Unable to deserialize place.", e);
                }
            }
            return false;
        }

        private void setPlaceText() {
            String text = null;
            if (selectedPlace != null) {
                text = selectedPlace.getName();
            }
            if (text == null) {
                text = getResources().getString(R.string.action_location_default);
            }
            setText2(text);
        }

    }

    private class PhotoListElement extends BaseListElement {
        private static final int CAMERA = 0;
        private static final int GALLERY = 1;
        private static final String PHOTO_URI_KEY = "photo_uri";
        private static final String TEMP_URI_KEY = "temp_uri";
        private static final String FILE_PREFIX = "scrumptious_img_";
        private static final String FILE_SUFFIX = ".jpg";

        private Uri tempUri = null;

        public PhotoListElement(int requestCode) {
            super(getActivity().getResources().getDrawable(R.drawable.add_photo),
                    getActivity().getResources().getString(R.string.action_photo),
                    null,
                    requestCode);
            photoUri = null;
        }

        @Override
        protected View.OnClickListener getOnClickListener() {
            return new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    showPhotoChoice();
                }
            };
        }

        @Override
        protected void onActivityResult(Intent data) {
            if (tempUri != null) {
                photoUri = tempUri;
            } else if (data != null) {
                photoUri = data.getData();
            }
            setPhotoThumbnail();
            setPhotoText();
        }

        @Override
        protected void populateOGAction(OpenGraphAction action) {
        }

        @Override
        protected void onSaveInstanceState(Bundle bundle) {
            if (photoUri != null) {
                bundle.putParcelable(PHOTO_URI_KEY, photoUri);
            }
            if (tempUri != null) {
                bundle.putParcelable(TEMP_URI_KEY, tempUri);
            }
        }

        @Override
        protected boolean restoreState(Bundle savedState) {
            photoUri = savedState.getParcelable(PHOTO_URI_KEY);
            tempUri = savedState.getParcelable(TEMP_URI_KEY);
            setPhotoText();
            return true;
        }

        private void showPhotoChoice() {
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            CharSequence camera = getResources().getString(R.string.action_photo_camera);
            CharSequence gallery = getResources().getString(R.string.action_photo_gallery);
            builder.setCancelable(true).
                    setItems(new CharSequence[] {camera, gallery}, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            if (i == CAMERA) {
                                startCameraActivity();
                            } else if (i == GALLERY) {
                                startGalleryActivity();
                            }
                        }
                    });
            builder.show();
        }

        private void setPhotoText() {
            if (photoUri == null) {
                setText2(getResources().getString(R.string.action_photo_default));
            } else {
                setText2(getResources().getString(R.string.action_photo_ready));
            }
        }
        
        private void setPhotoThumbnail() {
            photoThumbnail.setImageURI(photoUri);
        }

        private void startCameraActivity() {
            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            tempUri = getTempUri();
            if (tempUri != null) {
                intent.putExtra(MediaStore.EXTRA_OUTPUT, tempUri);
            }
            startActivityForResult(intent, getRequestCode());
        }

        private void startGalleryActivity() {
            tempUri = null;
            Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            intent.setType("image/*");
            String selectPicture = getResources().getString(R.string.select_picture);
            startActivityForResult(Intent.createChooser(intent, selectPicture), getRequestCode());
        }

        private Uri getTempUri() {
            String imgFileName = FILE_PREFIX + System.currentTimeMillis() + FILE_SUFFIX;

            // Note: on an emulator, you might need to create the "Pictures" directory in /mnt/sdcard first
            //       % adb shell
            //       % mkdir /mnt/sdcard/Pictures
            File image = new File(
                    Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES), imgFileName);
            return Uri.fromFile(image);
        }
    }

    private class ActionListAdapter extends ArrayAdapter<BaseListElement> {
        private List<BaseListElement> listElements;

        public ActionListAdapter(Context context, int resourceId, List<BaseListElement> listElements) {
            super(context, resourceId, listElements);
            this.listElements = listElements;
            for (int i = 0; i < listElements.size(); i++) {
                listElements.get(i).setAdapter(this);
            }
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View view = convertView;
            if (view == null) {
                LayoutInflater inflater =
                        (LayoutInflater) getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                view = inflater.inflate(R.layout.listitem, null);
            }

            BaseListElement listElement = listElements.get(position);
            if (listElement != null) {
                view.setOnClickListener(listElement.getOnClickListener());
                ImageView icon = (ImageView) view.findViewById(R.id.icon);
                TextView text1 = (TextView) view.findViewById(R.id.text1);
                TextView text2 = (TextView) view.findViewById(R.id.text2);
                if (icon != null) {
                    icon.setImageDrawable(listElement.getIcon());
                }
                if (text1 != null) {
                    text1.setText(listElement.getText1());
                }
                if (text2 != null) {
                    if (listElement.getText2() != null) {
                        text2.setVisibility(View.VISIBLE);
                        text2.setText(listElement.getText2());
                    } else {
                        text2.setVisibility(View.GONE);
                    }
                }
            }
            return view;
        }

    }
}
