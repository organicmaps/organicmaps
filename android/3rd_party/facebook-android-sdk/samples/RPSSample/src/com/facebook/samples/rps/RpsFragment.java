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
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.*;
import com.facebook.*;
import com.facebook.model.GraphObject;
import static com.facebook.samples.rps.OpenGraphUtils.*;
import static com.facebook.samples.rps.RpsGameUtils.*;

import com.facebook.model.OpenGraphAction;
import com.facebook.model.OpenGraphObject;
import com.facebook.widget.FacebookDialog;

import java.util.Arrays;
import java.util.Random;

public class RpsFragment extends Fragment {

    private static final String SHARE_GAME_LINK = "https://developers.facebook.com/android";
    private static final String SHARE_GAME_NAME = "Rock, Papers, Scissors Sample Application";
    private static final String DEFAULT_GAME_OBJECT_TITLE = "an awesome game of Rock, Paper, Scissors";
    private static final String WIN_KEY = "wins";
    private static final String LOSS_KEY = "losses";
    private static final String TIE_KEY = "ties";
    private static final String PLAYER_CHOICE_KEY = "player_choice";
    private static final String COMPUTER_CHOICE_KEY = "computer_choice";
    private static final String STATE_KEY = "state";
    private static final String RESULT_KEY = "result";
    private static final String PENDING_PUBLISH_KEY = "pending_publish";
    private static final String IMPLICIT_PUBLISH_KEY = "implicitly_publish";
    private static final String ADDITIONAL_PERMISSIONS = "publish_actions";
    private static final String PHOTO_REQUEST_NAME = "photorequest";
    private static final String PHOTO_REQUEST_RESULT = "{result=photorequest:$.uri}";
    private static final String GAME_REQUEST_NAME = "gamerequest";
    private static final String GAME_REQUEST_RESULT = "{result=gamerequest:$.id}";
    private static final int INITIAL_DELAY_MILLIS = 500;
    private static final int DEFAULT_DELAY_MILLIS = 1000;
    private static final String TAG = RpsFragment.class.getName();

    private static String[] PHOTO_URIS = { null, null, null };

    private TextView [] gestureTextViews = new TextView[3];
    private TextView shootTextView;
    private ImageView playerChoiceView;
    private ImageView computerChoiceView;
    private TextView resultTextView;
    private ViewGroup shootGroup;
    private ViewGroup resultGroup;
    private ViewGroup playerChoiceGroup;
    private Button againButton;
    private ImageButton [] gestureImages = new ImageButton[3];
    private ImageButton fbButton;
    private TextView statsTextView;
    private ViewFlipper rpsFlipper;

    private int wins = 0;
    private int losses = 0;
    private int ties = 0;
    private int playerChoice = INVALID_CHOICE;
    private int computerChoice = INVALID_CHOICE;
    private RpsState currentState = RpsState.INIT;
    private RpsResult result = RpsResult.INVALID;
    private InitHandler handler = new InitHandler();
    private Random random = new Random(System.currentTimeMillis());
    private boolean pendingPublish;
    private boolean shouldImplicitlyPublish = true;

    private Session.StatusCallback newPermissionsCallback = new Session.StatusCallback() {
        @Override
        public void call(Session session, SessionState state, Exception exception) {
            if (exception != null ||
                    !session.isOpened() ||
                    !session.getPermissions().contains(ADDITIONAL_PERMISSIONS)) {
                // this means the user did not grant us write permissions, so
                // we don't do implicit publishes
                shouldImplicitlyPublish = false;
                pendingPublish = false;
            } else {
                publishResult();
            }
        }
    };

