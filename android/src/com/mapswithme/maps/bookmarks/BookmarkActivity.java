package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Point;
import android.os.Bundle;
import android.support.v7.app.ActionBar;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MWMFragmentActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public class BookmarkActivity extends MWMFragmentActivity
{
  public static final String BOOKMARK_POSITION = "bookmark_position";
  public static final String PIN = "pin";
  public static final String PIN_ICON_ID = "pin";
  public static final String PIN_SET = "pin_set";
  public static final int REQUEST_CODE_SET = 0x1;
  public static final String BOOKMARK_NAME = "bookmark_name";
  public static final int REQUEST_CODE_EDIT_BOOKMARK = 0x2;

  private BookmarkManager mManager;
  private Bookmark mPin;
  private EditText mName;
  private View mClearName;
  private EditText mDescr;
  private TextView mSet;

  private int mCurrentCategoryId = -1;

  private Icon mIcon = null;

  public static void startWithBookmark(Activity context, int category, int bookmark)
  {
    context.startActivityForResult(new Intent(context, BookmarkActivity.class)
        .putExtra(BookmarkActivity.PIN, new ParcelablePoint(category, bookmark)), REQUEST_CODE_EDIT_BOOKMARK);
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.add_or_edit_bookmark);

    // Note that Point result from the intent is actually a pair of
    // (category index, bookmark index in category).
    final Point cab = ((ParcelablePoint) getIntent().getParcelableExtra(PIN)).getPoint();

    mManager = BookmarkManager.INSTANCE;
    mPin = mManager.getBookmark(cab.x, cab.y);
    mCurrentCategoryId = mPin.getCategoryId();

    setTitle(mPin.getName());
    setUpViews();

    final ActionBar ab = getSupportActionBar();
    if (ab != null)
    {
      ab.setDisplayShowHomeEnabled(false);
      ab.setDisplayShowTitleEnabled(false);
      ab.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM);

      final View abView = getLayoutInflater().inflate(R.layout.done_delete, (android.view.ViewGroup) findViewById(android.R.id.content), false);
      ab.setCustomView(abView);

      abView.findViewById(R.id.done).setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          onBackPressed();
        }
      });
      abView.findViewById(R.id.delete).setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          deleteBookmark();
        }
      });
    }
  }

  private void refreshValuesInViews()
  {
    Utils.setTextAndCursorToEnd(mName, mPin.getName());

    mSet.setText(mPin.getCategoryName(this));
    mDescr.setText(mPin.getBookmarkDescription());

    setUpClearNameView();
  }

  private void setUpViews()
  {
    mSet = (TextView) findViewById(R.id.pin_set_chooser);

    mName = (EditText) findViewById(R.id.pin_name);
    mClearName = findViewById(R.id.bookmark_name_clear);
    mClearName.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        mName.getText().clear();
      }
    });
    mDescr = (EditText) findViewById(R.id.pin_description);

    mSet.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        startActivityForResult(new Intent(BookmarkActivity.this, ChooseBookmarkCategoryActivity.class)
            .putExtra(PIN_SET, mCurrentCategoryId)
            .putExtra(PIN, new ParcelablePoint(mPin.getCategoryId(), mPin.getBookmarkId())), REQUEST_CODE_SET);
      }
    });

    refreshValuesInViews();

    mName.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        setTitle(s.toString());
        // Note! Do not set actual name here - saving process may be too long
        // see assignPinParams() instead.
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

      @Override
      public void afterTextChanged(Editable s)
      {
        setUpClearNameView();
      }
    });

    mName.requestFocus();
  }

  private void assignPinParams()
  {
    if (mPin != null)
    {
      final String descr = mDescr.getText().toString().trim();
      final String oldDescr = mPin.getBookmarkDescription().trim();
      if (!TextUtils.equals(descr, oldDescr))
        Statistics.INSTANCE.trackDescriptionChanged();

      mPin.setParams(mName.getText().toString(), mIcon, descr);
    }
  }

  @Override
  protected void onPause()
  {
    assignPinParams();

    super.onPause();
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == REQUEST_CODE_SET && resultCode == RESULT_OK)
    {

      final Point pin = ((ParcelablePoint) data.getParcelableExtra(PIN)).getPoint();
      mPin = mManager.getBookmark(pin.x, pin.y);
      refreshValuesInViews();

      if (mCurrentCategoryId != mPin.getCategoryId())
        Statistics.INSTANCE.trackGroupChanged();

      mCurrentCategoryId = mPin.getCategoryId();
    }
    super.onActivityResult(requestCode, resultCode, data);
  }

  public void deleteBookmark()
  {
    if (mPin != null)
    {
      mManager.deleteBookmark(mPin);
      mPin = null;

      finish();
    }
  }

  private void setUpClearNameView()
  {
    if (TextUtils.isEmpty(mName.getText().toString()))
      UiUtils.invisible(mClearName);
    else
      UiUtils.show(mClearName);
  }

  @Override
  public void onBackPressed()
  {
    setResult(RESULT_OK, new Intent().putExtra(BookmarkActivity.PIN, new ParcelablePoint(mPin.getCategoryId(), mPin.getBookmarkId())));

    super.onBackPressed();
  }
}
