package app.organicmaps.maplayer;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.util.SharedPropertiesUtils;
import app.organicmaps.util.UiUtils;

import java.util.List;

public class LayersAdapter extends RecyclerView.Adapter<LayerHolder>
{
  @NonNull
  private final List<LayerBottomSheetItem> mItems;

  public LayersAdapter(@NonNull List<LayerBottomSheetItem> items)
  {
    mItems = items;
  }

  @NonNull
  @Override
  public LayerHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View root = inflater.inflate(R.layout.item_layer, parent, false);
    return new LayerHolder(root);
  }

  @Override
  public void onBindViewHolder(LayerHolder holder, int position)
  {
    Context context = holder.itemView.getContext();
    LayerBottomSheetItem item = mItems.get(position);
    holder.mItem = item;

    boolean isEnabled = item.getMode().isEnabled(context);

    holder.mButton.setSelected(isEnabled);
    holder.mTitle.setSelected(isEnabled);
    holder.mTitle.setText(item.getTitle());
    boolean isNewLayer = SharedPropertiesUtils.shouldShowNewMarkerForLayerMode(context,
        item.getMode());
    UiUtils.showIf(isNewLayer, holder.mNewMarker);
    holder.mButton.setImageResource(isEnabled ? item.getEnabledStateDrawable()
        : item.getDisabledStateDrawable());
    holder.mListener = item::onClick;
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }
}
