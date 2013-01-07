package com.mapswithme.maps.bookmarks;

import java.util.List;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Point;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;

public class BookmarkActivity extends AbstractBookmarkActivity
{
  private static final int CONFIRMATION_DIALOG = 11001;

  public static final String BOOKMARK_POSITION = "bookmark_position";
  public static final String PIN = "pin";
  public static final String PIN_ICON_ID = "pin";
  public static final String PIN_SET = "pin_set";
  public static final int REQUEST_CODE_SET = 567890;
  public static final String BOOKMARK_NAME = "bookmark_name";
  private Spinner mSpinner;
  private Bookmark mPin;
  private EditText mName;
  private TextView mSetName;
  private int mCurrentCategoryId = -1;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.pin);
    if (getIntent().getExtras().containsKey(BOOKMARK_POSITION))
    {
      // create new bookmark
      mPin = mManager.getBookmark(((ParcelablePoint)getIntent().getExtras().getParcelable(BOOKMARK_POSITION)).getPoint());
      mPin.setName(getIntent().getExtras().getString(BOOKMARK_NAME));
      findViewById(R.id.pin_name).requestFocus();
    }
    else if (getIntent().getExtras().containsKey(PIN))
    {
      Point mPinPair = ((ParcelablePoint)getIntent().getParcelableExtra(PIN)).getPoint();
      mCurrentCategoryId = mPinPair.x;
      mPin = mManager.getBookmark(mPinPair.x, mPinPair.y);
    }
    if (mPin == null)
    {
     // mPin = mManager.createNewBookmark();
    }
    else
    {
      mCurrentCategoryId = mPin.getCategoryId();
    }

    setUpViews();
  }

  private void setUpViews()
  {
    (mSpinner = (Spinner) findViewById(R.id.pin_color)).setAdapter(new SpinnerAdapter(this, mManager.getIcons()));
    mSpinner.setOnItemSelectedListener(new OnItemSelectedListener()
    {

      @Override
      public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2, long arg3)
      {
        mPin.setIcon(((SpinnerAdapter) arg0.getAdapter()).getItem(arg2));
      }

      @Override
      public void onNothingSelected(AdapterView<?> arg0)
      {

      }
    });
    int i = 0;
    String type = mPin.getIcon().getType();
    List<Icon> icons = mManager.getIcons();
    for (i = 0; i < icons.size() && !icons.get(i).getType().equals(type); i++)
    {}
    mSpinner.setSelection(i);
    findViewById(R.id.pin_sets).setOnClickListener(new OnClickListener()
    {

      @Override
      public void onClick(View v)
      {
        startActivityForResult(
            new Intent(BookmarkActivity.this, ChooseBookmarkCategoryActivity.class).putExtra(PIN_SET,
                mCurrentCategoryId).putExtra(PIN, new ParcelablePoint(mPin.getCategoryId(), mPin.getBookmarkId())),
                REQUEST_CODE_SET);
      }
    });
    mSetName = (TextView) findViewById(R.id.pin_button_set_name);
    mSetName.setText(mPin.getCategoryName() != null ? mPin.getCategoryName() : getString(android.R.string.unknownName));
    mName = (EditText) findViewById(R.id.pin_name);
    mName.setText("");
    mName.append(mPin.getName());
    mName.addTextChangedListener(new TextWatcher()
    {

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        mPin.setName(s.toString());
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after)
      {
      }

      @Override
      public void afterTextChanged(Editable s)
      {
      }
    });

    EditText descr = (EditText)findViewById(R.id.pin_description);
    descr.setText(mPin.getBookmarkDescription());
    descr.addTextChangedListener(new TextWatcher()
    {

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        mPin.setDescription(s.toString());
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after)
      {
      }

      @Override
      public void afterTextChanged(Editable s)
      {
      }
    });
  }

  @Override
  @Deprecated
  protected Dialog onCreateDialog(int id)
  {
    if (CONFIRMATION_DIALOG == id)
    {
      AlertDialog.Builder builder = new Builder(this);
      builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener()
      {

        @Override
        public void onClick(DialogInterface dialog, int which)
        {
          dialog.dismiss();
        }
      });
      builder.setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener()
      {

        @Override
        public void onClick(DialogInterface dialog, int which)
        {
          mManager.deleteBookmark(mPin);
          dialog.dismiss();
          finish();
        }
      });
      builder.setTitle(R.string.are_you_sure);
      builder.setMessage(getString(R.string.delete)+ " " + mPin.getName() + "?");
      return builder.create();
    }
    else
      return super.onCreateDialog(id);
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == REQUEST_CODE_SET && resultCode == RESULT_OK)
    {
      if (data.getIntExtra(PIN_SET, -1) != mCurrentCategoryId)
      {
        mCurrentCategoryId = data.getIntExtra(PIN_SET, -1);
        BookmarkCategory set = mManager.getCategoryById(mCurrentCategoryId);
        if (set != null)
        {
          mPin.setCategory(set.getName(), mCurrentCategoryId);
          mSetName.setText(set.getName());
        }
      }
    }
    super.onActivityResult(requestCode, resultCode, data);
  }

  public void onDeleteClick(View v)
  {
    showDialog(CONFIRMATION_DIALOG);
  }
}
