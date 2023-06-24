package app.organicmaps.routing;

import android.content.res.ColorStateList;
import android.util.TypedValue;
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

    private void setIconTint(@NonNull SingleLaneInfo info)
    {
      int iconTint = info.mIsActive ? R.attr.iconTint : R.attr.iconTintLight;
      TypedValue color = new TypedValue();
      mArrow.getContext().getTheme().resolveAttribute(iconTint, color, true);
      mArrow.setImageTintList(ColorStateList.valueOf(color.data));
    }

    private void setIcon(@NonNull SingleLaneInfo info)
    {
      boolean haveLaneData = (info.mLane.length > 0);
      int imageRes = haveLaneData ? info.mLane[0].mTurnRes : 0;
      mArrow.setImageResource(imageRes);
    }

    void bind(@NonNull SingleLaneInfo info)
    {
      setIconTint(info);
      setIcon(info);
    }
  }
}
