package com.mapswithme.maps.pins;

import com.mapswithme.maps.R;
import com.mapswithme.maps.pins.pins.PinSet;

import android.content.Intent;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView;

public class AbstractPinSetsActivity extends AbstractPinListActivity
{
  private int mSelectedPosition;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
    {
      AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
      mSelectedPosition = info.position;
      MenuInflater inflater = getMenuInflater();
      inflater.inflate(R.menu.pin_sets_context_menu, menu);
    }
    super.onCreateContextMenu(menu, v, menuInfo);
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    PinSet set = ((AbstractPinSetAdapter) getListView().getAdapter()).getItem(mSelectedPosition);
    int itemId = item.getItemId();
    if (itemId == R.id.set_edit)
    {
      startActivity(new Intent(this, PinListActivity.class).putExtra(PinActivity.PIN_SET, mManager.getSetId(set)));
    }
    else if (itemId == R.id.set_delete)
    {
      mManager.deleteSet(set);
      ((AbstractPinSetAdapter) getListView().getAdapter()).remove(set);
      ((AbstractPinSetAdapter) getListView().getAdapter()).notifyDataSetChanged();
    }
    return super.onContextItemSelected(item);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    ((AbstractPinSetAdapter) getListView().getAdapter()).notifyDataSetChanged();
  }
}
