package com.mapswithme.maps.gdpr;

import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Statistics;

public class OptOutFragment extends Fragment
{
  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    Statistics.INSTANCE.trackSettingsDetails();
    return inflater.inflate(R.layout.fragment_gdpr, container, false);
  }
}
