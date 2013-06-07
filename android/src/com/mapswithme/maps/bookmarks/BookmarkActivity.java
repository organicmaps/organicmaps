package com.mapswithme.maps.bookmarks;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Point;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.api.MWMRequest;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.Icon;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.state.SuppotedState;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.Utils;

import java.util.List;

public class BookmarkActivity extends AbstractBookmarkActivity
{
  private static final int BOOKMARK_COLOR_DIALOG = 11002;

  public static final String BOOKMARK_POSITION = "bookmark_position";
  public static final String PIN = "pin";
  public static final String FROM_MAP = "from_map";
  public static final String PIN_ICON_ID = "pin";
  public static final String PIN_SET = "pin_set";
  public static final int REQUEST_CODE_SET = 567890;
  public static final String BOOKMARK_NAME = "bookmark_name";

  private boolean mStartedByMap;
  private Bookmark mPin;
  private EditText mName;
  private TextView mSetName;
  private int mCurrentCategoryId = -1;
  private List<Icon> mIcons;
  private ImageView mChooserImage;
  private EditText mDescr;
  private Icon mIcon = null;

  // API
  private Button mOpenWithAppBtn;
  
  
  public static void startWithBookmark(Context context, int category, int bookmark)
  {
    context.startActivity(new Intent(context, BookmarkActivity.class)
    .putExtra(BookmarkActivity.PIN, new ParcelablePoint(category, bookmark))
    .putExtra(BookmarkActivity.FROM_MAP, true));
  }

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
    mStartedByMap = getIntent().getBooleanExtra(FROM_MAP, false);

    setTitle(mPin.getName());
    setUpViews();
    
    adaptUiForOsVersion();
  }

  private void adaptUiForOsVersion()
  {
    ViewGroup compatBtns = (ViewGroup) findViewById(R.id.compat_btns_bar);
    if (Utils.apiLowerThan(11))
    {
      
      Button shareEmailBtn = (Button) compatBtns.findViewById(R.id.btn_share_email);
      if (ShareAction.getEmailShare().isSupported(this))
      {
        shareEmailBtn.setOnClickListener(new OnClickListener()
        {
          @Override
          public void onClick(View v)
          {
             assignPinParams();
             ShareAction.getEmailShare().shareBookmark(BookmarkActivity.this, mPin);
          }
        });
      } 
      else 
      {
        shareEmailBtn.setVisibility(View.GONE);
      }
      
      Button shareAnyBtn = (Button) compatBtns.findViewById(R.id.btn_share_any);
      if (ShareAction.getAnyShare().isSupported(this))
      {
        shareAnyBtn.setOnClickListener(new OnClickListener()
        {
          @Override
          public void onClick(View v)
          {
            assignPinParams();
            ShareAction.getAnyShare().shareBookmark(BookmarkActivity.this, mPin);
          }
        });
      }
      else 
      {
        shareAnyBtn.setVisibility(View.GONE);
      }
      
    }
    else 
    {
      compatBtns.setVisibility(View.GONE);
    }
  }

  private void updateColorChooser(Icon icon)
  {
    mIcon = icon;
    mChooserImage.setImageBitmap(mIcon.getIcon());
  }

  private void refreshValuesInViews()
  {
    updateColorChooser(mPin.getIcon());

    mSetName.setText(mPin.getCategoryName());

    Utils.setStringAndCursorToEnd(mName, mPin.getName());

    mDescr.setText(mPin.getBookmarkDescription());
  }

  private void setUpViews()
  {
    View colorChooser = findViewById(R.id.pin_color_chooser);
    mChooserImage = (ImageView)colorChooser.findViewById(R.id.row_color_image);
    mOpenWithAppBtn = (Button) findViewById(R.id.btn_get_this_point);
    mIcons = mManager.getIcons();

    colorChooser.setOnClickListener(new OnClickListener()
    {
      @SuppressWarnings("deprecation")
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
        startActivityForResult(new Intent(BookmarkActivity.this,
                                          ChooseBookmarkCategoryActivity.class)
        .putExtra(PIN_SET, mCurrentCategoryId)
        .putExtra(PIN, new ParcelablePoint(mPin.getCategoryId(), mPin.getBookmarkId())), REQUEST_CODE_SET);
      }
    });

    mSetName = (TextView) findViewById(R.id.pin_button_set_name);
    mName = (EditText) findViewById(R.id.pin_name);
    mDescr = (EditText)findViewById(R.id.pin_description);

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
      public void beforeTextChanged(CharSequence s, int start, int count, int after)
      {
      }
      @Override
      public void afterTextChanged(Editable s)
      {
      }
    });
  }

  private void assignPinParams()
  {
    if (mPin != null)
      mPin.setParams(mName.getText().toString(), mIcon, mDescr.getText().toString());
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

    return new AlertDialog.Builder(this)
    .setTitle(R.string.bookmark_color)
    .setSingleChoiceItems(adapter, adapter.getCheckedItemPosition(), new DialogInterface.OnClickListener()
    {
      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        updateColorChooser(mIcons.get(which));
        dialog.dismiss();
      }
    })
    .create();
  }

  @Override
  public void setViewFromState(SuppotedState state)
  {
    // TODO we need to differ if activity opened for api point
    final MWMRequest request = MWMRequest.getCurrentRequest();
    if (state == SuppotedState.API_REQUEST
        && request.hasPendingIntent()
        && request.hasPoint()
        && mStartedByMap)
    {
      // TODO add to resources
      final String pattern = "Open with %s";
      final String text = String.format(pattern, request.getCallerName(this));
      final Drawable icon = request.getIcon(this);
      final int iconSize = (int) getResources().getDimension(R.dimen.icon_size);
      icon.setBounds(0, 0, iconSize, iconSize);
      mOpenWithAppBtn.setCompoundDrawables(icon, null, null, null);
      mOpenWithAppBtn.setText(text);
      mOpenWithAppBtn.setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          if (MWMRequest.getCurrentRequest().sendResponse(getApplicationContext(), true))
          {
            finish();
            getMwmApplication().getAppStateManager().transitionTo(SuppotedState.DEFAULT_MAP);
          }
        }
      });
      mOpenWithAppBtn.setVisibility(View.VISIBLE);
    }
    else
      mOpenWithAppBtn.setVisibility(View.GONE);
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == REQUEST_CODE_SET && resultCode == RESULT_OK)
    {
      Point pin = ((ParcelablePoint)data.getParcelableExtra(PIN)).getPoint();
      mPin = mManager.getBookmark(pin.x, pin.y);
      refreshValuesInViews();
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

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    MenuItem menuItem = ShareAction.getAnyShare().addToMenuIfSupported(this, menu, true);
    if (menuItem != null)
      menuItem.setIcon(android.R.drawable.ic_menu_share);
    
    menuItem = ShareAction.getEmailShare().addToMenuIfSupported(this, menu, true);
    if (menuItem != null)
      menuItem.setIcon(android.R.drawable.ic_menu_send);

    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (ShareAction.ACTIONS.containsKey(item.getItemId()))
    {
      assignPinParams();

      final ShareAction shareAction = ShareAction.ACTIONS.get(item.getItemId());
      shareAction.shareBookmark(this, mPin);
      
      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }
}
