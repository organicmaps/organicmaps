package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.util.statistics.Statistics;

public class ChooseBookmarkCategoryFragment extends BaseMwmDialogFragment
                                         implements EditTextDialogFragment.OnTextSaveListener,
                                                    ChooseBookmarkCategoryAdapter.CategoryListener
{
  public static final String CATEGORY_POSITION = "ExtraCategoryPosition";

  private ChooseBookmarkCategoryAdapter mAdapter;
  private RecyclerView mRecycler;

  public interface Listener
  {
    void onCategoryChanged(long newCategoryId);
  }
  private Listener mListener;

  @Override
  protected int getStyle()
  {
    return STYLE_NO_TITLE;
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    mRecycler = (RecyclerView) inflater.inflate(R.layout.recycler_default, container, false);
    mRecycler.setLayoutManager(new org.solovyev.android.views.llm.LinearLayoutManager(getActivity()));

    return mRecycler;
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    final Bundle args = getArguments();
    final int catPosition = args.getInt(CATEGORY_POSITION, 0);
    mAdapter = new ChooseBookmarkCategoryAdapter(getActivity(), catPosition);
    mAdapter.setListener(this);
    mRecycler.setAdapter(mAdapter);
  }

  @Override
  public void onAttach(Activity activity)
  {
    if (mListener == null)
    {
      final Fragment parent = getParentFragment();
      if (parent instanceof Listener)
        mListener = (Listener) parent;
      else if (activity instanceof Listener)
        mListener = (Listener) activity;
    }

    super.onAttach(activity);
  }

  @Override
  public void onSaveText(String text)
  {
    createCategory(text);
  }

  private void createCategory(String name)
  {
    final long categoryId = BookmarkManager.INSTANCE.createCategory(name);
    final int categoryPosition = BookmarkManager.INSTANCE.getCategoriesCount() - 1;
    mAdapter.chooseItem(categoryPosition);

    if (mListener != null)
      mListener.onCategoryChanged(categoryId);
    dismiss();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.BMK_GROUP_CREATED);
  }

  @Override
  public void onCategorySet(int categoryPosition)
  {
    mAdapter.chooseItem(categoryPosition);
    if (mListener != null)
    {
      final long categoryId = BookmarkManager.INSTANCE.getCategoryIdByPosition(categoryPosition);
      mListener.onCategoryChanged(categoryId);
    }
    dismiss();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.BMK_GROUP_CHANGED);
  }

  @Override
  public void onCategoryCreate()
  {
    EditTextDialogFragment.show(getString(R.string.bookmark_set_name), null,
                                getString(R.string.ok), null, this);
  }
}
