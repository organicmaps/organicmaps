package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;

public class AdapterDataObserverWrapper extends RecyclerView.AdapterDataObserver
{
  @NonNull
  private final RecyclerView.Adapter<? extends RecyclerView.ViewHolder> mWrapped;

  public AdapterDataObserverWrapper(@NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> wrapped)
  {
    mWrapped = wrapped;
  }

  @Override
  public void onChanged()
  {
    super.onChanged();
    mWrapped.notifyDataSetChanged();
  }

  @Override
  public void onItemRangeChanged(int positionStart, int itemCount)
  {
    super.onItemRangeChanged(positionStart, itemCount);
    mWrapped.notifyItemChanged(positionStart, itemCount);
  }

  @Override
  public void onItemRangeChanged(int positionStart, int itemCount, @Nullable Object payload)
  {
    super.onItemRangeChanged(positionStart, itemCount, payload);
    mWrapped.notifyItemRangeChanged(positionStart, itemCount, payload);
  }

  @Override
  public void onItemRangeInserted(int positionStart, int itemCount)
  {
    super.onItemRangeInserted(positionStart, itemCount);
    mWrapped.notifyItemRangeInserted(positionStart, itemCount);
  }

  @Override
  public void onItemRangeRemoved(int positionStart, int itemCount)
  {
    super.onItemRangeRemoved(positionStart, itemCount);
    mWrapped.notifyItemRangeRemoved(positionStart, itemCount);
  }

  @Override
  public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount)
  {
    super.onItemRangeMoved(fromPosition, toPosition, itemCount);
    mWrapped.notifyItemMoved(fromPosition, toPosition);
  }
}
