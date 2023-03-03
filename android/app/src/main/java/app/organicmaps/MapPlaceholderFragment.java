package app.organicmaps;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.display.DisplayType;

public class MapPlaceholderFragment extends BaseMwmFragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    final View view = inflater.inflate(R.layout.fragment_map_placeholder, container, false);
    view.findViewById(R.id.btn_continue).setOnClickListener((var x) -> DisplayManager.from(requireContext()).changeDisplay(DisplayType.Device));
    return view;
  }
}
