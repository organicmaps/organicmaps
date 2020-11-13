package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;

public class ChooseBookmarkCategoryFragment extends BaseMwmDialogFragment
                                         implements EditTextDialogFragment.EditTextDialogInterface,
                                                    ChooseBookmarkCategoryAdapter.CategoryListener
{
  public static final String CATEGORY_POSITION = "ExtraCategoryPosition";

  private ChooseBookmarkCategoryAdapter mAdapter;
  private RecyclerView mRecycler;

  public interface Listener
  {
    void onCategoryChanged(@NonNull BookmarkCategory newCategory);
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
    View root = inflater.inflate(R.layout.choose_bookmark_category_fragment, container, false);
    mRecycler = root.findViewById(R.id.recycler);
    mRecycler.setLayoutManager(new LinearLayoutManager(getActivity()));
    return root;
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    final Bundle args = getArguments();
    final int catPosition = args.getInt(CATEGORY_POSITION, 0);
    List<BookmarkCategory> items = BookmarkManager.INSTANCE.getOwnedCategoriesSnapshot().getItems();
    mAdapter = new ChooseBookmarkCategoryAdapter(getActivity(), catPosition, items);
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

  @NonNull
  @Override
  public EditTextDialogFragment.OnTextSaveListener getSaveTextListener()
  {
    return this::createCategory;
  }

  @NonNull
  @Override
  public EditTextDialogFragment.Validator getValidator()
  {
    return new CategoryValidator();
  }


  private void createCategory(@NonNull String name)
  {
    BookmarkManager.INSTANCE.createCategory(name);

    List<BookmarkCategory> bookmarkCategories = mAdapter.getBookmarkCategories();

    if (bookmarkCategories.size() == 0)
      throw new AssertionError("BookmarkCategories are empty");

    final int categoryPosition = bookmarkCategories.size() - 1;
    mAdapter.chooseItem(categoryPosition);

    if (mListener != null)
    {
      BookmarkCategory newCategory = bookmarkCategories.get(categoryPosition);
      mListener.onCategoryChanged(newCategory);
    }
    dismiss();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.BM_GROUP_CREATED);
  }

  @Override
  public void onCategorySet(int categoryPosition)
  {
    mAdapter.chooseItem(categoryPosition);
    if (mListener != null)
    {
      final BookmarkCategory category = mAdapter.getBookmarkCategories().get(categoryPosition);
      mListener.onCategoryChanged(category);
    }
    dismiss();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.BM_GROUP_CHANGED);
  }

  @Override
  public void onCategoryCreate()
  {
    EditTextDialogFragment.show(getString(R.string.bookmark_set_name), null,
                                getString(R.string.ok), null, this);
  }
}
