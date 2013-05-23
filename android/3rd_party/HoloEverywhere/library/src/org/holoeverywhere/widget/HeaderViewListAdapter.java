
package org.holoeverywhere.widget;

import java.util.ArrayList;
import java.util.List;

import android.view.View;
import android.view.ViewGroup;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.ListAdapter;

public class HeaderViewListAdapter extends ListAdapterWrapper implements Filterable {
    public static final class ViewInfo {
        public Object data;
        public boolean isSelectable;
        public View view;
    }

    private static final List<ViewInfo> EMPTY_INFO_LIST = new ArrayList<ViewInfo>();
    boolean mAreAllFixedViewsSelectable;
    private final List<ViewInfo> mFooterViewInfos;
    private final List<ViewInfo> mHeaderViewInfos;
    private final boolean mIsFilterable;

    public HeaderViewListAdapter(List<ViewInfo> headerViewInfos,
            List<ViewInfo> footerViewInfos, ListAdapter adapter, ListAdapterCallback listener) {
        super(adapter, listener);
        mIsFilterable = adapter instanceof Filterable;
        if (headerViewInfos == null) {
            mHeaderViewInfos = EMPTY_INFO_LIST;
        } else {
            mHeaderViewInfos = headerViewInfos;
        }
        if (footerViewInfos == null) {
            mFooterViewInfos = EMPTY_INFO_LIST;
        } else {
            mFooterViewInfos = footerViewInfos;
        }
        mAreAllFixedViewsSelectable =
                areAllListInfosSelectable(mHeaderViewInfos)
                        && areAllListInfosSelectable(mFooterViewInfos);
    }

    @Override
    public boolean areAllItemsEnabled() {
        return mAreAllFixedViewsSelectable && getWrappedAdapter().areAllItemsEnabled();
    }

    private boolean areAllListInfosSelectable(List<ViewInfo> infos) {
        if (infos != null) {
            for (ViewInfo info : infos) {
                if (!info.isSelectable) {
                    return false;
                }
            }
        }
        return true;
    }

    @Override
    public int getCount() {
        return getFootersCount() + getHeadersCount() + getWrappedAdapter().getCount();
    }

    @Override
    public Filter getFilter() {
        if (mIsFilterable) {
            return ((Filterable) getWrappedAdapter()).getFilter();
        }
        return null;
    }

    public int getFootersCount() {
        return mFooterViewInfos.size();
    }

    public int getHeadersCount() {
        return mHeaderViewInfos.size();
    }

    @Override
    public Object getItem(int position) {
        int numHeaders = getHeadersCount();
        if (position < numHeaders) {
            return mHeaderViewInfos.get(position).data;
        }
        final int adjPosition = position - numHeaders;
        int adapterCount = getWrappedAdapter().getCount();
        if (adjPosition < adapterCount) {
            return super.getItem(adjPosition);
        }
        return mFooterViewInfos.get(adjPosition - adapterCount).data;
    }

    @Override
    public long getItemId(int position) {
        int numHeaders = getHeadersCount();
        if (position >= numHeaders) {
            int adjPosition = position - numHeaders;
            int adapterCount = getWrappedAdapter().getCount();
            if (adjPosition < adapterCount) {
                return super.getItemId(adjPosition);
            }
        }
        return AdapterView.INVALID_ROW_ID;
    }

    @Override
    public int getItemViewType(int position) {
        int numHeaders = getHeadersCount();
        if (getWrappedAdapter() != null && position >= numHeaders) {
            int adjPosition = position - numHeaders;
            int adapterCount = getWrappedAdapter().getCount();
            if (adjPosition < adapterCount) {
                return super.getItemViewType(adjPosition);
            }
        }
        return AdapterView.ITEM_VIEW_TYPE_HEADER_OR_FOOTER;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        int numHeaders = getHeadersCount();
        if (position < numHeaders) {
            convertView = mHeaderViewInfos.get(position).view;
        } else {
            final int adjPosition = position - numHeaders;
            int adapterCount = getWrappedAdapter().getCount();
            if (adjPosition < adapterCount) {
                convertView = getWrappedAdapter().getView(adjPosition, convertView, parent);
            } else {
                convertView = mFooterViewInfos.get(adjPosition - adapterCount).view;
            }
        }
        return onPrepareView(convertView, position);
    }

    @Override
    public boolean isEnabled(int position) {
        int numHeaders = getHeadersCount();
        if (position < numHeaders) {
            return mHeaderViewInfos.get(position).isSelectable;
        }
        final int adjPosition = position - numHeaders;
        int adapterCount = getWrappedAdapter().getCount();
        if (adjPosition < adapterCount) {
            return super.isEnabled(adjPosition);
        }
        return mFooterViewInfos.get(adjPosition - adapterCount).isSelectable;
    }

    public boolean removeFooter(View v) {
        for (int i = 0; i < mFooterViewInfos.size(); i++) {
            ViewInfo info = mFooterViewInfos.get(i);
            if (info.view == v) {
                mFooterViewInfos.remove(i);
                mAreAllFixedViewsSelectable =
                        areAllListInfosSelectable(mHeaderViewInfos)
                                && areAllListInfosSelectable(mFooterViewInfos);
                return true;
            }
        }
        return false;
    }

    public boolean removeHeader(View v) {
        for (int i = 0; i < mHeaderViewInfos.size(); i++) {
            ViewInfo info = mHeaderViewInfos.get(i);
            if (info.view == v) {
                mHeaderViewInfos.remove(i);
                mAreAllFixedViewsSelectable =
                        areAllListInfosSelectable(mHeaderViewInfos)
                                && areAllListInfosSelectable(mFooterViewInfos);
                return true;
            }
        }
        return false;
    }
}
