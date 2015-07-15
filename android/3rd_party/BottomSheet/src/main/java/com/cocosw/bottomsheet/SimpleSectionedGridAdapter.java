package com.cocosw.bottomsheet;


import java.util.Arrays;
import java.util.Comparator;

import android.content.Context;
import android.database.DataSetObserver;
import android.text.TextUtils;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

class SimpleSectionedGridAdapter extends BaseAdapter{
    protected static final int TYPE_FILLER = 0;
    protected static final int TYPE_HEADER = 1;
    protected static final int TYPE_HEADER_FILLER = 2;
    private boolean mValid = true;
    private int mSectionResourceId;
    private LayoutInflater mLayoutInflater;
    private ListAdapter mBaseAdapter;
    SparseArray<Section> mSections = new SparseArray<Section>();
    private Section[] mInitialSections = new Section[0];
    private Context mContext;
    private View mLastViewSeen;
    private int mHeaderWidth;
    private int mNumColumns;
    private int mWidth;
    private int mColumnWidth;
    private int mHorizontalSpacing;
    private int mStrechMode;
    private int requestedColumnWidth;
    private int requestedHorizontalSpacing;
    private GridView mGridView;
    private int mHeaderLayoutResId;
    private int mHeaderTextViewResId;

    public static class Section {
        int firstPosition;
        int sectionedPosition;
        CharSequence title;
        int type = 0;

        public Section(int firstPosition, CharSequence title) {
            this.firstPosition = firstPosition;
            this.title =title;
        }

        public CharSequence getTitle() {
            return title;
        }
    }

