package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.base.OnBackPressListener;

public class EditorHostFragment extends BaseMwmToolbarFragment
                             implements OnBackPressListener
{
  public static final String EXTRA_MAP_OBJECT = "MapObject";

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_editor_host, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    final Fragment editorFragment = Fragment.instantiate(getActivity(), EditorFragment.class.getName(), getArguments());
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, editorFragment)
                             .commit();

    mToolbarController.setTitle("Edit POI");
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }
}
