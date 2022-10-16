package app.organicmaps.search;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.CallSuper;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.widget.PlaceholderView;
import app.organicmaps.widget.SearchToolbarController;
import app.organicmaps.util.UiUtils;

public class SearchHistoryFragment extends BaseMwmRecyclerFragment<SearchHistoryAdapter>
{
  private PlaceholderView mPlaceHolder;

  private void updatePlaceholder()
  {
    UiUtils.showIf(getAdapter().getItemCount() == 0, mPlaceHolder);
  }

  @NonNull
  @Override
  protected SearchHistoryAdapter createAdapter()
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
    mPlaceHolder = view.findViewById(R.id.placeholder);
    mPlaceHolder.setContent(R.string.search_history_title, R.string.search_history_text);

    getAdapter().registerAdapterDataObserver(new RecyclerView.AdapterDataObserver()
    {
      @Override
      public void onChanged()
      {
        updatePlaceholder();
      }
    });
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
