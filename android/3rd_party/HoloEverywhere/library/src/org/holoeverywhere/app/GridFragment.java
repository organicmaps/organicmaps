
package org.holoeverywhere.app;

import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.widget.GridView;

import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.widget.AdapterView;
import android.widget.ListAdapter;
import android.widget.TextView;

public class GridFragment extends Fragment {
    private ListAdapter mAdapter;
    private CharSequence mEmptyText;
    private View mEmptyView;
    private GridView mGrid;
    private View mGridContainer;
    private boolean mGridShown;
    final private Handler mHandler = new Handler();
    final private AdapterView.OnItemClickListener mOnClickListener = new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View v, int position,
                long id) {
            onGridItemClick((GridView) parent, v, position, id);
        }
    };
    private View mProgressContainer;
    final private Runnable mRequestFocus = new Runnable() {
        @Override
        public void run() {
            mGrid.focusableViewAvailable(mGrid);
        }
    };
    private TextView mStandardEmptyView;

    private void ensureGrid() {
        if (mGrid != null) {
            return;
        }
        View root = getView();
        if (root == null) {
            throw new IllegalStateException("Content view not yet created");
        }
        if (root instanceof GridView) {
            mGrid = (GridView) root;
        } else {
            mStandardEmptyView = (TextView) root
                    .findViewById(R.id.internalEmpty);
            if (mStandardEmptyView == null) {
                mEmptyView = root.findViewById(android.R.id.empty);
            } else {
                mStandardEmptyView.setVisibility(View.GONE);
            }
            mProgressContainer = root.findViewById(R.id.progressContainer);
            mGridContainer = root.findViewById(R.id.listContainer);
            View rawGridVIew = root.findViewById(android.R.id.list);
            if (!(rawGridVIew instanceof GridView)) {
                if (rawGridVIew == null) {
                    throw new RuntimeException(
                            "Your content must have a GridVIew whose id attribute is "
                                    + "'android.R.id.list'");
                }
                throw new RuntimeException(
                        "Content has view with id attribute 'android.R.id.list' "
                                + "that is not a GridVIew class");
            }
            mGrid = (GridView) rawGridVIew;
            if (mEmptyView != null) {
                mGrid.setEmptyView(mEmptyView);
            } else if (mEmptyText != null) {
                mStandardEmptyView.setText(mEmptyText);
                mGrid.setEmptyView(mStandardEmptyView);
            }
        }
        mGridShown = true;
        mGrid.setOnItemClickListener(mOnClickListener);
        if (mAdapter != null) {
            ListAdapter adapter = mAdapter;
            mAdapter = null;
            setGridAdapter(adapter);
        } else {
            if (mProgressContainer != null) {
                setGridShown(false, false);
            }
        }
        mHandler.post(mRequestFocus);
    }

    protected View getEmptyView() {
        return mEmptyView;
    }

    public GridView getGridView() {
        ensureGrid();
        return mGrid;
    }

    public ListAdapter getListAdapter() {
        return mAdapter;
    }

    public long getSelectedItemId() {
        ensureGrid();
        return mGrid.getSelectedItemId();
    }

    public int getSelectedItemPosition() {
        ensureGrid();
        return mGrid.getSelectedItemPosition();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.grid_content, container, false);
    }

    @Override
    public void onDestroyView() {
        mHandler.removeCallbacks(mRequestFocus);
        mGrid = null;
        mGridShown = false;
        mEmptyView = mProgressContainer = mGridContainer = null;
        mStandardEmptyView = null;
        super.onDestroyView();
    }

    public void onGridItemClick(GridView l, View v, int position, long id) {
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        ensureGrid();
    }

    public void setEmptyText(CharSequence text) {
        ensureGrid();
        if (mStandardEmptyView == null) {
            throw new IllegalStateException(
                    "Can't be used with a custom content view");
        }
        mStandardEmptyView.setText(text);
        if (mEmptyText == null) {
            mGrid.setEmptyView(mStandardEmptyView);
        }
        mEmptyText = text;
    }

    public void setGridAdapter(ListAdapter adapter) {
        boolean hadAdapter = mAdapter != null;
        mAdapter = adapter;
        if (mGrid != null) {
            mGrid.setAdapter(adapter);
            if (!mGridShown && !hadAdapter) {
                // The list was hidden, and previously didn't have an
                // adapter. It is now time to show it.
                setGridShown(true, getView().getWindowToken() != null);
            }
        }
    }

    public void setGridShown(boolean shown) {
        setGridShown(shown, true);
    }

    private void setGridShown(boolean shown, boolean animate) {
        ensureGrid();
        if (mProgressContainer == null) {
            throw new IllegalStateException(
                    "Can't be used with a custom content view");
        }
        if (mGridShown == shown) {
            return;
        }
        mGridShown = shown;
        if (shown) {
            if (animate) {
                mProgressContainer.startAnimation(AnimationUtils.loadAnimation(
                        getActivity(), R.anim.fade_out));
                mGridContainer.startAnimation(AnimationUtils.loadAnimation(
                        getActivity(), R.anim.fade_in));
            } else {
                mProgressContainer.clearAnimation();
                mGridContainer.clearAnimation();
            }
            mProgressContainer.setVisibility(View.GONE);
            mGridContainer.setVisibility(View.VISIBLE);
        } else {
            if (animate) {
                mProgressContainer.startAnimation(AnimationUtils.loadAnimation(
                        getActivity(), R.anim.fade_in));
                mGridContainer.startAnimation(AnimationUtils.loadAnimation(
                        getActivity(), R.anim.fade_out));
            } else {
                mProgressContainer.clearAnimation();
                mGridContainer.clearAnimation();
            }
            mProgressContainer.setVisibility(View.VISIBLE);
            mGridContainer.setVisibility(View.GONE);
        }
    }

    public void setGridShownNoAnimation(boolean shown) {
        setGridShown(shown, false);
    }

    public void setSelection(int position) {
        ensureGrid();
        mGrid.setSelection(position);
    }
}
