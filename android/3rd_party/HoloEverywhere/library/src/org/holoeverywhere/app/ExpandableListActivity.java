
package org.holoeverywhere.app;

import org.holoeverywhere.R;
import org.holoeverywhere.widget.ExpandableListView;

import android.os.Bundle;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.widget.ExpandableListAdapter;

public abstract class ExpandableListActivity extends Activity implements
        OnCreateContextMenuListener, ExpandableListView.OnChildClickListener,
        ExpandableListView.OnGroupCollapseListener,
        ExpandableListView.OnGroupExpandListener {
    ExpandableListAdapter mAdapter;
    boolean mFinishedStart = false;
    ExpandableListView mList;

    private void ensureList() {
        if (mList != null) {
            return;
        }
        setContentView(R.layout.expandable_list_content);
    }

    public ExpandableListAdapter getExpandableListAdapter() {
        return mAdapter;
    }

    public ExpandableListView getExpandableListView() {
        ensureList();
        return mList;
    }

    public long getSelectedId() {
        return mList.getSelectedId();
    }

    public long getSelectedPosition() {
        return mList.getSelectedPosition();
    }

    @Override
    public boolean onChildClick(ExpandableListView parent, View v,
            int groupPosition, int childPosition, long id) {
        return false;
    }

    @Override
    public void onContentChanged() {
        super.onContentChanged();
        View emptyView = findViewById(R.id.empty);
        mList = (ExpandableListView) findViewById(android.R.id.list);
        if (mList == null) {
            throw new RuntimeException(
                    "Your content must have a ExpandableListView whose id attribute is "
                            + "'android.R.id.list'");
        }
        if (emptyView != null) {
            mList.setEmptyView(emptyView);
        }
        mList.setOnChildClickListener(this);
        mList.setOnGroupExpandListener(this);
        mList.setOnGroupCollapseListener(this);

        if (mFinishedStart) {
            setListAdapter(mAdapter);
        }
        mFinishedStart = true;
    }

    @Override
    public void onGroupCollapse(int groupPosition) {
    }

    @Override
    public void onGroupExpand(int groupPosition) {
    }

    @Override
    protected void onRestoreInstanceState(Bundle state) {
        ensureList();
        super.onRestoreInstanceState(state);
    }

    public void setListAdapter(ExpandableListAdapter adapter) {
        synchronized (this) {
            ensureList();
            mAdapter = adapter;
            mList.setAdapter(adapter);
        }
    }

    public boolean setSelectedChild(int groupPosition, int childPosition,
            boolean shouldExpandGroup) {
        return mList.setSelectedChild(groupPosition, childPosition,
                shouldExpandGroup);
    }

    public void setSelectedGroup(int groupPosition) {
        mList.setSelectedGroup(groupPosition);
    }
}
