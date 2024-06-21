package app.organicmaps.maplayer;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.adapter.OnItemClickListener;

class RouteHolder extends RecyclerView.ViewHolder
{
  @NonNull
  final TextView mTitle;
  @NonNull
  final ImageView mRenameButton;
  @NonNull
  final ImageView mDeleteButton;
  @Nullable
  RouteBottomSheetItem mItem;
  @Nullable
  OnItemClickListener<RouteBottomSheetItem> mTitleListener;
  @Nullable
  OnItemClickListener<RouteBottomSheetItem> mRenameListener;
  @Nullable
  OnItemClickListener<RouteBottomSheetItem> mDeleteListener;

  RouteHolder(@NonNull View root)
  {
    super(root);
    mTitle = root.findViewById(R.id.name);
    mTitle.setOnClickListener(this::onItemTitleClicked);
    mRenameButton = root.findViewById(R.id.rename);
    mRenameButton.setOnClickListener(this::onItemRenameClicked);
    mDeleteButton = root.findViewById(R.id.delete);
    mDeleteButton.setOnClickListener(this::onItemDeleteClicked);
  }

  public void onItemTitleClicked(@NonNull View v)
  {
    if (mTitleListener != null && mItem != null)
      mTitleListener.onItemClick(v, mItem);
  }

  public void onItemRenameClicked(@NonNull View v)
  {
    if (mRenameListener != null && mItem != null)
      mRenameListener.onItemClick(v, mItem);
  }

  public void onItemDeleteClicked(@NonNull View v)
  {
    if (mDeleteListener != null && mItem != null)
      mDeleteListener.onItemClick(v, mItem);
  }
}
