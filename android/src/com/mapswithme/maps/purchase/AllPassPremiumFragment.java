package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.R;
import com.mapswithme.maps.databinding.FragmentAllPassPremiumBinding;

import java.util.Objects;

public class AllPassPremiumFragment extends Fragment
{
  private static final String BUNDLE_INDEX = "index";

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    int index = Objects.requireNonNull(getArguments()).getInt(BUNDLE_INDEX);
    AllPassSubscriptionType type = AllPassSubscriptionType.values()[index];
    FragmentAllPassPremiumBinding binding = makeBinding(inflater, container);
    binding.setModel(type);
    return binding.getRoot();
  }

  @NonNull
  private static FragmentAllPassPremiumBinding makeBinding(@NonNull LayoutInflater inflater,
                                                           @Nullable ViewGroup container)
  {
    return DataBindingUtil.inflate(inflater, R.layout.fragment_all_pass_premium, container, false);
  }

  @NonNull
  public static AllPassPremiumFragment newInstance(int index)
  {
    AllPassPremiumFragment fragment = new AllPassPremiumFragment();
    Bundle args = new Bundle();
    args.putInt(BUNDLE_INDEX, index);
    fragment.setArguments(args);
    return fragment;
  }
}
