package com.mapswithme.maps.adapter;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class RecyclerCompositeAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  @NonNull
  private final List<RecyclerView.Adapter<? extends RecyclerView.ViewHolder>> mAdapters = new ArrayList<>();
  @NonNull
  private final AdapterIndexConverter mIndexConverter;

  @SafeVarargs
  public RecyclerCompositeAdapter(@NonNull AdapterIndexConverter indexConverter,
                                  @NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder>... adapters)
  {
    mIndexConverter = indexConverter;
    mAdapters.addAll(Arrays.asList(adapters));
  }

  @Override
  public final int getItemCount()
  {
    int total = 0;
    for (RecyclerView.Adapter<? extends RecyclerView.ViewHolder> each : mAdapters)
    {
      total += each.getItemCount();
    }

    return total;
  }

  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    AdapterIndexAndViewType entity = mIndexConverter.getRelativeViewType(viewType);
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(entity.getIndex());
    return adapter.onCreateViewHolder(parent, entity.getRelativeViewType());
  }

  @Override
  public int getItemViewType(int position)
  {
    AdapterIndexAndPosition entity = mIndexConverter.getRelativePosition(position);
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(entity.getIndex());

    int relativeViewType = adapter.getItemViewType(entity.getRelativePosition());
    AdapterIndexAndViewType type = new AdapterIndexAndViewTypeImpl(entity.getIndex(),
                                                                   relativeViewType);
    int absoluteViewType = mIndexConverter.getAbsoluteViewType(type);

    return absoluteViewType;
  }

  @Override
  public void onBindViewHolder(RecyclerView.ViewHolder holder, int position)
  {
    AdapterIndexAndPosition index = mIndexConverter.getRelativePosition(position);
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(index.getIndex());
    bindViewHolder(adapter, holder, index.getRelativePosition());
  }

  private <Holder extends RecyclerView.ViewHolder> void bindViewHolder(@NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter,
                                                                       @NonNull Holder holder,
                                                                       int position)
  {
    ((RecyclerView.Adapter<Holder>) adapter).onBindViewHolder(holder, position);
  }

  protected static abstract class AbstractAdapterIndexConverter implements AdapterIndexConverter
  {
    @NonNull
    @Override
    public AdapterIndexAndPosition getRelativePosition(int absPosition)
    {
      return getIndexAndPositionItems().get(absPosition);
    }

    @NonNull
    @Override
    public AdapterIndexAndViewType getRelativeViewType(int absViewType)
    {
      return getIndexAndViewTypeItems().get(absViewType);
    }

    @Override
    public int getAbsoluteViewType(@NonNull AdapterIndexAndViewType relViewType)
    {
      List<AdapterIndexAndViewType> items = getIndexAndViewTypeItems();
      int indexOf = items.indexOf(relViewType);
      if (indexOf < 0)
      {
        throw new IllegalArgumentException("Item " + relViewType + " not found in list : " +
                                           Arrays.toString(items.toArray()));
      }
      return indexOf;
    }

    @NonNull
    protected abstract List<AdapterIndexAndViewType> getIndexAndViewTypeItems();

    @NonNull
    protected abstract List<AdapterIndexAndPosition> getIndexAndPositionItems();
  }
}
