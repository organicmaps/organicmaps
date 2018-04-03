package com.mapswithme.maps.auth;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.base.BaseMwmDialogFragment;

public class PassportAuthDialogFragment extends BaseMwmDialogFragment
{
  private Authorizer mAuthorizer = new Authorizer(this);

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mAuthorizer.authorize();
    return null;
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    mAuthorizer.onActivityResult(requestCode, resultCode, data);
    dismiss();
  }

  @Override
  protected int getStyle()
  {
    return STYLE_NO_TITLE;
  }
}
