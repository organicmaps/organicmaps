/**
 * Copyright 2010-present Facebook
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

package com.facebook.android;

import android.content.Context;
import android.os.Bundle;
import com.facebook.FacebookDialogException;
import com.facebook.FacebookException;
import com.facebook.FacebookOperationCanceledException;
import com.facebook.android.Facebook.DialogListener;
import com.facebook.widget.WebDialog;

/**
 * This class is deprecated. See {@link com.facebook.widget.WebDialog}.
 */
@Deprecated
public class FbDialog extends WebDialog {
    private DialogListener mListener;

    public FbDialog(Context context, String url, DialogListener listener) {
        this(context, url, listener, DEFAULT_THEME);
    }

    public FbDialog(Context context, String url, DialogListener listener, int theme) {
        super(context, url, theme);
        setDialogListener(listener);
    }

    public FbDialog(Context context, String action, Bundle parameters, DialogListener listener) {
        super(context, action, parameters, DEFAULT_THEME, null);
        setDialogListener(listener);
    }

    public FbDialog(Context context, String action, Bundle parameters, DialogListener listener,
            int theme) {
        super(context, action, parameters, theme, null);
        setDialogListener(listener);
    }

    private void setDialogListener(DialogListener listener) {
        this.mListener = listener;
        setOnCompleteListener(new OnCompleteListener() {
            @Override
            public void onComplete(Bundle values, FacebookException error) {
                callDialogListener(values, error);
            }
        });
    }

    private void callDialogListener(Bundle values, FacebookException error) {
        if (mListener == null) {
            return;
        }

        if (values != null) {
            mListener.onComplete(values);
        } else {
            if (error instanceof FacebookDialogException) {
                FacebookDialogException facebookDialogException = (FacebookDialogException) error;
                DialogError dialogError = new DialogError(facebookDialogException.getMessage(),
                        facebookDialogException.getErrorCode(), facebookDialogException.getFailingUrl());
                mListener.onError(dialogError);
            } else if (error instanceof FacebookOperationCanceledException) {
                mListener.onCancel();
            } else {
                FacebookError facebookError = new FacebookError(error.getMessage());
                mListener.onFacebookError(facebookError);
            }
        }
    }
}
