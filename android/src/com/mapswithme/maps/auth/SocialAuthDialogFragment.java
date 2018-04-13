package com.mapswithme.maps.auth;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;

import com.facebook.AccessToken;
import com.facebook.CallbackManager;
import com.facebook.FacebookCallback;
import com.facebook.FacebookException;
import com.facebook.login.LoginResult;
import com.facebook.login.widget.LoginButton;
import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.common.SignInButton;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.lang.ref.WeakReference;

public class SocialAuthDialogFragment extends BaseMwmDialogFragment
{

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = SocialAuthDialogFragment.class.getSimpleName();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private GoogleSignInClient mGoogleSignInClient;
  @NonNull
  private final CallbackManager mFacebookCallbackManager = CallbackManager.Factory.create();
  @Nullable
  private String mPhoneAuthToken;

  @NonNull
  private final View.OnClickListener mPhoneClickListener = (View v) ->
  {
    PhoneAuthActivity.startForResult(this);
  };

  @NonNull
  private final View.OnClickListener mGoogleClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      Intent intent = mGoogleSignInClient.getSignInIntent();
      startActivity(intent);
    }
  };

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);
    return res;
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    GoogleSignInOptions gso = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
        .requestIdToken(PrivateVariables.googleWebClientId())
        .requestEmail()
        .build();
    mGoogleSignInClient = GoogleSignIn.getClient(getActivity(), gso);
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View view = inflater.inflate(R.layout.fragment_auth_passport_dialog, container, false);

    SignInButton googleButton = view.findViewById(R.id.google_button);
    googleButton.setOnClickListener(mGoogleClickListener);

    LoginButton facebookButton = view.findViewById(R.id.facebook_button);
    facebookButton.setReadPermissions(Constants.FACEBOOK_PERMISSIONS);
    facebookButton.setFragment(this);
    facebookButton.registerCallback(mFacebookCallbackManager, new FBCallback(this));

    View phoneButton = view.findViewById(R.id.phone_button);
    phoneButton.setOnClickListener(mPhoneClickListener);
    return view;
  }

  @Override
  public void onResume()
  {
    super.onResume();

    AccessToken facebookToken = AccessToken.getCurrentAccessToken();
    String fbTokenValue = null;
    if (facebookToken != null)
      fbTokenValue = facebookToken.getToken();

    GoogleSignInAccount account = GoogleSignIn.getLastSignedInAccount(MwmApplication.get());
    String googleTokenValue = null;
    if (account != null)
      googleTokenValue = account.getIdToken();

    if (TextUtils.isEmpty(fbTokenValue) && TextUtils.isEmpty(googleTokenValue))
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_AUTH_SHOWN);
      return;
    }

    LOGGER.i(TAG, "Social token is already obtained");
    dismiss();
  }

  private void sendResult(int resultCode, @Nullable String socialToken,
                          @Framework.AuthTokenType int type, @Nullable String error,
                          boolean isCancel)
  {
    Fragment caller = getTargetFragment();
    if (caller == null || !caller.isAdded())
      return;

    Intent data = new Intent();
    data.putExtra(Constants.EXTRA_SOCIAL_TOKEN, socialToken);
    data.putExtra(Constants.EXTRA_TOKEN_TYPE, type);
    data.putExtra(Constants.EXTRA_AUTH_ERROR, error);
    data.putExtra(Constants.EXTRA_IS_CANCEL, isCancel);
    caller.onActivityResult(Constants.REQ_CODE_GET_SOCIAL_TOKEN, resultCode, data);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    if (data != null && requestCode == Constants.REQ_CODE_PHONE_AUTH_RESULT)
    {
      mPhoneAuthToken = data.getStringExtra(Constants.EXTRA_PHONE_AUTH_TOKEN);
      dismiss();
    }

    mFacebookCallbackManager.onActivityResult(requestCode, resultCode, data);
  }

  @Override
  public void onDismiss(DialogInterface dialog)
  {
    String token = mPhoneAuthToken;

    if (TextUtils.isEmpty(token))
    {
      GoogleSignInAccount googleAccount = GoogleSignIn.getLastSignedInAccount(MwmApplication.get());
      token = googleAccount != null ? googleAccount.getIdToken() : null;
    }

    if (TextUtils.isEmpty(token))
    {
      AccessToken facebookToken = AccessToken.getCurrentAccessToken();
      token = facebookToken != null ? facebookToken.getToken() : null;
    }

    int resultCode = TextUtils.isEmpty(token) ? Activity.RESULT_CANCELED : Activity.RESULT_OK;
    sendResult(resultCode, token, getAuthTokenType(), null, true);
    super.onDismiss(dialog);
  }

  @Framework.AuthTokenType
  private int getAuthTokenType()
  {
    if (!TextUtils.isEmpty(mPhoneAuthToken))
      return Framework.SOCIAL_TOKEN_PHONE;

    GoogleSignInAccount googleAccount = GoogleSignIn.getLastSignedInAccount(MwmApplication.get());
    if (googleAccount != null && !TextUtils.isEmpty(googleAccount.getIdToken()))
      return Framework.SOCIAL_TOKEN_GOOGLE;

    return Framework.SOCIAL_TOKEN_FACEBOOK;
  }

  private static class FBCallback implements FacebookCallback<LoginResult>
  {
    @NonNull
    private final WeakReference<SocialAuthDialogFragment> mFragmentRef;

    private FBCallback(@NonNull SocialAuthDialogFragment fragment)
    {
      mFragmentRef = new WeakReference<>(fragment);
    }

    @Override
    public void onSuccess(LoginResult loginResult)
    {
      Statistics.INSTANCE.trackUGCExternalAuthSucceed(Statistics.ParamValue.FACEBOOK);
      LOGGER.d(TAG, "onSuccess");
    }

    @Override
    public void onCancel()
    {
      LOGGER.w(TAG, "onCancel");
      sendErrorResult(Activity.RESULT_CANCELED, Framework.SOCIAL_TOKEN_FACEBOOK,
                      null, true);
    }

    @Override
    public void onError(FacebookException error)
    {
      LOGGER.e(TAG, "onError", error);
      sendErrorResult(Activity.RESULT_CANCELED, Framework.SOCIAL_TOKEN_FACEBOOK,
                 error != null ? error.getMessage() : null, false);
    }

    private void sendErrorResult(int resultCode, @Framework.AuthTokenType int type,
                                 @Nullable String error, boolean isCancel)
    {
      SocialAuthDialogFragment fragment = mFragmentRef.get();
      if (fragment == null)
        return;

      fragment.sendResult(resultCode, null, type, error, isCancel);
    }
  }
}
