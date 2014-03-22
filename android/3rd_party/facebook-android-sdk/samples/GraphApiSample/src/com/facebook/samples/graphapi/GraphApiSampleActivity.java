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

package com.facebook.samples.graphapi;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import com.facebook.*;
import com.facebook.Session.StatusCallback;
import com.facebook.model.GraphObject;

import java.util.ArrayList;
import java.util.List;

public class GraphApiSampleActivity extends Activity {
    static final String APP_ID = "307234779396415";
    static final String PENDING_REQUEST_BUNDLE_KEY = "com.facebook.samples.graphapi:PendingRequest";

    Button buttonRequest;
    EditText editRequests;
    TextView textViewResults;
    Session session;
    boolean pendingRequest;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        this.buttonRequest = (Button) findViewById(R.id.buttonRequest);
        this.buttonRequest.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                onClickRequest();
            }
        });
        this.editRequests = (EditText) findViewById(R.id.editRequests);
        this.textViewResults = (TextView) findViewById(R.id.textViewResults);

        this.session = createSession();
        Settings.addLoggingBehavior(LoggingBehavior.INCLUDE_ACCESS_TOKENS);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (this.session.onActivityResult(this, requestCode, resultCode, data) &&
                pendingRequest &&
                this.session.getState().isOpened()) {
            sendRequests();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Call the 'activateApp' method to log an app event for use in analytics and advertising reporting.  Do so in
        // the onResume methods of the primary Activities that an app may be launched into.
        AppEventsLogger.activateApp(this, APP_ID);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);

        pendingRequest = savedInstanceState.getBoolean(PENDING_REQUEST_BUNDLE_KEY, pendingRequest);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean(PENDING_REQUEST_BUNDLE_KEY, pendingRequest);
    }

    private void onClickRequest() {
        if (this.session.isOpened()) {
            sendRequests();
        } else {
            StatusCallback callback = new StatusCallback() {
                public void call(Session session, SessionState state, Exception exception) {
                    if (exception != null) {
                        new AlertDialog.Builder(GraphApiSampleActivity.this)
                                .setTitle(R.string.login_failed_dialog_title)
                                .setMessage(exception.getMessage())
                                .setPositiveButton(R.string.ok_button, null)
                                .show();
                        GraphApiSampleActivity.this.session = createSession();
                    }
                }
            };
            pendingRequest = true;
            this.session.openForRead(new Session.OpenRequest(this).setCallback(callback));
        }
    }

    private void sendRequests() {
        textViewResults.setText("");

        String requestIdsText = editRequests.getText().toString();
        String[] requestIds = requestIdsText.split(",");

        List<Request> requests = new ArrayList<Request>();
        for (final String requestId : requestIds) {
            requests.add(new Request(session, requestId, null, null, new Request.Callback() {
                public void onCompleted(Response response) {
                    GraphObject graphObject = response.getGraphObject();
                    FacebookRequestError error = response.getError();
                    String s = textViewResults.getText().toString();
                    if (graphObject != null) {
                        if (graphObject.getProperty("id") != null) {
                            s = s + String.format("%s: %s\n", graphObject.getProperty("id"), graphObject.getProperty(
                                    "name"));
                        } else {
                            s = s + String.format("%s: <no such id>\n", requestId);
                        }
                    } else if (error != null) {
                        s = s + String.format("Error: %s", error.getErrorMessage());
                    }
                    textViewResults.setText(s);
                }
            }));
        }
        pendingRequest = false;
        Request.executeBatchAsync(requests);
    }

    private Session createSession() {
        Session activeSession = Session.getActiveSession();
        if (activeSession == null || activeSession.getState().isClosed()) {
            activeSession = new Session.Builder(this).setApplicationId(APP_ID).build();
            Session.setActiveSession(activeSession);
        }
        return activeSession;
    }
}
