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
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.bookmarks.data.ParcelablePointD;

public class BookmarkActivity extends AbstractBookmarkActivity
{
  private static final int CONFIRMATION_DIALOG = 11001;
  private static final int BOOKMARK_COLOR_DIALOG = 11002;

  public static final String BOOKMARK_POSITION = "bookmark_position";
  public static final String PIN = "pin";
  public static final String PIN_ICON_ID = "pin";
  public static final String PIN_SET = "pin_set";
  public static final int REQUEST_CODE_SET = 567890;
  public static final String BOOKMARK_NAME = "bookmark_name";

  private Bookmark mPin;
  private EditText mName;
  private TextView mSetName;
  private int mCurrentCategoryId = -1;
  private List<Icon> mIcons;
  private ImageView mChooserImage;
  private TextView mChooserName;
  private IconsAdapter mIconsAdapter;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.pin);
    if (getIntent().getExtras().containsKey(BOOKMARK_POSITION))
    {
      // create new bookmark
      mPin = mManager.getBookmark(((ParcelablePointD)getIntent().getExtras().getParcelable(BOOKMARK_POSITION)));
      mPin.setName(getIntent().getExtras().getString(BOOKMARK_NAME));
      findViewById(R.id.pin_name).requestFocus();
    }
    else if (getIntent().getExtras().containsKey(PIN))
    {
      Point mPinPair = ((ParcelablePoint)getIntent().getParcelableExtra(PIN)).getPoint();
      mCurrentCategoryId = mPinPair.x;
      mPin = mManager.getBookmark(mPinPair.x, mPinPair.y);
    }
    mCurrentCategoryId = mPin.getCategoryId();
    setUpViews();
  }

  private void updateColorChooser(int position)
  {
    mChooserImage.setImageBitmap(mIcons.get(position).getIcon());
    //mChooserName.setText(mIcons.get(position).getName());
  }

  private void setUpViews()
  {
    View colorChooser = findViewById(R.id.pin_color_chooser);
    mChooserImage = (ImageView)colorChooser.findViewById(R.id.row_color_image);

    mIcons = mManager.getIcons();
    updateColorChooser(mIcons.indexOf(mPin.getIcon()));
    colorChooser.setOnClickListener(new OnClickListener()
    {

      @Override
      public void onClick(View v)
      {
        showDialog(BOOKMARK_COLOR_DIALOG);
      }
    });
    findViewById(R.id.pin_sets).setOnClickListener(new OnClickListener()
    {

      @Override
      public void onClick(View v)
      {
        startActivityForResult(
            new Intent(BookmarkActivity.this, ChooseBookmarkCategoryActivity.class).putExtra(PIN_SET,
                mCurrentCategoryId).
                putExtra(PIN, new ParcelablePoint(mPin.getCategoryId(), mPin.getBookmarkId())), REQUEST_CODE_SET);
      }
    });
    mSetName = (TextView) findViewById(R.id.pin_button_set_name);
    mSetName.setText(mPin.getCategoryName());
    mName = (EditText) findViewById(R.id.pin_name);
    // This hack move cursor to the end of bookmark name
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
      if (id == BOOKMARK_COLOR_DIALOG)
      {
        return createColorChooser();
      }
      else
        return super.onCreateDialog(id);
  }

  private Dialog createColorChooser()
  {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setTitle(R.string.bookmark_color);
    builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener()
    {

      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        updateColorChooser(mIconsAdapter.getCheckedItemPosition());
        dialog.dismiss();
      }
    });
    mIconsAdapter = new IconsAdapter(this, mIcons);
    mIconsAdapter.chooseItem(mIcons.indexOf(mPin.getIcon()));
    builder.setSingleChoiceItems(mIconsAdapter, mIconsAdapter.getCheckedItemPosition(), new DialogInterface.OnClickListener()
    {

      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        mPin.setIcon(mIcons.get(which));
        mIconsAdapter.chooseItem(which);
      }
    });
    return builder.create();
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
