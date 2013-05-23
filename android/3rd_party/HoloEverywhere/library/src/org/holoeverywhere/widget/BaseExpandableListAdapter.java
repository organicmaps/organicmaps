
package org.holoeverywhere.widget;

import android.database.DataSetObservable;
import android.database.DataSetObserver;
import android.widget.ExpandableListAdapter;

public abstract class BaseExpandableListAdapter implements ExpandableListAdapter,
        HeterogeneousExpandableList {
    private final DataSetObservable mDataSetObservable = new DataSetObservable();

    @Override
    public boolean areAllItemsEnabled() {
        return true;
    }

    @Override
    public int getChildType(int groupPosition, int childPosition) {
        return 0;
    }

    @Override
    public int getChildTypeCount() {
        return 1;
    }

    @Override
    public long getCombinedChildId(long groupId, long childId) {
        return 0x8000000000000000L | (groupId & 0x7FFFFFFF) << 32 | childId & 0xFFFFFFFF;
    }

    @Override
    public long getCombinedGroupId(long groupId) {
        return (groupId & 0x7FFFFFFF) << 32;
    }

    @Override
    public int getGroupType(int groupPosition) {
        return 0;
    }

    @Override
    public int getGroupTypeCount() {
        return 1;
    }

    @Override
    public boolean isEmpty() {
        return getGroupCount() == 0;
    }

    public void notifyDataSetChanged() {
        mDataSetObservable.notifyChanged();
    }

    public void notifyDataSetInvalidated() {
        mDataSetObservable.notifyInvalidated();
    }

    @Override
    public void onGroupCollapsed(int groupPosition) {
    }

    @Override
    public void onGroupExpanded(int groupPosition) {
    }

    @Override
    public void registerDataSetObserver(DataSetObserver observer) {
        mDataSetObservable.registerObserver(observer);
    }

    @Override
    public void unregisterDataSetObserver(DataSetObserver observer) {
        mDataSetObservable.unregisterObserver(observer);
    }
}
