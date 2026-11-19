package app.organicmaps.search;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;

/** Placeholder below bookmark results while map results are still loading. */
final class MapResultsLoadingAdapter extends RecyclerView.Adapter<MapResultsLoadingAdapter.Holder>
{
  private boolean visible;

  void setVisible(boolean value)
  {
    if (visible == value)
      return;
    visible = value;
    notifyDataSetChanged();
  }

  @Override
  public int getItemCount()
  {
    return visible ? 3 : 0;
  }
  @NonNull
  @Override
  public Holder onCreateViewHolder(@NonNull ViewGroup parent, int type)
  {
    return new Holder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_search_shimmer, parent, false));
  }
  @Override
  public void onBindViewHolder(@NonNull Holder holder, int position)
  {}
  static final class Holder extends RecyclerView.ViewHolder
  {
    Holder(View view)
    {
      super(view);
    }
  }
}
