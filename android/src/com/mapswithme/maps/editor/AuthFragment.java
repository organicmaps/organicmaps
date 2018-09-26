package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.statistics.Statistics;

public class AuthFragment extends BaseMwmToolbarFragment
{
  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_auth_editor, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle(R.string.thank_you);
    OsmAuthFragmentDelegate osmAuthDelegate = new OsmAuthFragmentDelegate(this)
    {
      @Override
      protected void loginOsm()
      {
        ((BaseMwmFragmentActivity) getActivity()).replaceFragment(OsmAuthFragment.class, null, null);
      }
    };
    osmAuthDelegate.onViewCreated(view, savedInstanceState);
  }

  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root, getActivity())
    {
      @Override
      public void onUpClick()
      {
        Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_AUTH_DECLINED);
        super.onUpClick();
      }
    };
  }
}
