package app.organicmaps.homewidgets;

import android.appwidget.AppWidgetManager;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import java.util.ArrayList;
import java.util.List;

import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkCategory;
import app.organicmaps.bookmarks.data.BookmarkInfo;
import app.organicmaps.bookmarks.data.BookmarkManager;

public class FavoriteBookmarkWidgetConfigActivity extends AppCompatActivity {
    
    private static final String TAG = "BookmarkWidgetConfig";
    private int mAppWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID;
    private ListView mCategoriesListView;
    private ListView mBookmarksListView;
    private TextView mHeaderTextView;
    private Button mBackButton;
    
    private List<BookmarkCategory> mCategories = new ArrayList<>();
    private List<BookmarkInfo> mBookmarks = new ArrayList<>();
    private int mCurrentCategoryIndex = -1;
    private boolean mShowingCategories = true;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setResult(RESULT_CANCELED);
        
        Intent intent = getIntent();
        Bundle extras = intent.getExtras();
        if (extras != null) {
            mAppWidgetId = extras.getInt(
                AppWidgetManager.EXTRA_APPWIDGET_ID, 
                AppWidgetManager.INVALID_APPWIDGET_ID);
        }
        
        if (mAppWidgetId == AppWidgetManager.INVALID_APPWIDGET_ID) {
            finish();
            return;
        }
        
        setContentView(R.layout.activity_widget_config);
        
        mHeaderTextView = findViewById(R.id.header_text);
        mCategoriesListView = findViewById(R.id.categories_list);
        mBookmarksListView = findViewById(R.id.bookmarks_list);
        mBackButton = findViewById(R.id.back_button);
        
        mBackButton.setOnClickListener(v -> {
            if (!mShowingCategories) {
                showCategories();
            } else {
                setResult(RESULT_CANCELED);
                finish();
            }
        });
        
        loadCategories();
    }
    
    private void loadCategories() {
        mShowingCategories = true;
        mHeaderTextView.setText(R.string.select_category);
        mBookmarksListView.setVisibility(View.GONE);
        mCategoriesListView.setVisibility(View.VISIBLE);
        
        mCategories = BookmarkManager.INSTANCE.getCategories();
        
        List<BookmarkCategory> nonEmptyCategories = new ArrayList<>();
        for (BookmarkCategory category : mCategories) {
            if (category.getBookmarksCount() > 0) {
                nonEmptyCategories.add(category);
            }
        }
        mCategories = nonEmptyCategories;
        
        List<String> categoryNames = new ArrayList<>();
        for (BookmarkCategory category : mCategories) {
            categoryNames.add(category.getName());
        }
        
        ArrayAdapter<String> adapter = new ArrayAdapter<>(
            this, 
            android.R.layout.simple_list_item_1, 
            categoryNames);
            
        mCategoriesListView.setAdapter(adapter);
        
        mCategoriesListView.setOnItemClickListener((parent, view, position, id) -> {
            if (position >= 0 && position < mCategories.size()) {
                mCurrentCategoryIndex = position;
                showBookmarksForCategory(mCategories.get(position));
            }
        });
    }
    
    private void showBookmarksForCategory(BookmarkCategory category) {
        mShowingCategories = false;
        mHeaderTextView.setText(category.getName());
        mCategoriesListView.setVisibility(View.GONE);
        mBookmarksListView.setVisibility(View.VISIBLE);
        
        mBookmarks = new ArrayList<>();
        int bookmarksCount = category.getBookmarksCount();
        
        for (int i = 0; i < bookmarksCount; i++) {
            long bookmarkId = BookmarkManager.INSTANCE.getBookmarkIdByPosition(category.getId(), i);
            BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(bookmarkId);
            if (info != null) {
                mBookmarks.add(info);
            }
        }
        
        List<String> bookmarkNames = new ArrayList<>();
        for (BookmarkInfo bookmark : mBookmarks) {
            bookmarkNames.add(bookmark.getName());
        }
        
        ArrayAdapter<String> adapter = new ArrayAdapter<>(
            this, 
            android.R.layout.simple_list_item_1, 
            bookmarkNames);
            
        mBookmarksListView.setAdapter(adapter);
        
        mBookmarksListView.setOnItemClickListener((parent, view, position, id) -> {
            if (position >= 0 && position < mBookmarks.size()) {
                BookmarkInfo bookmark = mBookmarks.get(position);
                
                Log.d(TAG, "Selected bookmark: " + bookmark.getName() + " (categoryIndex=" + 
                      mCurrentCategoryIndex + ", bookmarkIndex=" + position + ")");
                
                FavoriteBookmarkWidget.saveBookmarkPref(
                    this, 
                    mAppWidgetId, 
                    mCurrentCategoryIndex,
                    position, 
                    bookmark.getName());
                
                AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(this);
                FavoriteBookmarkWidget.updateWidget(this, appWidgetManager, mAppWidgetId);
                
                Intent resultValue = new Intent();
                resultValue.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
                setResult(RESULT_OK, resultValue);
                
                finish();
            }
        });
    }
    
    private void showCategories() {
        mCurrentCategoryIndex = -1;
        loadCategories();
    }
    
    @Override
    public void onBackPressed() {
        if (!mShowingCategories) {
            showCategories();
        } else {
            setResult(RESULT_CANCELED);
            super.onBackPressed();
        }
    }
}