    public SimpleSectionedGridAdapter(Context context, BaseAdapter baseAdapter, int sectionResourceId, int headerLayoutResId,
                                      int headerTextViewResId) {
        mLayoutInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mSectionResourceId = sectionResourceId;
        mHeaderLayoutResId = headerLayoutResId;
        mHeaderTextViewResId = headerTextViewResId;
        mBaseAdapter = baseAdapter;
        mContext = context;
        mBaseAdapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onChanged() {
                mValid = !mBaseAdapter.isEmpty();
                notifyDataSetChanged();
            }

            @Override
            public void onInvalidated() {
                mValid = false;
                notifyDataSetInvalidated();
            }
        });
    }

    public void setGridView(GridView gridView){
        if(!(gridView instanceof PinnedSectionGridView)){
            throw new IllegalArgumentException("Does your grid view extends PinnedSectionGridView?");
        }
        mGridView = gridView;
        mStrechMode = gridView.getStretchMode();
        mWidth = gridView.getWidth() - (mGridView.getPaddingLeft() + mGridView.getPaddingRight());
        mNumColumns = ((PinnedSectionGridView)gridView).getNumColumns();
        requestedColumnWidth = ((PinnedSectionGridView)gridView).getColumnWidth();
        requestedHorizontalSpacing = ((PinnedSectionGridView)gridView).getHorizontalSpacing();
    }

    private int getHeaderSize(){
        if(mHeaderWidth > 0){
            return mHeaderWidth;
        }
        if(mWidth != mGridView.getWidth()){
            mStrechMode = mGridView.getStretchMode();
            mWidth = ((PinnedSectionGridView)mGridView).getAvailableWidth() - (mGridView.getPaddingLeft() + mGridView.getPaddingRight());
            mNumColumns = ((PinnedSectionGridView)mGridView).getNumColumns();
            requestedColumnWidth = ((PinnedSectionGridView)mGridView).getColumnWidth();
            requestedHorizontalSpacing = ((PinnedSectionGridView)mGridView).getHorizontalSpacing();
        }

        int spaceLeftOver = mWidth - (mNumColumns * requestedColumnWidth) -
                ((mNumColumns - 1) * requestedHorizontalSpacing);
        switch (mStrechMode) {
            case GridView.NO_STRETCH:            // Nobody stretches
                mWidth -= spaceLeftOver;
                mColumnWidth = requestedColumnWidth;
                mHorizontalSpacing = requestedHorizontalSpacing;
                break;

            case GridView.STRETCH_COLUMN_WIDTH:
                mColumnWidth = requestedColumnWidth + spaceLeftOver / mNumColumns;
                mHorizontalSpacing = requestedHorizontalSpacing;
                break;

            case GridView.STRETCH_SPACING:
                mColumnWidth = requestedColumnWidth;
                if (mNumColumns > 1) {
                    mHorizontalSpacing = requestedHorizontalSpacing +
                            spaceLeftOver / (mNumColumns - 1);
                } else {
                    mHorizontalSpacing = requestedHorizontalSpacing + spaceLeftOver;
                }
                break;

            case GridView.STRETCH_SPACING_UNIFORM:
                mColumnWidth = requestedColumnWidth;
                mHorizontalSpacing = requestedHorizontalSpacing;
                mWidth = mWidth - spaceLeftOver + (2 * mHorizontalSpacing);
                break;
        }
        mHeaderWidth = mWidth + ((mNumColumns - 1) * (mColumnWidth + mHorizontalSpacing)) ;
        return mHeaderWidth;
    }


    public void setSections(Section... sections) {
        mInitialSections = sections;
        setSections();
    }

    public void setSections() {
        mSections.clear();

        getHeaderSize();
        Arrays.sort(mInitialSections, new Comparator<Section>() {
            @Override
            public int compare(Section o, Section o1) {
                return (o.firstPosition == o1.firstPosition)
                        ? 0
                        : ((o.firstPosition < o1.firstPosition) ? -1 : 1);
            }
        });

        int offset = 0; // offset positions for the headers we're adding
        for (int i = 0; i < mInitialSections.length; i++) {
            Section section = mInitialSections[i];
            Section sectionAdd;

            for (int j = 0; j < mNumColumns - 1; j++) {
                sectionAdd = new Section(section.firstPosition, section.title);
                sectionAdd.type = TYPE_HEADER_FILLER;
                sectionAdd.sectionedPosition = sectionAdd.firstPosition + offset;
                mSections.append(sectionAdd.sectionedPosition, sectionAdd);
                ++offset;
            }

            sectionAdd = new Section(section.firstPosition, section.title);
            sectionAdd.type = TYPE_HEADER;
            sectionAdd.sectionedPosition = sectionAdd.firstPosition + offset;
            mSections.append(sectionAdd.sectionedPosition, sectionAdd);
            ++offset;

            if(i < mInitialSections.length - 1){
                int nextPos = mInitialSections[i+1].firstPosition;
                int itemsCount = nextPos - section.firstPosition;
                int dummyCount = mNumColumns - (itemsCount % mNumColumns);
                if(mNumColumns != dummyCount){
                    for (int k = 0 ;k < dummyCount; k++) {
                        sectionAdd = new Section(section.firstPosition, section.title);
                        sectionAdd.type = TYPE_FILLER;
                        sectionAdd.sectionedPosition = nextPos + offset;
                        mSections.append(sectionAdd.sectionedPosition, sectionAdd);
                        ++offset;
                    }
                }
            }
        }

        notifyDataSetChanged();
    }

    public int positionToSectionedPosition(int position) {
        int offset = 0;
        for (int i = 0; i < mSections.size(); i++) {
            if (mSections.valueAt(i).firstPosition > position) {
                break;
            }
            ++offset;
        }
        return position + offset;
    }

    public int sectionedPositionToPosition(int sectionedPosition) {
        if (isSectionHeaderPosition(sectionedPosition)) {
            return ListView.INVALID_POSITION;
        }

        int offset = 0;
        for (int i = 0; i < mSections.size(); i++) {
            if (mSections.valueAt(i).sectionedPosition > sectionedPosition) {
                break;
            }
            --offset;
        }
        return sectionedPosition + offset;
    }

    public boolean isSectionHeaderPosition(int position) {
        return mSections.get(position) != null;
    }

    @Override
    public int getCount() {
        return (mValid ? mBaseAdapter.getCount() + mSections.size() : 0);
    }

    @Override
    public Object getItem(int position) {
        return isSectionHeaderPosition(position)
                ? mSections.get(position)
                : mBaseAdapter.getItem(sectionedPositionToPosition(position));
    }

    @Override
    public long getItemId(int position) {
        return isSectionHeaderPosition(position)
                ? Integer.MAX_VALUE - mSections.indexOfKey(position)
                : mBaseAdapter.getItemId(sectionedPositionToPosition(position));
    }

    @Override
    public int getItemViewType(int position) {
        return isSectionHeaderPosition(position)
                ? getViewTypeCount() - 1
                : mBaseAdapter.getItemViewType(sectionedPositionToPosition(position));
    }

    @Override
    public boolean isEnabled(int position) {
        //noinspection SimplifiableConditionalExpression
        return isSectionHeaderPosition(position)
                ? false
                : mBaseAdapter.isEnabled(sectionedPositionToPosition(position));
    }

    @Override
    public int getViewTypeCount() {
        return mBaseAdapter.getViewTypeCount() + 1; // the section headings
    }

    @Override
    public boolean areAllItemsEnabled() {
        return mBaseAdapter.areAllItemsEnabled();
    }

    @Override
    public boolean hasStableIds() {
        return mBaseAdapter.hasStableIds();
    }

    @Override
    public boolean isEmpty() {
        return mBaseAdapter.isEmpty();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (isSectionHeaderPosition(position)) {
            HeaderLayout header;
            TextView view;
            if (null == convertView) {
                convertView = mLayoutInflater.inflate(mSectionResourceId, parent, false);
            } else {
                if (null == convertView.findViewById(mHeaderLayoutResId)) {
                    convertView = mLayoutInflater.inflate(mSectionResourceId, parent, false);
                }
            }
            switch (mSections.get(position).type) {
                case TYPE_HEADER:
                    header = (HeaderLayout) convertView.findViewById(mHeaderLayoutResId);
                    if (!TextUtils.isEmpty(mSections.get(position).title)) {
                        view = (TextView) convertView.findViewById(mHeaderTextViewResId);
                        view.setText(mSections.get(position).title);
                    }
                    header.setHeaderWidth(getHeaderSize());
                    break;
                case TYPE_HEADER_FILLER:
                    header = (HeaderLayout) convertView.findViewById(mHeaderLayoutResId);
                    if (!TextUtils.isEmpty(mSections.get(position).title)) {
                        view = (TextView) convertView.findViewById(mHeaderTextViewResId);
                        view.setText(mSections.get(position).title);
                    }
                    header.setHeaderWidth(0);
                    break;
                default:
                    convertView = getFillerView(mLastViewSeen);
            }
        } else {
            convertView = mBaseAdapter.getView(sectionedPositionToPosition(position), convertView, parent);
            mLastViewSeen = convertView;
        }
        return convertView;
    }

    private FillerView getFillerView(final View lastViewSeen) {
        final FillerView fillerView = new FillerView(mContext);
        fillerView.setMeasureTarget(lastViewSeen);
        return fillerView;
    }

    public int getHeaderLayoutResId() {
        return mHeaderLayoutResId;
    }

    public static class ViewHolder {
        @SuppressWarnings("unchecked")
        public static <T extends View> T get(View view, int id) {
            SparseArray<View> viewHolder = (SparseArray<View>) view.getTag();
            if (viewHolder == null) {
                viewHolder = new SparseArray<View>();
                view.setTag(viewHolder);
            }
            View childView = viewHolder.get(id);
            if (childView == null) {
                childView = view.findViewById(id);
                viewHolder.put(id, childView);
            }
            return (T) childView;
        }
    }

}