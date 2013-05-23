
package org.holoeverywhere.app;

import org.holoeverywhere.R;
import org.holoeverywhere.widget.ListView;

import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListAdapter;

public abstract class ListActivity extends Activity {
    protected ListAdapter mAdapter;
    private boolean mFinishedStart = false;

    private Handler mHandler = new Handler();
    protected ListView mList;

    private AdapterView.OnItemClickListener mOnClickListener = new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View v, int position,
                long id) {
            onListItemClick((ListView) parent, v, position, id);
        }
    };

    private Runnable mRequestFocus = new Runnable() {
        @Override
        public void run() {
            mList.focusableViewAvailable(mList);
        }
    };

    private void ensureList() {
        if (mList != null) {
            return;
        }
        setContentView(R.layout.list_content);
    }

    public ListAdapter getListAdapter() {
        return mAdapter;
    }

    public ListView getListView() {
        ensureList();
        return mList;
    }

    public long getSelectedItemId() {
        return mList.getSelectedItemId();
    }

    public int getSelectedItemPosition() {
        return mList.getSelectedItemPosition();
    }

    @Override
    public void onContentChanged() {
        super.onContentChanged();
        View emptyView = findViewById(android.R.id.empty);
        mList = (ListView) findViewById(android.R.id.list);
        if (mList == null) {
            throw new RuntimeException(
                    "Your content must have a ListView whose id attribute is "
                            + "'android.R.id.list'");
        }
        if (emptyView != null) {
            mList.setEmptyView(emptyView);
        }
        mList.setOnItemClickListener(mOnClickListener);
        if (mFinishedStart) {
            setListAdapter(mAdapter);
        }
        mHandler.post(mRequestFocus);
        mFinishedStart = true;
    }

    @Override
    protected void onDestroy() {
        mHandler.removeCallbacks(mRequestFocus);
        super.onDestroy();
    }

    protected void onListItemClick(ListView l, View v, int position, long id) {
    }

    @Override
    protected void onRestoreInstanceState(Bundle state) {
        ensureList();
        super.onRestoreInstanceState(state);
    }

    public void setListAdapter(ListAdapter adapter) {
        synchronized (this) {
            ensureList();
            mAdapter = adapter;
            mList.setAdapter(adapter);
        }
    }

    public void setSelection(int position) {
        mList.setSelection(position);
    }
}
