package com.mapswithme.maps.ads;

import android.annotation.SuppressLint;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class FacebookPedestrianFirstDialogFragment extends FacebookBasePedestrianDialogFragment
{
  protected View buildView(LayoutInflater inflater)
  {
    @SuppressLint("InflateParams") final View root = inflater.inflate(R.layout.fragment_pedestrian_dialog, null);

    final ImageView imageView = (ImageView) root.findViewById(R.id.iv__image);
    imageView.setImageResource(UiUtils.isTablet() ? R.drawable.ic_img_pedestrian_teblet : R.drawable.ic_img_pedestrian_phone);
    final TextView title = (TextView) root.findViewById(R.id.tv__title);
    title.setText(R.string.title_walking_available);
    final TextView text = (TextView) root.findViewById(R.id.tv__text);
    text.setText(R.string.share_walking_routes_first_launch);

    return root;
  }
}
