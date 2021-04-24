package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;

import androidx.annotation.CallSuper;
import androidx.annotation.IdRes;
import androidx.annotation.LayoutRes;
import androidx.annotation.MenuRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import com.cocosw.bottomsheet.BottomSheet;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.base.DataChangedListener;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.BookmarkSharingResult;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.BottomSheetHelper;

import java.util.List;

public class BookmarkCategoriesFragment extends BaseMwmRecyclerFragment<BookmarkCategoriesAdapter>
    implements EditTextDialogFragment.EditTextDialogInterface,
    MenuItem.OnMenuItemClickListener,
    BookmarkManager.BookmarksLoadingListener,
    CategoryListCallback,
    OnItemClickListener<BookmarkCategory>,
    OnItemLongClickListener<BookmarkCategory>, BookmarkManager.BookmarksSharingListener

{
  static final int REQ_CODE_DELETE_CATEGORY = 102;

  private static final int MAX_CATEGORY_NAME_LENGTH = 60;

  @Nullable
  private BookmarkCategory mSelectedCategory;
  @Nullable
  private CategoryEditor mCategoryEditor;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private DataChangedListener mCategoriesAdapterObserver;

  @Override
  @LayoutRes
  protected int getLayoutRes()
  {
    return R.layout.fragment_bookmark_categories;
  }

  @NonNull
  @Override
  protected BookmarkCategoriesAdapter createAdapter()
  {
    List<BookmarkCategory> items = BookmarkManager.INSTANCE.getCategories();
    return new BookmarkCategoriesAdapter(requireContext(), items);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    onPrepareControllers(view);
    getAdapter().setOnClickListener(this);
    getAdapter().setOnLongClickListener(this);
    getAdapter().setCategoryListCallback(this);

    RecyclerView rw = getRecyclerView();
    if (rw == null) return;

    rw.setNestedScrollingEnabled(false);
    RecyclerView.ItemDecoration decor = ItemDecoratorFactory
        .createDecoratorWithPadding(getContext());
    rw.addItemDecoration(decor);
    mCategoriesAdapterObserver = new CategoriesAdapterObserver(this);
    BookmarkManager.INSTANCE.addCategoriesUpdatesListener(mCategoriesAdapterObserver);
  }

  protected void onPrepareControllers(@NonNull View view)
  {
    // No op
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    BookmarksSharingHelper.INSTANCE.onPreparedFileForSharing(requireActivity(), result);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addLoadingListener(this);
  }
  
  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeLoadingListener(this);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    getAdapter().notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    super.onPause();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(mCategoriesAdapterObserver);
  }

  @Override
  public boolean onMenuItemClick(MenuItem item)
  {
    MenuItemClickProcessorWrapper processor = MenuItemClickProcessorWrapper
        .getInstance(item.getItemId());

    processor
        .mInternalProcessor
        .process(this, getSelectedCategory());
    return true;
  }

  protected final void showBottomMenu(@NonNull BookmarkCategory item)
  {
    mSelectedCategory = item;
    showBottomMenuInternal(item);
  }

  private void showBottomMenuInternal(@NonNull BookmarkCategory item)
  {
    BottomSheetHelper.Builder bs = BottomSheetHelper.create(getActivity(), item.getName())
        .sheet(getCategoryMenuResId())
        .listener(this);

    BottomSheet bottomSheet = bs.build();
    prepareBottomMenuItems(bottomSheet);
    MenuItem menuItem = BottomSheetHelper.findItemById(bottomSheet, R.id.show_on_map);
    menuItem.setIcon(item.isVisible() ? R.drawable.ic_hide : R.drawable.ic_show)
        .setTitle(item.isVisible() ? R.string.hide : R.string.show);
    BottomSheetHelper.tint(bottomSheet);
    bottomSheet.show();
  }

  protected void prepareBottomMenuItems(@NonNull BottomSheet bottomSheet)
  {
    boolean isMultipleItems = getAdapter().getBookmarkCategories().size() > 1;
    setEnableForMenuItem(R.id.delete, bottomSheet, isMultipleItems);
  }

  @MenuRes
  protected int getCategoryMenuResId()
  {
    return R.menu.menu_bookmark_categories;
  }

  @Override
  public void onMoreOperationClick(@NonNull BookmarkCategory item)
  {
    showBottomMenu(item);
  }

  @Override
  protected void setupPlaceholder(@Nullable PlaceholderView placeholder)
  {
    // A placeholder is no needed on this screen.
  }

  @Override
  public void onBookmarksLoadingStarted()
  {
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    getAdapter().notifyDataSetChanged();
  }

  @Override
  public void onBookmarksFileLoaded(boolean success)
  {
    // Do nothing here.
  }

  @Override
  public void onFooterClick()
  {
    mCategoryEditor = BookmarkManager.INSTANCE::createCategory;

    EditTextDialogFragment.show(getString(R.string.bookmarks_create_new_group),
        getString(R.string.bookmarks_new_list_hint),
        getString(R.string.bookmark_set_name),
        getString(R.string.create), getString(R.string.cancel),
        MAX_CATEGORY_NAME_LENGTH, this);
  }

  @NonNull
  @Override
  public EditTextDialogFragment.OnTextSaveListener getSaveTextListener()
  {
    return this::onSaveText;
  }

  @NonNull
  @Override
  public EditTextDialogFragment.Validator getValidator()
  {
    return new CategoryValidator();
  }

  @Override
  public void onItemClick(@NonNull View v, @NonNull BookmarkCategory category)
  {
    mSelectedCategory = category;
    BookmarkListActivity.startForResult(this, category);
  }

  protected void onShareActionSelected(@NonNull BookmarkCategory category)
  {
    BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(getActivity(), category.getId());
  }

  private void onDeleteActionSelected(@NonNull BookmarkCategory category)
  {
    BookmarkManager.INSTANCE.deleteCategory(category.getId());
    getAdapter().notifyDataSetChanged();
    onDeleteActionSelected();
  }

  protected void onDeleteActionSelected()
  {
    // Do nothing.
  }

  @Override
  public final void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (resultCode == Activity.RESULT_OK && requestCode == REQ_CODE_DELETE_CATEGORY)
    {
      onDeleteActionSelected(getSelectedCategory());
      return;
    }
  }

  @Override
  public void onItemLongClick(@NonNull View v, @NonNull BookmarkCategory category)
  {
    showBottomMenu(category);
  }

  static void setEnableForMenuItem(@IdRes int id, @NonNull BottomSheet bottomSheet,
                                   boolean enable)
  {
    BottomSheetHelper
        .findItemById(bottomSheet, id)
        .setVisible(enable)
        .setEnabled(enable);
  }

  private void onSaveText(@NonNull String text)
  {
    if (mCategoryEditor != null)
      mCategoryEditor.commit(text);

    getAdapter().notifyDataSetChanged();
  }

  @NonNull
  protected BookmarkCategory getSelectedCategory()
  {
    if (mSelectedCategory == null)
      throw new AssertionError("Invalid attempt to use null selected category.");
    return mSelectedCategory;
  }

  interface CategoryEditor
  {
    void commit(@NonNull String newName);
  }


  protected enum MenuItemClickProcessorWrapper
  {
    SET_SHARE(R.id.share, shareAction()),
    SET_EDIT(R.id.edit, editAction()),
    SHOW_ON_MAP(R.id.show_on_map, showAction()),
    LIST_SETTINGS(R.id.settings, showListSettings()),
    DELETE_LIST(R.id.delete, deleteAction());

    @NonNull
    private static MenuClickProcessorBase showListSettings()
    {
      return new MenuClickProcessorBase.OpenListSettings();
    }

    @NonNull
    private static MenuClickProcessorBase.ShowAction showAction()
    {
      return new MenuClickProcessorBase.ShowAction();
    }

    @NonNull
    private static MenuClickProcessorBase.ShareAction shareAction()
    {
      return new MenuClickProcessorBase.ShareAction();
    }

    @NonNull
    private static MenuClickProcessorBase.DeleteAction deleteAction()
    {
      return new MenuClickProcessorBase.DeleteAction();
    }

    @NonNull
    private static MenuClickProcessorBase.EditAction editAction()
    {
      return new MenuClickProcessorBase.EditAction();
    }

    @IdRes
    private final int mId;
    @NonNull
    private final MenuClickProcessorBase mInternalProcessor;

    MenuItemClickProcessorWrapper(@IdRes int id, @NonNull MenuClickProcessorBase processorBase)
    {
      mId = id;
      mInternalProcessor = processorBase;
    }

    @NonNull
    public static MenuItemClickProcessorWrapper getInstance(@IdRes int resId)
    {
      for (MenuItemClickProcessorWrapper each : values())
      {
        if (each.mId == resId)
        {
          return each;
        }
      }
      throw new IllegalArgumentException("Enum value for res id = " + resId + " not found");
    }
  }

  protected static abstract class MenuClickProcessorBase
  {
    public abstract void process(@NonNull BookmarkCategoriesFragment frag,
                                 @NonNull BookmarkCategory category);

    protected static class ShowAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        BookmarkManager.INSTANCE.toggleCategoryVisibility(category);
        frag.getAdapter().notifyDataSetChanged();
      }
    }

    protected static class ShareAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        frag.onShareActionSelected(category);
      }
    }

    protected static class DeleteAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        frag.onDeleteActionSelected(category);
      }
    }

    protected static class EditAction extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        frag.mCategoryEditor = newName ->
        {
          BookmarkManager.INSTANCE.setCategoryName(category.getId(), newName);
        };
        EditTextDialogFragment.show(frag.getString(R.string.bookmark_set_name),
            category.getName(),
            frag.getString(R.string.rename),
            frag.getString(R.string.cancel),
            MAX_CATEGORY_NAME_LENGTH,
            frag);
      }
    }

    protected static class OpenListSettings extends MenuClickProcessorBase
    {
      @Override
      public void process(@NonNull BookmarkCategoriesFragment frag,
                          @NonNull BookmarkCategory category)
      {
        BookmarkCategorySettingsActivity.startForResult(frag, category);
      }
    }
  }

  private static class CategoriesAdapterObserver implements DataChangedListener<BookmarkCategoriesFragment>
  {
    @Nullable
    private BookmarkCategoriesFragment mFragment;

    CategoriesAdapterObserver(@NonNull BookmarkCategoriesFragment fragment)
    {
      mFragment = fragment;
    }

    @Override
    public void attach(@NonNull BookmarkCategoriesFragment object)
    {
      mFragment = object;
    }

    @Override
    public void detach()
    {
      mFragment = null;
    }

    @Override
    public void onChanged()
    {
      if (mFragment == null)
        return;

      mFragment.getAdapter().setItems(BookmarkManager.INSTANCE.getCategories());
    }
  }
}
