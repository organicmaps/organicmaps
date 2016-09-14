package com.mapswithme.maps.gallery;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.widget.recycler.GridDividerItemDecoration;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;

public class GalleryFragment extends BaseMwmFragment implements RecyclerClickListener {

  private ArrayList<Image> mImages;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
          @Nullable Bundle savedInstanceState) {
    return inflater.inflate(R.layout.fragment_gallery, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    readArguments();

    RecyclerView rvGallery = (RecyclerView) view.findViewById(R.id.rv__gallery);
    rvGallery.setLayoutManager(new GridLayoutManager(getContext(), 3));
    rvGallery.setAdapter(new ImageAdapter(mImages, this));
    Drawable divider = ContextCompat.getDrawable(getContext(), R.drawable.divider_transparent);
    rvGallery.addItemDecoration(new GridDividerItemDecoration(divider, divider, 3));
  }

  private void readArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return;

    mImages = arguments.getParcelableArrayList(GalleryActivity.EXTRA_IMAGES);
  }

  @Override
  public void onItemClick(View v, int position) {
//  TODO show full screen image activity
  }
}
