
package android.support.v4.app;

import org.holoeverywhere.LayoutInflater;

import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.TypedArray;
import android.support.v4.app.FragmentActivity.FragmentTag;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;

public class _HoloFragmentInflater {
    private static View inflate(AttributeSet attrs, View parent, FragmentActivity activity,
            Fragment parentFragment) {
        String fname = attrs.getAttributeValue(null, "class");
        TypedArray a = activity.obtainStyledAttributes(attrs, FragmentTag.Fragment);
        if (fname == null) {
            fname = a.getString(FragmentTag.Fragment_name);
        }
        if (fname.startsWith(".")) {
            fname = activity.getPackageName() + fname;
        }
        int id = a.getResourceId(FragmentTag.Fragment_id, View.NO_ID);
        String tag = a.getString(FragmentTag.Fragment_tag);
        a.recycle();
        int containerId = parent != null ? parent.getId() : 0;
        if (containerId == View.NO_ID && id == View.NO_ID && tag == null) {
            throw new IllegalArgumentException(
                    attrs.getPositionDescription()
                            + ": Must specify unique android:id, android:tag, or have a parent with an id for "
                            + fname);
        }
        final FragmentManagerImpl fm = obtainFragmentManager(activity, parentFragment);
        Fragment fragment = id != View.NO_ID ? fm.findFragmentById(id) : null;
        if (fragment == null && tag != null) {
            fragment = fm.findFragmentByTag(tag);
        }
        if (fragment == null && containerId != View.NO_ID) {
            fragment = fm.findFragmentById(containerId);
        }
        if (fragment == null) {
            fragment = Fragment.instantiate(activity, fname);
            fragment.mParentFragment = parentFragment;
            fragment.mFromLayout = true;
            fragment.mFragmentId = id != 0 ? id : containerId;
            fragment.mContainer = (ViewGroup) parent;
            fragment.mContainerId = containerId;
            fragment.mTag = tag;
            fragment.mInLayout = true;
            fragment.mFragmentManager = fm;
            fragment.onInflate(activity, attrs, fragment.mSavedFragmentState);
            fm.addFragment(fragment, false);
            fm.moveToState(fragment, Fragment.CREATED, 0, 0, false);
        } else if (fragment.mInLayout) {
            throw new IllegalArgumentException(attrs.getPositionDescription()
                    + ": Duplicate id 0x" + Integer.toHexString(id)
                    + ", tag " + tag + ", or parent id 0x" + Integer.toHexString(containerId)
                    + " with another fragment for " + fname);
        } else {
            fragment.mInLayout = true;
            if (!fragment.mRetaining) {
                fragment.onInflate(activity, attrs, fragment.mSavedFragmentState);
            }
            fm.moveToState(fragment, Fragment.CREATED, 0, 0, false);
        }
        if (fragment.mView == null) {
            throw new IllegalStateException("Fragment " + fname
                    + " did not create a view.");
        }
        if (id != 0) {
            fragment.mView.setId(id);
        }
        if (fragment.mView.getTag() == null) {
            fragment.mView.setTag(tag);
        }
        return fragment.mView;
    }

    public static View inflate(LayoutInflater layoutInflater, AttributeSet attrs, View parent,
            Fragment fragment) {
        FragmentActivity activity = layoutInflater.getFragmentActivity();
        if (activity != null) {
            return inflate(attrs, parent, activity, fragment);
        }
        Context context = layoutInflater.getContext();
        while (context instanceof ContextWrapper) {
            if (context instanceof FragmentActivity) {
                activity = (FragmentActivity) context;
                break;
            }
            context = ((ContextWrapper) context).getBaseContext();
        }
        if (activity == null) {
            throw new IllegalStateException("Cannot find any reference to FragmentActivity");
        }
        return inflate(attrs, parent, activity, fragment);
    }

    private static FragmentManagerImpl obtainFragmentManager(FragmentActivity activity,
            Fragment fragment) {
        FragmentManagerImpl fm = null;
        if (fragment != null) {
            fm = fragment.mChildFragmentManager;
            if (fm == null) {
                try {
                    fm = (FragmentManagerImpl) fragment.getChildFragmentManager();
                } catch (ClassCastException e) {
                    fm = fragment.mChildFragmentManager;
                }
            }
        }
        if (fm == null && activity != null) {
            fm = activity.mFragments;
        }
        return fm;
    }
}
