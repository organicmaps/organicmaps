package com.mapswithme.maps.editor;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;

public class AuthDialogFragment extends BaseMwmDialogFragment
{
  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_auth_editor_dialog, container, false);
  }

  @Override
  protected int getStyle()
  {
    return STYLE_NO_TITLE;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    OsmAuthFragmentDelegate osmAuthDelegate = new OsmAuthFragmentDelegate(this)
    {
      @Override
      protected void loginOsm()
      {
        startActivity(new Intent(getContext(), OsmAuthActivity.class));
        dismiss();
      }
    };
    osmAuthDelegate.onViewCreated(view, savedInstanceState);
  }
}
