package com.mapswithme.maps.settings;

import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.Nullable;

import com.mapswithme.maps.R;
import com.mapswithme.maps.WebContainerDelegate;
import com.mapswithme.util.Constants;

public class HelpFragment extends BaseSettingsFragment
{
  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_prefs_help;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View root = super.onCreateView(inflater, container, savedInstanceState);

    new WebContainerDelegate(root, Constants.Url.FAQ)
    {
      @Override
      protected void doStartActivity(Intent intent)
      {
        startActivity(intent);
      }
    };

    TextView feedback = root.findViewById(R.id.feedback);
    feedback.setOnClickListener(v -> FeedbackDialog.show(requireActivity()));
    return root;
  }
}
