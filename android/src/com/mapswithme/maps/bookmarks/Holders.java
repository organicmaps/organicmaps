package com.mapswithme.maps.bookmarks;

import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.ImageView;
import android.widget.SearchView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

public class Holders
{
  static class GeneralViewHolder extends RecyclerView.ViewHolder
  {

    GeneralViewHolder(@NonNull View itemView)
    {
      super(itemView);
    }
  }

  static class CategoryViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mName;
    @NonNull
    ImageView mVisibilityMarker;
    TextView mSize;

    CategoryViewHolder(@NonNull View root)
    {
      super(root);
      mName = root.findViewById(R.id.tv__set_name);
      mVisibilityMarker = root.findViewById(R.id.iv__set_visible);
      mSize = root.findViewById(R.id.tv__set_size);
    }

    void setVisibilityState(boolean visible)
    {
      Drawable drawable;
      if (visible)
      {
        mVisibilityMarker.setBackgroundResource(UiUtils.getStyledResourceId(
            mVisibilityMarker.getContext(), R.attr.activeIconBackground));
        drawable = Graphics.tint(mVisibilityMarker.getContext(), R.drawable.ic_bookmark_show, R.attr.activeIconTint);
      }
      else
      {
        mVisibilityMarker.setBackgroundResource(UiUtils.getStyledResourceId(
            mVisibilityMarker.getContext(), R.attr.steadyIconBackground));
        drawable = Graphics.tint(mVisibilityMarker.getContext(), R.drawable.ic_bookmark_hide,
                                 R.attr.steadyIconTint);
      }
      mVisibilityMarker.setImageDrawable(drawable);
    }

    void setVisibilityListener(@Nullable View.OnClickListener listener)
    {
      mVisibilityMarker.setOnClickListener(listener);
    }

    void setName(@NonNull String name)
    {
      mName.setText(name);
    }

    void setSize(int size)
    {
      mSize.setText(String.valueOf(size));
    }
  }
}
