package app.organicmaps.routing;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.widget.ArrowView;

import java.util.ArrayList;
import java.util.List;

public class LanesAdapter extends RecyclerView.Adapter<LanesAdapter.LanesViewHolder>
{
  @NonNull
  private final List<SingleLaneInfo> mItems = new ArrayList<>();

  @NonNull
  @Override
  public LanesViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    final View root = inflater.inflate(R.layout.nav_single_lane, parent, false);
    return new LanesViewHolder(root);
  }

  @Override
  public void onBindViewHolder(@NonNull LanesViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  public void setItems(@NonNull List<SingleLaneInfo> items)
  {
    mItems.clear();
    mItems.addAll(items);
    notifyDataSetChanged();
  }

  public void clearItems()
  {
    boolean alreadyEmpty = mItems.isEmpty();
    mItems.clear();
    if (!alreadyEmpty)
      notifyDataSetChanged();
  }

  static class LanesViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final ArrowView mArrow;

    public LanesViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mArrow = itemView.findViewById(R.id.lane_image);
    }

    void bind(@NonNull SingleLaneInfo info)
    {
    }
  }
}
