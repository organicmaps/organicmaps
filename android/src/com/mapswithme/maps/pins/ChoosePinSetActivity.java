package com.mapswithme.maps.pins;

import com.mapswithme.maps.R;
import com.mapswithme.maps.pins.pins.PinManager;

import android.os.Bundle;
import android.content.Intent;
import android.view.Menu;
import android.widget.ListView;

public class ChoosePinSetActivity extends AbstractPinSetsActivity
{
  private ChoosePinSetAdapter mAdapter;
  private ListView mList;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_pin_sets);
    mList = (ListView) findViewById(android.R.id.list);
    mList.setAdapter(mAdapter = new ChoosePinSetAdapter(this, PinManager.getPinManager(getApplicationContext())
        .getPinSets(), getIntent().getIntExtra(PinActivity.PIN_SET, -1)));
    int checked = getIntent().getIntExtra(PinActivity.PIN_SET, -1);
    mList.setItemChecked(checked, true);
    mList.setSelection(checked);
    mList.setOnItemClickListener(mAdapter);
    registerForContextMenu(getListView());
  }

  @Override
  public void onBackPressed()
  {
    setResult(RESULT_OK, new Intent().putExtra(PinActivity.PIN_SET, mList.getCheckedItemPosition()));
    super.onBackPressed();
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    getMenuInflater().inflate(R.menu.activity_pin_sets, menu);
    return true;
  }
}
