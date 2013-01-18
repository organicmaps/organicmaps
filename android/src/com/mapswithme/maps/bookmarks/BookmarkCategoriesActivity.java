package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.R;

import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;

public class BookmarkCategoriesActivity extends AbstractBookmarkCategoryActivity
{
  private ListView mListView;
  private BookmarkCategoriesAdapter mAdapter;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mListView = getListView();
    mListView.setAdapter(mAdapter = new BookmarkCategoriesAdapter(this));
    mListView.setOnItemClickListener(new OnItemClickListener()
    {

      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        startActivity(new Intent(BookmarkCategoriesActivity.this, BookmarkListActivity.class).putExtra(BookmarkActivity.PIN_SET,
            position));
      }
    });
    registerForContextMenu(getListView());
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    getMenuInflater().inflate(R.menu.activity_pin_sets, menu);
    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if(item.getItemId() == R.id.pinsets_add){
      //TODO create new set
    }
    return super.onOptionsItemSelected(item);
  }
}