    private DialogInterface.OnClickListener canPublishClickListener = new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialogInterface, int i) {
            final Session session = Session.getActiveSession();
            if (session != null && session.isOpened()) {
                // if they choose to publish, then we request for publish permissions
                shouldImplicitlyPublish = true;
                pendingPublish = true;
                Session.NewPermissionsRequest newPermissionsRequest =
                        new Session.NewPermissionsRequest(RpsFragment.this, ADDITIONAL_PERMISSIONS)
                                .setDefaultAudience(SessionDefaultAudience.FRIENDS)
                                .setCallback(newPermissionsCallback);
                session.requestNewPublishPermissions(newPermissionsRequest);
            }
        }
    };

    private DialogInterface.OnClickListener dontPublishClickListener = new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialogInterface, int i) {
            // if they choose not to publish, then we save that choice, and don't prompt them
            // until they restart the app
            pendingPublish = false;
            shouldImplicitlyPublish = false;
        }
    };

    private class InitHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if (!isResumed()) {
                // if we're not in between onResume and onPause, don't do animation transitions
                return;
            }
            switch (msg.what) {
                case ROCK:
                    showViews(gestureTextViews[ROCK], gestureImages[ROCK]);
                    sendNextMessage(PAPER);
                    break;
                case PAPER:
                    showViews(gestureTextViews[PAPER], gestureImages[PAPER]);
                    sendNextMessage(SCISSORS);
                    break;
                case SCISSORS:
                    showViews(gestureTextViews[SCISSORS], gestureImages[SCISSORS]);
                    sendNextMessage(SHOOT);
                    break;
                case SHOOT:
                    showViews(shootTextView);
                    switchState(RpsState.PLAYING, false);
                    break;
                default:
                    Log.e(TAG, "Unexpected message received: " + msg.toString());
                    break;
            }
        }

        private void sendNextMessage(int what) {
            Message newMsg = new Message();
            newMsg.what = what;
            sendMessageDelayed(newMsg, DEFAULT_DELAY_MILLIS);
        }
    }

    private void switchState(RpsState newState, boolean isOnResume) {
        if (!isResumed()) {
            // if we're not in between onResume and onPause, don't transition states
            return;
        }
        switch (newState) {
            case INIT:
                playerChoice = INVALID_CHOICE;
                computerChoice = INVALID_CHOICE;
                result = RpsResult.INVALID;
                showViews(shootGroup, playerChoiceGroup, rpsFlipper);
                rpsFlipper.startFlipping();
                hideViews(gestureImages);
                hideViews(gestureTextViews);
                hideViews(resultGroup, shootTextView, againButton);
                enableViews(false, gestureImages);
                enableViews(false, againButton);
                Message initMessage = new Message();
                initMessage.what = ROCK;
                handler.sendMessageDelayed(initMessage, INITIAL_DELAY_MILLIS);
                break;
            case PLAYING:
                enableViews(true, gestureImages);
                showViews(rpsFlipper);
                rpsFlipper.startFlipping();
                break;
            case RESULT:
                hideViews(shootGroup, playerChoiceGroup);
                playerChoiceView.setImageResource(DRAWABLES_HUMAN[playerChoice]);
                computerChoiceView.setImageResource(DRAWABLES_COMPUTER[computerChoice]);
                resultTextView.setText(result.getStringId());
                showViews(resultGroup, againButton);
                enableViews(true, againButton);
                if (!isOnResume) {
                    // don't publish if we're switching states because onResumed is being called
                    publishResult();
                }
                break;
            default:
                Log.e(TAG, "Unexpected state reached: " + newState.toString());
                break;
        }

        String statsFormat = getResources().getString(R.string.stats_format);
        statsTextView.setText(String.format(statsFormat, wins, losses, ties));

        currentState = newState;
    }

    private void hideViews(View ... views) {
        for (View view : views) {
            view.setVisibility(View.INVISIBLE);
        }
    }

    private void showViews(View ... views) {
        for (View view : views) {
            view.setVisibility(View.VISIBLE);
        }
    }

    private void enableViews(boolean enabled, View ... views) {
        for (View view : views) {
            view.setEnabled(enabled);
        }
    }

    private void playerPlayed(int choice) {
        playerChoice = choice;
        computerChoice = getComputerChoice();
        result = RESULTS[playerChoice][computerChoice];
        switch (result) {
            case WIN:
                wins++;
                break;
            case LOSS:
                losses++;
                break;
            case TIE:
                ties++;
                break;
            default:
                Log.e(TAG, "Unexpected result: " + result.toString());
                break;
        }
        switchState(RpsState.RESULT, false);
    }

    private int getComputerChoice() {
        return random.nextInt(3);
    }

    private boolean canPublish() {
        final Session session = Session.getActiveSession();
        if (session != null && session.isOpened()) {
            if (session.getPermissions().contains(ADDITIONAL_PERMISSIONS)) {
                // if we already have publish permissions, then go ahead and publish
                return true;
            } else {
                // otherwise we ask the user if they'd like to publish to facebook
                new AlertDialog.Builder(getActivity())
                        .setTitle(R.string.share_with_friends_title)
                        .setMessage(R.string.share_with_friends_message)
                        .setPositiveButton(R.string.share_with_friends_yes, canPublishClickListener)
                        .setNegativeButton(R.string.share_with_friends_no, dontPublishClickListener)
                        .show();
                return false;
            }
        }
        return false;
    }

    private Request publishPlayerPhoto(final int choice) {
        Bitmap bitmap = BitmapFactory.decodeResource(getResources(), DRAWABLES_HUMAN[choice]);
        Request request = Request.newUploadStagingResourceWithImageRequest(
                Session.getActiveSession(),
                bitmap,
                new Request.Callback() {
                    @Override
                    public void onCompleted(Response response) {
                        if (response.getError() != null) {
                            Log.e(TAG, "photo staging upload failed: " + response.getError());
                        } else {
                            PHOTO_URIS[choice] = response.getGraphObject().getProperty("uri").toString();
                        }
                    }
                });
        request.setBatchEntryName(PHOTO_REQUEST_NAME);
        request.setBatchEntryOmitResultOnSuccess(false);
        return request;
    }

    private void publishResult() {
        if (shouldImplicitlyPublish && canPublish()) {
            RequestBatch batch = new RequestBatch();
            String photoUri = PHOTO_URIS[playerChoice];
            if (photoUri == null) {
                batch.add(publishPlayerPhoto(playerChoice));
                photoUri = PHOTO_REQUEST_RESULT;
            }

            GameGraphObject gameObject = createGameObject();
            gameObject.setImageUrls(Arrays.asList(photoUri));

            Request gameRequest = Request.newPostOpenGraphObjectRequest(Session.getActiveSession(), gameObject,
                    new Request.Callback() {
                        @Override
                        public void onCompleted(Response response) {
                            if (response.getError() != null) {
                                Log.e(TAG, "game object creation failed: " + response.getError());
                            }
                        }
                    });
            gameRequest.setBatchEntryName(GAME_REQUEST_NAME);

            batch.add(gameRequest);

            PlayAction playAction = createPlayActionWithGame(GAME_REQUEST_RESULT);
            Request playRequest = Request.newPostOpenGraphActionRequest(Session.getActiveSession(),
                    playAction,
                    new Request.Callback() {
                        @Override
                        public void onCompleted(Response response) {
                            if (response.getError() != null) {
                                Log.e(TAG, "Play action creation failed: " + response.getError());
                            } else {
                                PostResponse postResponse = response.getGraphObjectAs(PostResponse.class);
                                Log.i(TAG, "Posted OG Action with id: " + postResponse.getId());
                            }
                        }
                    });

            batch.add(playRequest);
            batch.executeAsync();
        }
    }

    private GameGraphObject createGameObject() {
        GameGraphObject gameGraphObject =
                OpenGraphObject.Factory.createForPost(GameGraphObject.class, GameGraphObject.TYPE);
        gameGraphObject.setTitle(DEFAULT_GAME_OBJECT_TITLE);
        GraphObject dataObject = GraphObject.Factory.create();
        dataObject.setProperty("player_gesture", CommonObjects.BUILT_IN_OPEN_GRAPH_OBJECTS[playerChoice]);
        dataObject.setProperty("opponent_gesture", CommonObjects.BUILT_IN_OPEN_GRAPH_OBJECTS[computerChoice]);
        dataObject.setProperty("result", getString(result.getResultStringId()));
        gameGraphObject.setData(dataObject);
        return gameGraphObject;
    }

    private PlayAction createPlayActionWithGame(String game) {
        PlayAction playAction = OpenGraphAction.Factory.createForPost(PlayAction.class, PlayAction.TYPE);
        playAction.setProperty("game", game);
        return playAction;
    }

    private GestureGraphObject getBuiltInGesture(int choice) {
        if (choice < 0 || choice >= CommonObjects.BUILT_IN_OPEN_GRAPH_OBJECTS.length) {
            throw new IllegalArgumentException("Invalid choice");
        }
        GestureGraphObject gesture =
                GraphObject.Factory.create(GestureGraphObject.class);
        gesture.setId(CommonObjects.BUILT_IN_OPEN_GRAPH_OBJECTS[choice]);
        return gesture;
    }

    public void shareUsingNativeDialog() {
        if (playerChoice == INVALID_CHOICE || computerChoice == INVALID_CHOICE) {
            FacebookDialog.ShareDialogBuilder builder = new FacebookDialog.ShareDialogBuilder(getActivity())
                    .setLink(SHARE_GAME_LINK)
                    .setName(SHARE_GAME_NAME)
                    .setFragment(this);
            // share the app
            if (builder.canPresent()) {
                builder.build().present();
            }
        } else {
            ThrowAction throwAction = OpenGraphAction.Factory.createForPost(ThrowAction.class, ThrowAction.TYPE);
            throwAction.setGesture(getBuiltInGesture(playerChoice));
            throwAction.setOpposingGesture(getBuiltInGesture(computerChoice));

            // The OG objects have their own bitmaps we could rely on, but in order to demonstrate attaching
            // an in-memory bitmap (e.g., a game screencap) we'll send the bitmap explicitly ourselves.
            ImageButton view = gestureImages[playerChoice];
            BitmapDrawable drawable = (BitmapDrawable) view.getBackground();
            Bitmap bitmap = drawable.getBitmap();

            FacebookDialog.OpenGraphActionDialogBuilder builder = new FacebookDialog.OpenGraphActionDialogBuilder(
                    getActivity(),
                    throwAction,
                    ThrowAction.PREVIEW_PROPERTY_NAME)
                    .setFragment(this)
                    .setImageAttachmentsForAction(Arrays.asList(bitmap));

            // share the game play
            if (builder.canPresent()) {
                builder.build().present();
            }
        }
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        super.onCreateView(inflater, container, savedInstanceState);
        View view = inflater.inflate(R.layout.rps_fragment, container, false);

        gestureTextViews[ROCK] = (TextView) view.findViewById(R.id.text_rock);
        gestureTextViews[PAPER] = (TextView) view.findViewById(R.id.text_paper);
        gestureTextViews[SCISSORS] = (TextView) view.findViewById(R.id.text_scissors);
        shootTextView = (TextView) view.findViewById(R.id.shoot);
        playerChoiceView = (ImageView) view.findViewById(R.id.player_choice);
        computerChoiceView = (ImageView) view.findViewById(R.id.computer_choice);
        resultTextView = (TextView) view.findViewById(R.id.who_won);
        shootGroup = (ViewGroup) view.findViewById(R.id.shoot_display_group);
        resultGroup = (ViewGroup) view.findViewById(R.id.result_display_group);
        playerChoiceGroup = (ViewGroup) view.findViewById(R.id.player_choice_display_group);
        againButton = (Button) view.findViewById(R.id.again_button);
        gestureImages[ROCK] = (ImageButton) view.findViewById(R.id.player_rock);
        gestureImages[PAPER] = (ImageButton) view.findViewById(R.id.player_paper);
        gestureImages[SCISSORS] = (ImageButton) view.findViewById(R.id.player_scissors);
        fbButton = (ImageButton) view.findViewById(R.id.facebook_button);
        statsTextView = (TextView) view.findViewById(R.id.stats);
        rpsFlipper = (ViewFlipper) view.findViewById(R.id.rps_flipper);

        gestureImages[ROCK].setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                playerPlayed(ROCK);
            }
        });

        gestureImages[PAPER].setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                playerPlayed(PAPER);
            }
        });

        gestureImages[SCISSORS].setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                playerPlayed(SCISSORS);
            }
        });

        againButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                switchState(RpsState.INIT, false);
            }
        });

        fbButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                getActivity().openOptionsMenu();
            }
        });

        return view;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState != null) {
            wins = savedInstanceState.getInt(WIN_KEY);
            losses = savedInstanceState.getInt(LOSS_KEY);
            ties = savedInstanceState.getInt(TIE_KEY);
            computerChoice = savedInstanceState.getInt(COMPUTER_CHOICE_KEY);
            playerChoice = savedInstanceState.getInt(PLAYER_CHOICE_KEY);
            currentState = (RpsState) savedInstanceState.getSerializable(STATE_KEY);
            result = (RpsResult) savedInstanceState.getSerializable(RESULT_KEY);
            pendingPublish = savedInstanceState.getBoolean(PENDING_PUBLISH_KEY);
            shouldImplicitlyPublish = savedInstanceState.getBoolean(IMPLICIT_PUBLISH_KEY);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (Session.getActiveSession() != null) {
            Session.getActiveSession().onActivityResult(getActivity(), requestCode, resultCode, data);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        switchState(currentState, true);
    }

    @Override
    public void onSaveInstanceState(Bundle bundle) {
        super.onSaveInstanceState(bundle);
        bundle.putInt(WIN_KEY, wins);
        bundle.putInt(LOSS_KEY, losses);
        bundle.putInt(TIE_KEY, ties);
        bundle.putInt(COMPUTER_CHOICE_KEY, computerChoice);
        bundle.putInt(PLAYER_CHOICE_KEY, playerChoice);
        bundle.putSerializable(STATE_KEY, currentState);
        bundle.putSerializable(RESULT_KEY, result);
        bundle.putBoolean(PENDING_PUBLISH_KEY, pendingPublish);
        bundle.putBoolean(IMPLICIT_PUBLISH_KEY, shouldImplicitlyPublish);
    }

}
