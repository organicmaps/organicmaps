package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.text.Html;
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.DisabledBrowserMovementMethod;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.BaseMwmAuthorizationFragment;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.util.ConnectionState;

public class UgcSharingOptionsFragment extends BaseMwmAuthorizationFragment
{
  private static final String NO_NETWORK_CONNECTION_DIALOG_TAG = "no_network_connection_dialog";

  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root);
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_routes_sharing_options, container, false);
    initClickListeners(root);
    TextView licenceAgreementText = root.findViewById(R.id.license_agreement_message);

    String src = getResources().getString(R.string.ugc_routes_user_agreement,
                                          /* FIXME */
                                          Framework.nativeGetPrivacyPolicyLink());
    Spanned spanned = Html.fromHtml(src);
    licenceAgreementText.setMovementMethod(DisabledBrowserMovementMethod.getInstance());
    licenceAgreementText.setText(spanned);
    return root;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.sharing_options);
  }

  private void initClickListeners(@NonNull View root)
  {
    View getDirectLinkView = root.findViewById(R.id.get_direct_link_text);
    getDirectLinkView.setOnClickListener(directLinkListener -> onGetDirectLinkClicked());
    View uploadAndPublishView = root.findViewById(R.id.upload_and_publish_text);
    uploadAndPublishView.setOnClickListener(uploadListener -> onUploadAndPublishBtnClicked());
  }

  private void onUploadAndPublishBtnClicked()
  {
    if (isNetworkConnectionAbsent())
    {
      showNoNetworkConnectionDialog();
      return;
    }

    if (hasAuthToken())
      openTagsScreen();
    else
      requestAuth();
  }

  private void showNoNetworkConnectionDialog()
  {
    Fragment fragment = getFragmentManager().findFragmentByTag(NO_NETWORK_CONNECTION_DIALOG_TAG);
    if (fragment != null)
      return;

    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.common_check_internet_connection_dialog_title)
        .setMessageId(R.string.common_check_internet_connection_dialog)
        .setPositiveBtnId(R.string.ok)
        .build();
    dialog.show(dialog, NO_NETWORK_CONNECTION_DIALOG_TAG);
  }

  private boolean isNetworkConnectionAbsent()
  {
    return !ConnectionState.isConnected();
  }

  private void requestAuth()
  {
    getAuthorizer().authorize();
  }

  private void openTagsScreen()
  {

  }

  private boolean hasAuthToken()
  {
    return getAuthorizer().isAuthorized();
  }

  private void onGetDirectLinkClicked()
  {

  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    /*FIXME*/
    if (requestCode == 1 && resultCode == Activity.RESULT_OK)
      requestPublishing(data);
  }

  private void requestPublishing(@NonNull Intent data)
  {

  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {

  }

  @Override
  public void onAuthorizationStart()
  {

  }

  @Override
  public void onSocialAuthenticationCancel(int type)
  {

  }

  @Override
  public void onSocialAuthenticationError(int type, @Nullable String error)
  {

  }

  private class ToolbarController extends com.mapswithme.maps.widget.ToolbarController
  {
    public ToolbarController(View root)
    {
      super(root, UgcSharingOptionsFragment.this.getActivity());
    }

    @Override
    public void onUpClick()
    {
      getActivity().finish();
    }
  }
}
