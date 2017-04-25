package com.mapswithme.maps.search;

import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.UiUtils;

public class SearchHistoryFragment extends BaseMwmRecyclerFragment
{
  private PlaceholderView mPlaceHolder;

  private void updatePlaceholder()
  {
    UiUtils.showIf(getAdapter() != null && getAdapter().getItemCount() == 0, mPlaceHolder);
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new SearchHistoryAdapter(((SearchToolbarController.Container) getParentFragment()).getController());
  }

  @Override
  protected @LayoutRes int getLayoutRes()
  {
    return R.layout.fragment_search_base;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getRecyclerView().setLayoutManager(new LinearLayoutManager(view.getContext()));
    mPlaceHolder = (PlaceholderView) view.findViewById(R.id.placeholder);
    mPlaceHolder.setContent(R.drawable.img_search_empty_history_light,
                            R.string.search_history_title, R.string.search_history_text);

    if (getAdapter() != null)
    {
      getAdapter().registerAdapterDataObserver(new RecyclerView.AdapterDataObserver()
      {
        @Override
        public void onChanged()
        {
          updatePlaceholder();
        }
      });
    }
    updatePlaceholder();
  }

  @CallSuper
  @Override
  public void onActivityCreated(@Nullable Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);
    ((SearchFragment) getParentFragment()).setRecyclerScrollListener(getRecyclerView());
  }
}
