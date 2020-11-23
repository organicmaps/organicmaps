package com.mapswithme.maps.bookmarks;

import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.util.UiUtils;

class BookmarkDescriptionAdapter extends RecyclerView.Adapter<Holders.BookmarkDescriptionHolder>
{
  @NonNull
  private final BookmarkCategory mCategory;
  private boolean mVisible;
  @NonNull
  private final View.OnClickListener mOnClickListener;

  BookmarkDescriptionAdapter(@NonNull BookmarkCategory category,
                             @NonNull View.OnClickListener onClickListener)
  {
    mCategory = category;
    mOnClickListener = onClickListener;
  }

  @NonNull
  @Override
  public Holders.BookmarkDescriptionHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    BookmarkHeaderView view = new BookmarkHeaderView(parent.getContext());
    return new Holders.BookmarkDescriptionHolder(view);
  }

  @Override
  public void onBindViewHolder(@NonNull Holders.BookmarkDescriptionHolder holder, int position)
  {
    holder.bind(mCategory);
    View button = holder.itemView.findViewById(R.id.btn_description);
    button.setOnClickListener(mOnClickListener);
    UiUtils.showRecyclerItemView(mVisible, holder.itemView);
  }

  @Override
  public int getItemCount()
  {
    return 1;
  }

  void show(boolean visible)
  {
    mVisible = visible;
    notifyItemChanged(0);
  }
}
