package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;

public class StreetFragment extends BaseMwmRecyclerFragment
{
  public static final String EXTRA_CURRENT_STREET = "Street";

  private String mSelectedString;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    mSelectedString = getArguments().getString(EXTRA_CURRENT_STREET, "");
    return super.onCreateView(inflater, container, savedInstanceState);
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new StreetAdapter(Editor.nativeGetNearbyStreets(), mSelectedString);
  }

  @NonNull
  public String getStreet()
  {
    return ((StreetAdapter) getAdapter()).getSelectedStreet();
  }
}
