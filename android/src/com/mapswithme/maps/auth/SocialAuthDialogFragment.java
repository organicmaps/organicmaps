package com.mapswithme.maps.auth;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.CheckBox;

import com.facebook.CallbackManager;
import com.facebook.FacebookCallback;
import com.facebook.FacebookException;
import com.facebook.login.LoginManager;
import com.facebook.login.LoginResult;
import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.lang.ref.WeakReference;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

public class SocialAuthDialogFragment extends BaseMwmDialogFragment
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = SocialAuthDialogFragment.class.getSimpleName();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private GoogleSignInClient mGoogleSignInClient;
  @NonNull
  private final CallbackManager mFacebookCallbackManager = CallbackManager.Factory.create();
  @NonNull
  private final List<TokenHandler> mTokenHandlers = Arrays.asList(
      new FacebookTokenHandler(), new GoogleTokenHandler(), new PhoneTokenHandler());
  @Nullable
  private TokenHandler mCurrentTokenHandler;
  @NonNull
  private final View.OnClickListener mPhoneClickListener = (View v) ->
  {
    PhoneAuthActivity.startForResult(this);
    trackStatsIfArgsExist(Statistics.EventName.AUTH_START);
  };
  @NonNull
  private final View.OnClickListener mGoogleClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      Intent intent = mGoogleSignInClient.getSignInIntent();
      startActivityForResult(intent, Constants.REQ_CODE_GOOGLE_SIGN_IN);
      trackStatsIfArgsExist(Statistics.EventName.AUTH_START);
    }
  };
  @NonNull
  private final View.OnClickListener mFacebookClickListener = v -> {
    LoginManager lm = LoginManager.getInstance();
    lm.logInWithReadPermissions(SocialAuthDialogFragment.this,
                                  Constants.FACEBOOK_PERMISSIONS);
    lm.registerCallback(mFacebookCallbackManager, new FBCallback(SocialAuthDialogFragment.this));
    trackStatsIfArgsExist(Statistics.EventName.AUTH_START);
  };
  @SuppressWarnings("NullableProblems")
  @NonNull
  private CheckBox mPrivacyPolicyCheck;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private CheckBox mTermOfUseCheck;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private CheckBox mPromoCheck;
  @Nullable
  private TargetFragmentCallback mTargetCallback;

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
    setTargetCallback();
    GoogleSignInOptions gso = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
        .requestIdToken(PrivateVariables.googleWebClientId())
        .requestEmail()
        .build();
    mGoogleSignInClient = GoogleSignIn.getClient(requireActivity(), gso);
  }

  private void trackStatsIfArgsExist(@NonNull String action)
  {
    Bundle args = getArguments();
    if (args == null)
      return;

    Statistics.INSTANCE.trackAuthDialogAction(action,
                                              Objects.requireNonNull(args.getString(Statistics.EventParam.FROM)));
  }

  private void setTargetCallback()
  {
    try
    {
      mTargetCallback = (TargetFragmentCallback) getParentFragment();
    }
    catch (ClassCastException e)
    {
      throw new ClassCastException("Caller must implement TargetFragmentCallback interface!");
    }
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View view = inflater.inflate(R.layout.fragment_auth_passport_dialog, container, false);

    setLoginButton(view, R.id.google_button, mGoogleClickListener);
    setLoginButton(view, R.id.facebook_button, mFacebookClickListener);
    setLoginButton(view, R.id.phone_button, mPhoneClickListener);

    mPromoCheck = view.findViewById(R.id.newsCheck);
    mPrivacyPolicyCheck = view.findViewById(R.id.privacyPolicyCheck);
    mPrivacyPolicyCheck.setOnCheckedChangeListener((buttonView, isChecked) -> {
      setButtonAvailability(view, isChecked && mTermOfUseCheck.isChecked(),
                            R.id.google_button, R.id.facebook_button, R.id.phone_button);
    });

    mTermOfUseCheck = view.findViewById(R.id.termOfUseCheck);
    mTermOfUseCheck.setOnCheckedChangeListener((buttonView, isChecked) -> {
      setButtonAvailability(view, isChecked && mPrivacyPolicyCheck.isChecked(),
                            R.id.google_button, R.id.facebook_button, R.id.phone_button);
    });

    UiUtils.linkifyView(view, R.id.privacyPolicyLink, R.string.sign_agree_pp_gdpr,
                        Framework.nativeGetPrivacyPolicyLink());

    UiUtils.linkifyView(view, R.id.termOfUseLink, R.string.sign_agree_tof_gdpr,
                        Framework.nativeGetTermsOfUseLink());

    setButtonAvailability(view, false, R.id.google_button, R.id.facebook_button,
                          R.id.phone_button);

    trackStatsIfArgsExist(Statistics.EventName.AUTH_SHOWN);
    return view;
  }

  private static void setLoginButton(@NonNull View root, @IdRes int id,
                                     @NonNull View.OnClickListener clickListener)
  {
    View button = root.findViewById(id);
    button.setOnClickListener(clickListener);
  }

  private static void setButtonAvailability(@NonNull View root, boolean available, @IdRes int... ids)
  {
    for (int id : ids)
    {
      View button = root.findViewById(id);
      button.setEnabled(available);
    }
  }

  @Override
  public void onResume()
  {
    super.onResume();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.UGC_AUTH_SHOWN);
  }

  private void sendResult(int resultCode, @Nullable String socialToken,
                          @Framework.AuthTokenType int type, @Nullable String error,
                          boolean isCancel)
  {
    if (mTargetCallback == null || !mTargetCallback.isTargetAdded())
      return;

    Intent data = new Intent();
    data.putExtra(Constants.EXTRA_SOCIAL_TOKEN, socialToken);
    data.putExtra(Constants.EXTRA_TOKEN_TYPE, type);
    data.putExtra(Constants.EXTRA_AUTH_ERROR, error);
    data.putExtra(Constants.EXTRA_IS_CANCEL, isCancel);
    data.putExtra(Constants.EXTRA_PRIVACY_POLICY_ACCEPTED, mPrivacyPolicyCheck.isChecked());
    data.putExtra(Constants.EXTRA_TERMS_OF_USE_ACCEPTED, mTermOfUseCheck.isChecked());
    data.putExtra(Constants.EXTRA_PROMO_ACCEPTED, mPromoCheck.isChecked());
    mTargetCallback.onTargetFragmentResult(resultCode, data);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    mFacebookCallbackManager.onActivityResult(requestCode, resultCode, data);

    if (resultCode != Activity.RESULT_OK || data == null)
      return;

    for (TokenHandler handler : mTokenHandlers)
    {
      if (handler.checkToken(requestCode, data))
      {
        mCurrentTokenHandler = handler;
        break;
      }
    }

    if (mCurrentTokenHandler == null)
      return;

    dismissAllowingStateLoss();
  }

  @Override
  public void onDismiss(DialogInterface dialog)
  {
    int resultCode;
    String token;
    @Framework.AuthTokenType
    int type;
    if (mCurrentTokenHandler == null)
    {
      resultCode = Activity.RESULT_CANCELED;
      token = null;
      type = Framework.SOCIAL_TOKEN_INVALID;
    }
    else
    {
      resultCode = Activity.RESULT_OK;
      token = mCurrentTokenHandler.getToken();
      type = mCurrentTokenHandler.getType();
      if (TextUtils.isEmpty(token))
        throw new AssertionError("Token must be non-null while token handler is non-null!");
      if (type == Framework.SOCIAL_TOKEN_INVALID)
        throw new AssertionError("Token type must be non-invalid while token handler is non-null!");
    }

    sendResult(resultCode, token, type, null, true);
    super.onDismiss(dialog);
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
      sendEmptyResult(Activity.RESULT_CANCELED, Framework.SOCIAL_TOKEN_FACEBOOK,
                      null, true);
    }

    @Override
    public void onError(FacebookException error)
    {
      LOGGER.e(TAG, "onError", error);
      sendEmptyResult(Activity.RESULT_CANCELED, Framework.SOCIAL_TOKEN_FACEBOOK,
                 error != null ? error.getMessage() : null, false);
    }

    private void sendEmptyResult(int resultCode, @Framework.AuthTokenType int type,
                                 @Nullable String error, boolean isCancel)
    {
      SocialAuthDialogFragment fragment = mFragmentRef.get();
      if (fragment == null)
        return;

      fragment.sendResult(resultCode, null, type, error, isCancel);
    }
  }
}
