package com.mapswithme.maps.bookmarks;

import java.util.List;

import android.annotation.TargetApi;
import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public class BookmarkActivity extends AbstractBookmarkActivity
{
  private static final int BOOKMARK_COLOR_DIALOG = 11002;

  public static final String BOOKMARK_POSITION = "bookmark_position";
  public static final String PIN = "pin";
  public static final String PIN_ICON_ID = "pin";
  public static final String PIN_SET = "pin_set";
  public static final int REQUEST_CODE_SET = 0x1;
  public static final String BOOKMARK_NAME = "bookmark_name";

  private Bookmark mPin;
  private EditText mName;
  private EditText mDescr;
  private TextView mSet;
  private ImageView mPinImage;

  private int mCurrentCategoryId = -1;

  private Icon mIcon = null;
  private List<Icon> mIcons;


  public static void startWithBookmark(Context context, int category, int bookmark)
  {
    context.startActivity(new Intent(context, BookmarkActivity.class)
    .putExtra(BookmarkActivity.PIN, new ParcelablePoint(category, bookmark)));
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.add_or_edit_bookmark);

    // Note that Point result from the intent is actually a pair of (category index, bookmark index in category).
    assert(getIntent().getExtras().containsKey(PIN));
    final Point cab = ((ParcelablePoint)getIntent().getParcelableExtra(PIN)).getPoint();

    mPin = mManager.getBookmark(cab.x, cab.y);
    mCurrentCategoryId = mPin.getCategoryId();

    setTitle(mPin.getName());
    setUpViews();



    if (Utils.apiEqualOrGreaterThan(11) && getActionBar() != null)
    {
      final ActionBar ab = getActionBar();
      ab.setDisplayShowHomeEnabled(false);
      ab.setDisplayShowTitleEnabled(false);
      ab.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM);

      final View abView = getLayoutInflater().inflate(R.layout.done_delete, null);
      ab.setCustomView(abView);

      abView.findViewById(R.id.done).setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          onOkClick(null);
        }
      });
      abView.findViewById(R.id.delete).setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          onDeleteClick(null);
        }
      });
      UiUtils.hide(findViewById(R.id.btn_done), findViewById(R.id.btn_delete));
    }
  }


  private void updateColorChooser(Icon icon)
  {
    if (mIcon != null)
    {
      final String from = mIcon.getName();
      final String to   = icon.getName();
      if (!TextUtils.equals(from, to))
        Statistics.INSTANCE.trackColorChanged(this, from, to);
    }

    mIcon = icon;
    mPinImage.setImageDrawable(UiUtils
        .drawCircleForPin(mIcon.getName(), (int) getResources().getDimension(R.dimen.dp_x_6), getResources()));
  }

  private void refreshValuesInViews()
  {
    updateColorChooser(mPin.getIcon());


    Utils.setStringAndCursorToEnd(mName, mPin.getName());
    mSet.setText(mPin.getCategoryName(this));
    mDescr.setText(mPin.getBookmarkDescription());
  }

  private void setUpViews()
  {
    mIcons = mManager.getIcons();
    mSet = (TextView) findViewById(R.id.pin_set_chooser);
    mPinImage = (ImageView)findViewById(R.id.color_image);

    mName = (EditText) findViewById(R.id.pin_name);
    mDescr = (EditText)findViewById(R.id.pin_description);

    mPinImage.setOnClickListener(new OnClickListener()
    {
      @SuppressWarnings("deprecation")
      @Override
      public void onClick(View v)
      {
        showDialog(BOOKMARK_COLOR_DIALOG);
      }
    });

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
      public void afterTextChanged(Editable s) {}
    });
  }

  private void assignPinParams()
  {
    if (mPin != null)
    {
      final String descr = mDescr.getText().toString().trim();
      final String oldDescr = mPin.getBookmarkDescription().trim();
      if (!TextUtils.equals(descr, oldDescr))
        Statistics.INSTANCE.trackDescriptionChanged(this);

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
  @Deprecated
  protected Dialog onCreateDialog(int id)
  {
    if (id == BOOKMARK_COLOR_DIALOG)
      return createColorChooser();
    else
      return super.onCreateDialog(id);
  }

  private Dialog createColorChooser()
  {
    final IconsAdapter adapter = new IconsAdapter(this, mIcons);
    adapter.chooseItem(mIcons.indexOf(mPin.getIcon()));

    final LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
    final int padSide = (int) getResources().getDimension(R.dimen.dp_x_8);
    final int padTopB = (int) getResources().getDimension(R.dimen.dp_x_6);

    final GridView gView = new GridView(this);
    gView.setAdapter(adapter);
    gView.setNumColumns(4);
    gView.setGravity(Gravity.CENTER);
    gView.setPadding(padSide, padTopB, padSide, padTopB);
    gView.setLayoutParams(params);
    gView.setSelector(new ColorDrawable(Color.TRANSPARENT));

    final Dialog d = new AlertDialog.Builder(this)
      .setTitle(R.string.bookmark_color)
      .setView(gView)
      .create();

    gView.setOnItemClickListener(new OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> arg0, View who, int pos, long id)
      {
        updateColorChooser(mIcons.get(pos));
        adapter.chooseItem(pos);
        adapter.notifyDataSetChanged();
        d.dismiss();
      }});

    return d;
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == REQUEST_CODE_SET && resultCode == RESULT_OK)
    {

      final Point pin = ((ParcelablePoint)data.getParcelableExtra(PIN)).getPoint();
      mPin = mManager.getBookmark(pin.x, pin.y);
      refreshValuesInViews();

      if (mCurrentCategoryId != mPin.getCategoryId())
        Statistics.INSTANCE.trackGroupChanged(this);

      mCurrentCategoryId = mPin.getCategoryId();
    }
    super.onActivityResult(requestCode, resultCode, data);
  }

  public void onOkClick(View v)
  {
    onBackPressed();
  }

  public void onDeleteClick(View v)
  {
    if (mPin != null)
    {
      mManager.deleteBookmark(mPin);
      mPin = null;

      finish();
    }
  }
}
