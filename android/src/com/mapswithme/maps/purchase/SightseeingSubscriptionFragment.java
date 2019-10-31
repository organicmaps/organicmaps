package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;

public class SightseeingSubscriptionFragment extends BaseMwmFragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_sightseeing_subscription, container, false);
    return root;
  }
}
