package com.mapswithme.maps.gallery;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.widget.recycler.GridDividerItemDecoration;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;

import java.util.ArrayList;

public class GalleryFragment extends BaseMwmFragment implements RecyclerClickListener
{
  private static final int NUM_COLUMNS = 3;

  @Nullable
  private ArrayList<Image> mImages;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_gallery, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    readArguments();

    if (mImages != null)
    {
      RecyclerView rvGallery = (RecyclerView) view.findViewById(R.id.rv__gallery);
      rvGallery.setLayoutManager(new GridLayoutManager(getContext(), NUM_COLUMNS));
      rvGallery.setAdapter(new ImageAdapter(mImages, this));
      Drawable divider = ContextCompat.getDrawable(getContext(), R.drawable.divider_transparent_quarter);
      rvGallery.addItemDecoration(new GridDividerItemDecoration(divider, divider, NUM_COLUMNS));
    }
  }

  private void readArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return;

    mImages = arguments.getParcelableArrayList(GalleryActivity.EXTRA_IMAGES);
  }

  @Override
  public void onItemClick(View v, int position)
  {
    FullScreenGalleryActivity.start(getContext(), mImages, position);
  }
}
