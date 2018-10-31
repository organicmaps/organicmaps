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
  private final AdapterPositionConverter mIndexConverter;

  @SafeVarargs
  public RecyclerCompositeAdapter(@NonNull AdapterPositionConverter indexConverter,
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
  public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int absViewType)
  {
    AdapterIndexAndViewType indexAndViewType = mIndexConverter.toRelativeViewTypeAndAdapterIndex(absViewType);

    int adapterIndex = indexAndViewType.getIndex();
    int relViewType = indexAndViewType.getRelativeViewType();
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(adapterIndex);
    return adapter.onCreateViewHolder(parent, relViewType);
  }

  @Override
  public int getItemViewType(int position)
  {
    AdapterIndexAndPosition indexAndPosition = mIndexConverter.toRelativePositionAndAdapterIndex(position);

    int adapterIndex = indexAndPosition.getIndex();
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(adapterIndex);
    int relViewType = adapter.getItemViewType(indexAndPosition.getRelativePosition());
    int absViewType = mIndexConverter.toAbsoluteViewType(relViewType, adapterIndex);

    return absViewType;
  }

  @Override
  public void onBindViewHolder(RecyclerView.ViewHolder holder, int position)
  {
    AdapterIndexAndPosition indexAndPosition = mIndexConverter.toRelativePositionAndAdapterIndex(position);
    int adapterIndex = indexAndPosition.getIndex();
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(adapterIndex);
    int relPosition = indexAndPosition.getRelativePosition();
    bindViewHolder(adapter, holder, relPosition);
  }

  @SuppressWarnings("unchecked")
  private <Holder extends RecyclerView.ViewHolder> void bindViewHolder(@NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter,
                                                                       @NonNull Holder holder,
                                                                       int position)
  {
    ((RecyclerView.Adapter<Holder>) adapter).onBindViewHolder(holder, position);
  }

  protected static abstract class AbstractAdapterPositionConverter implements AdapterPositionConverter
  {
    @NonNull
    @Override
    public AdapterIndexAndPosition toRelativePositionAndAdapterIndex(int absPosition)
    {
      return getIndexAndPositionItems().get(absPosition);
    }

    @NonNull
    @Override
    public AdapterIndexAndViewType toRelativeViewTypeAndAdapterIndex(int absViewType)
    {
      return getIndexAndViewTypeItems().get(absViewType);
    }

    @Override
    public int toAbsoluteViewType(int relViewType, int adapterIndex)
    {
      AdapterIndexAndViewType indexAndViewType = new AdapterIndexAndViewTypeImpl(adapterIndex, relViewType);
      List<AdapterIndexAndViewType> items = getIndexAndViewTypeItems();
      int indexOf = items.indexOf(indexAndViewType);
      if (indexOf < 0)
      {
        throw new IllegalArgumentException("Item " + indexAndViewType + " not found in list : " +
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
