package app.organicmaps.bookmarks;

import android.appwidget.AppWidgetManager;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.adapter.OnItemClickListener;
import app.organicmaps.bookmarks.data.BookmarkCategory;
import app.organicmaps.bookmarks.data.BookmarkInfo;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.content.DataSource;
import app.organicmaps.MwmApplication;
import app.organicmaps.util.Config;
import app.organicmaps.util.ThemeUtils;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class FavoriteBookmarkWidgetConfigActivity extends AppCompatActivity
{
  private static final String TAG = "BookmarkWidgetConfig";

  private int mAppWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID;
  private TextView mSelectTextView;
  private RecyclerView mCategoriesRecyclerView;
  private RecyclerView mBookmarksRecyclerView;
  private View mBackButton;

  private List<BookmarkCategory> mCategories = new ArrayList<>();
  private BookmarkCategory mCurrentCategory;
  private boolean mShowingCategories = true;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    applyTheme();

    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_widget_config);

    mSelectTextView = findViewById(R.id.select_text);
    mCategoriesRecyclerView = findViewById(R.id.categories_recycler);
    mBookmarksRecyclerView = findViewById(R.id.bookmarks_recycler);
    mBackButton = findViewById(R.id.back_button);

    mCategoriesRecyclerView.setLayoutManager(new LinearLayoutManager(this));
    mBookmarksRecyclerView.setLayoutManager(new LinearLayoutManager(this));

    mBackButton.setOnClickListener(v -> onBackPressed());

    setResult(RESULT_CANCELED);

    Intent intent = getIntent();
    Bundle extras = intent.getExtras();
    if (extras != null)
    {
      mAppWidgetId = extras.getInt(
          AppWidgetManager.EXTRA_APPWIDGET_ID,
          AppWidgetManager.INVALID_APPWIDGET_ID);
    }

    if (mAppWidgetId == AppWidgetManager.INVALID_APPWIDGET_ID)
    {
      finish();
      return;
    }

    try
    {
      MwmApplication app = MwmApplication.from(this);
      if (!app.arePlatformAndCoreInitialized())
      {
        app.init(() -> {
          runOnUiThread(this::loadCategories);
        });
      }
      else
      {
        loadCategories();
      }
    } catch (IOException e)
    {
      Log.e(TAG, "Failed to initialize app", e);
      finish();
    }
  }

  private void applyTheme()
  {
    String currentTheme = Config.getCurrentUiTheme(this);
    setTheme(ThemeUtils.getCardBgThemeResourceId(this, currentTheme));
  }

  private void loadCategories()
  {
    mShowingCategories = true;
    mBookmarksRecyclerView.setVisibility(View.GONE);
    mCategoriesRecyclerView.setVisibility(View.VISIBLE);
    mSelectTextView.setText(R.string.select_list);

    mCategories = BookmarkManager.INSTANCE.getCategories();
    List<BookmarkCategory> nonEmptyCategories = new ArrayList<>();
    for (BookmarkCategory category : mCategories)
    {
      if (category.getBookmarksCount() > 0)
      {
        nonEmptyCategories.add(category);
      }
    }
    mCategories = nonEmptyCategories;

    BookmarkWidgetCategoriesAdapter adapter = new BookmarkWidgetCategoriesAdapter(this, mCategories);

    adapter.setOnClickListener(new OnItemClickListener<BookmarkCategory>()
    {
      @Override
      public void onItemClick(View view, BookmarkCategory category)
      {
        mCurrentCategory = category;
        showBookmarksForCategory(category);
      }
    });

    mCategoriesRecyclerView.setAdapter(adapter);
  }

  private void showBookmarksForCategory(BookmarkCategory category)
  {
    mShowingCategories = false;
    mCategoriesRecyclerView.setVisibility(View.GONE);
    mBookmarksRecyclerView.setVisibility(View.VISIBLE);
    mSelectTextView.setText(R.string.select_bookmark);

    DataSource<BookmarkCategory> dataSource = new DataSource<BookmarkCategory>()
    {
      @Override
      public BookmarkCategory getData()
      {
        return category;
      }

      @Override
      public void invalidate()
      {
      }
    };

    BookmarkListAdapter adapter = new BookmarkListAdapter(dataSource);

    adapter.setOnClickListener((view, position) -> {
      Object item = adapter.getItem(position);
      if (item instanceof BookmarkInfo)
      {
        BookmarkInfo bookmarkInfo = (BookmarkInfo) item;

        FavoriteBookmarkWidget.saveBookmarkPref(
            this,
            mAppWidgetId,
            bookmarkInfo,
            mCurrentCategory);

        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(this);
        FavoriteBookmarkWidget.updateWidget(this, appWidgetManager, mAppWidgetId);

        Intent resultValue = new Intent();
        resultValue.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
        setResult(RESULT_OK, resultValue);

        finish();
      }
    });

    mBookmarksRecyclerView.setAdapter(adapter);
  }

  @Override
  public void onBackPressed()
  {
    if (!mShowingCategories)
    {
      loadCategories();
    }
    else
    {
      setResult(RESULT_CANCELED);
      super.onBackPressed();
    }
  }
}
