
package org.holoeverywhere.app;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.internal.AlertController;
import org.holoeverywhere.widget.Button;
import org.holoeverywhere.widget.GridView;
import org.holoeverywhere.widget.ListView;
import org.holoeverywhere.widget.TextView;

import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.LabeledIntent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build.VERSION;
import android.os.Bundle;
import android.os.PatternMatcher;
import android.os.Process;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;

public abstract class ResolverActivity extends AlertActivity implements
        AdapterView.OnItemClickListener {
    private final class DisplayResolveInfo {
        Drawable displayIcon;
        CharSequence displayLabel;
        CharSequence extendedInfo;
        Intent origIntent;
        ResolveInfo ri;

        DisplayResolveInfo(ResolveInfo pri, CharSequence pLabel,
                CharSequence pInfo, Intent pOrigIntent) {
            ri = pri;
            displayLabel = pLabel;
            extendedInfo = pInfo;
            origIntent = pOrigIntent;
        }
    }

    class ItemLongClickListener implements AdapterView.OnItemLongClickListener {
        @Override
        public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
            ResolveInfo ri = mAdapter.resolveInfoForPosition(position);
            showAppDetails(ri);
            return true;
        }
    }

    private final class ResolveListAdapter extends BaseAdapter {
        private final List<ResolveInfo> mBaseResolveList;
        private List<ResolveInfo> mCurrentResolveList;
        private final LayoutInflater mInflater;
        private final Intent[] mInitialIntents;
        private final Intent mIntent;

        private final int mLaunchedFromUid;
        private List<DisplayResolveInfo> mList;

        public ResolveListAdapter(Context context, Intent intent,
                Intent[] initialIntents, List<ResolveInfo> rList, int launchedFromUid) {
            mIntent = new Intent(intent);
            mIntent.setComponent(null);
            mInitialIntents = initialIntents;
            mBaseResolveList = rList;
            mLaunchedFromUid = launchedFromUid;
            mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            rebuildList();
        }

        private final void bindView(View view, DisplayResolveInfo info) {
            TextView text = (TextView) view.findViewById(android.R.id.text1);
            TextView text2 = (TextView) view.findViewById(android.R.id.text2);
            ImageView icon = (ImageView) view.findViewById(R.id.icon);
            text.setText(info.displayLabel);
            if (mShowExtended) {
                text2.setVisibility(View.VISIBLE);
                text2.setText(info.extendedInfo);
            } else {
                text2.setVisibility(View.GONE);
            }
            if (info.displayIcon == null) {
                info.displayIcon = loadIconForResolveInfo(info.ri);
            }
            icon.setImageDrawable(info.displayIcon);
        }

        @Override
        public int getCount() {
            return mList != null ? mList.size() : 0;
        }

        @Override
        public Object getItem(int position) {
            return position;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View view;
            if (convertView == null) {
                view = mInflater.inflate(R.layout.resolve_list_item, parent, false);
                ImageView icon = (ImageView) view.findViewById(R.id.icon);
                ViewGroup.LayoutParams lp = icon.getLayoutParams();
                lp.width = lp.height = mIconSize;
            } else {
                view = convertView;
            }
            bindView(view, mList.get(position));
            return view;
        }

        public void handlePackagesChanged() {
            final int oldItemCount = getCount();
            rebuildList();
            notifyDataSetChanged();
            if (mList.size() == 0) {
                finish();
            }
            final int newItemCount = getCount();
            if (newItemCount != oldItemCount) {
                resizeGrid();
            }
        }

        public Intent intentForPosition(int position) {
            if (mList == null) {
                return null;
            }
            DisplayResolveInfo dri = mList.get(position);
            Intent intent = new Intent(dri.origIntent != null
                    ? dri.origIntent : mIntent);
            intent.addFlags(Intent.FLAG_ACTIVITY_FORWARD_RESULT
                    | Intent.FLAG_ACTIVITY_PREVIOUS_IS_TOP);
            ActivityInfo ai = dri.ri.activityInfo;
            intent.setComponent(new ComponentName(
                    ai.applicationInfo.packageName, ai.name));
            return intent;
        }

        private void processGroup(List<ResolveInfo> rList, int start, int end, ResolveInfo ro,
                CharSequence roLabel) {
            int num = end - start + 1;
            if (num == 1) {
                mList.add(new DisplayResolveInfo(ro, roLabel, null, null));
            } else {
                mShowExtended = true;
                boolean usePkg = false;
                CharSequence startApp = ro.activityInfo.applicationInfo.loadLabel(mPm);
                if (startApp == null) {
                    usePkg = true;
                }
                if (!usePkg) {
                    HashSet<CharSequence> duplicates =
                            new HashSet<CharSequence>();
                    duplicates.add(startApp);
                    for (int j = start + 1; j <= end; j++) {
                        ResolveInfo jRi = rList.get(j);
                        CharSequence jApp = jRi.activityInfo.applicationInfo.loadLabel(mPm);
                        if (jApp == null || duplicates.contains(jApp)) {
                            usePkg = true;
                            break;
                        } else {
                            duplicates.add(jApp);
                        }
                    }
                    duplicates.clear();
                }
                for (int k = start; k <= end; k++) {
                    ResolveInfo add = rList.get(k);
                    if (usePkg) {
                        mList.add(new DisplayResolveInfo(add, roLabel,
                                add.activityInfo.packageName, null));
                    } else {
                        mList.add(new DisplayResolveInfo(add, roLabel,
                                add.activityInfo.applicationInfo.loadLabel(mPm), null));
                    }
                }
            }
        }

        private void rebuildList() {
            if (mBaseResolveList != null) {
                mCurrentResolveList = mBaseResolveList;
            } else {
                mCurrentResolveList = mPm.queryIntentActivities(
                        mIntent, PackageManager.MATCH_DEFAULT_ONLY
                                | (mAlwaysUseOption ? PackageManager.GET_RESOLVED_FILTER : 0));
                if (mCurrentResolveList != null) {
                    for (int i = mCurrentResolveList.size() - 1; i >= 0; i--) {
                        ActivityInfo ai = mCurrentResolveList.get(i).activityInfo;
                        int granted = checkComponentPermission(
                                ai.permission, mLaunchedFromUid,
                                ai.applicationInfo.uid, ai.exported);
                        if (granted != PackageManager.PERMISSION_GRANTED) {
                            mCurrentResolveList.remove(i);
                        }
                    }
                }
            }
            int N;
            if (mCurrentResolveList != null && (N = mCurrentResolveList.size()) > 0) {
                ResolveInfo r0 = mCurrentResolveList.get(0);
                for (int i = 1; i < N; i++) {
                    ResolveInfo ri = mCurrentResolveList.get(i);
                    if (r0.priority != ri.priority ||
                            r0.isDefault != ri.isDefault) {
                        while (i < N) {
                            mCurrentResolveList.remove(i);
                            N--;
                        }
                    }
                }
                if (N > 1) {
                    ResolveInfo.DisplayNameComparator rComparator =
                            new ResolveInfo.DisplayNameComparator(mPm);
                    Collections.sort(mCurrentResolveList, rComparator);
                }
                mList = new ArrayList<DisplayResolveInfo>();
                if (mInitialIntents != null) {
                    for (Intent ii : mInitialIntents) {
                        if (ii == null) {
                            continue;
                        }
                        ActivityInfo ai = ii.resolveActivityInfo(
                                getPackageManager(), 0);
                        if (ai == null) {
                            Log.w("ResolverActivity", "No activity found for "
                                    + ii);
                            continue;
                        }
                        ResolveInfo ri = new ResolveInfo();
                        ri.activityInfo = ai;
                        if (ii instanceof LabeledIntent) {
                            LabeledIntent li = (LabeledIntent) ii;
                            ri.resolvePackageName = li.getSourcePackage();
                            ri.labelRes = li.getLabelResource();
                            ri.nonLocalizedLabel = li.getNonLocalizedLabel();
                            ri.icon = li.getIconResource();
                        }
                        mList.add(new DisplayResolveInfo(ri,
                                ri.loadLabel(getPackageManager()), null, ii));
                    }
                }
                r0 = mCurrentResolveList.get(0);
                int start = 0;
                CharSequence r0Label = r0.loadLabel(mPm);
                mShowExtended = false;
                for (int i = 1; i < N; i++) {
                    if (r0Label == null) {
                        r0Label = r0.activityInfo.packageName;
                    }
                    ResolveInfo ri = mCurrentResolveList.get(i);
                    CharSequence riLabel = ri.loadLabel(mPm);
                    if (riLabel == null) {
                        riLabel = ri.activityInfo.packageName;
                    }
                    if (riLabel.equals(r0Label)) {
                        continue;
                    }
                    processGroup(mCurrentResolveList, start, i - 1, r0, r0Label);
                    r0 = ri;
                    r0Label = riLabel;
                    start = i;
                }
                processGroup(mCurrentResolveList, start, N - 1, r0, r0Label);
            }
        }

        public ResolveInfo resolveInfoForPosition(int position) {
            if (mList == null) {
                return null;
            }
            return mList.get(position).ri;
        }
    }

    public static final int FIRST_ISOLATED_UID = 99000;
    public static final int LAST_ISOLATED_UID = 99999;
    public static final int PER_USER_RANGE = 100000;
    private static final String TAG = "ResolverActivity";

    public static final int getAppId(int uid) {
        return uid % PER_USER_RANGE;
    }

    public static final boolean isIsolated(int uid) {
        if (uid > 0) {
            final int appId = getAppId(uid);
            return appId >= FIRST_ISOLATED_UID && appId <= LAST_ISOLATED_UID;
        } else {
            return false;
        }
    }

    public static final boolean isSameApp(int uid1, int uid2) {
        return getAppId(uid1) == getAppId(uid2);
    }

    private ResolveListAdapter mAdapter;
    private Button mAlwaysButton;
    private boolean mAlwaysUseOption;

    private GridView mGrid;

    private int mIconDpi;

    private int mIconSize;

    private int mLastSelected = GridView.INVALID_POSITION;

    private int mLaunchedFromUid;

    private int mMaxColumns;

    private Button mOnceButton;

    private PackageManager mPm;

    private boolean mShowExtended;

    public int checkComponentPermission(String permission, int uid,
            int owningUid, boolean exported) {
        if (uid == 0 || uid == Process.SYSTEM_UID) {
            return PackageManager.PERMISSION_GRANTED;
        }
        if (isIsolated(uid)) {
            return PackageManager.PERMISSION_DENIED;
        }
        if (owningUid >= 0 && isSameApp(uid, owningUid)) {
            return PackageManager.PERMISSION_GRANTED;
        }
        if (!exported) {
            Log.w(TAG, "Permission denied: checkComponentPermission() owningUid=" + owningUid);
            return PackageManager.PERMISSION_DENIED;
        }
        if (permission == null) {
            return PackageManager.PERMISSION_GRANTED;
        }
        // TODO try {
        // return AppGlobals.getPackageManager()
        // .checkUidPermission(permission, uid);
        // } catch (RemoteException e) {
        // Log.e(TAG, "PackageManager is dead?!?", e);
        // }
        // return PackageManager.PERMISSION_DENIED;
        return PackageManager.PERMISSION_GRANTED;
    }

    Drawable getIcon(Resources res, int resId) {
        try {
            if (VERSION.SDK_INT >= 15) {
                return res.getDrawableForDensity(resId, mIconDpi);
            } else {
                return res.getDrawable(resId);
            }
        } catch (Resources.NotFoundException e) {
            return null;
        }
    }

    private int getLauncherLargeIconDensity(ActivityManager am) {
        if (VERSION.SDK_INT >= 11) {
            return am.getLauncherLargeIconDensity();
        }
        final Resources res = getResources();
        final int density = res.getDisplayMetrics().densityDpi;
        final Configuration conf = res.getConfiguration();
        final int sw;
        if (VERSION.SDK_INT >= 13) {
            sw = conf.smallestScreenWidthDp;
        } else {
            final DisplayMetrics metrics = res.getDisplayMetrics();
            sw = (int) (Math.min(metrics.widthPixels, metrics.heightPixels) / metrics.density);
        }
        if (sw < 600) {
            return density;
        }
        switch (density) {
            case DisplayMetrics.DENSITY_LOW:
                return DisplayMetrics.DENSITY_MEDIUM;
            case DisplayMetrics.DENSITY_MEDIUM:
                return DisplayMetrics.DENSITY_HIGH;
            case DisplayMetrics.DENSITY_TV:
                return DisplayMetrics.DENSITY_XHIGH;
            case DisplayMetrics.DENSITY_HIGH:
                return DisplayMetrics.DENSITY_XHIGH;
            case DisplayMetrics.DENSITY_XHIGH:
                return DisplayMetrics.DENSITY_XXHIGH;
            case DisplayMetrics.DENSITY_XXHIGH:
                return DisplayMetrics.DENSITY_XHIGH * 2;
            default:
                return (int) (density * 1.5f + .5f);
        }
    }

    private int getLauncherLargeIconSize(ActivityManager am) {
        if (VERSION.SDK_INT >= 11) {
            return am.getLauncherLargeIconSize();
        }
        final Resources res = getResources();
        final int size = res.getDimensionPixelSize(android.R.dimen.app_icon_size);
        final Configuration conf = res.getConfiguration();
        final int sw;
        if (VERSION.SDK_INT >= 13) {
            sw = conf.smallestScreenWidthDp;
        } else {
            final DisplayMetrics metrics = res.getDisplayMetrics();
            sw = (int) (Math.min(metrics.widthPixels, metrics.heightPixels) / metrics.density);
        }
        if (sw < 600) {
            return size;
        }
        final int density = res.getDisplayMetrics().densityDpi;
        switch (density) {
            case DisplayMetrics.DENSITY_LOW:
                return size * DisplayMetrics.DENSITY_MEDIUM / DisplayMetrics.DENSITY_LOW;
            case DisplayMetrics.DENSITY_MEDIUM:
                return size * DisplayMetrics.DENSITY_HIGH / DisplayMetrics.DENSITY_MEDIUM;
            case DisplayMetrics.DENSITY_TV:
                return size * DisplayMetrics.DENSITY_XHIGH / DisplayMetrics.DENSITY_HIGH;
            case DisplayMetrics.DENSITY_HIGH:
                return size * DisplayMetrics.DENSITY_XHIGH / DisplayMetrics.DENSITY_HIGH;
            case DisplayMetrics.DENSITY_XHIGH:
                return size * DisplayMetrics.DENSITY_XXHIGH / DisplayMetrics.DENSITY_XHIGH;
            case DisplayMetrics.DENSITY_XXHIGH:
                return size * DisplayMetrics.DENSITY_XHIGH * 2 / DisplayMetrics.DENSITY_XXHIGH;
            default:
                return (int) (size * 1.5f + .5f);
        }
    }

    Drawable loadIconForResolveInfo(ResolveInfo ri) {
        Drawable dr;
        try {
            if (ri.resolvePackageName != null && ri.icon != 0) {
                dr = getIcon(mPm.getResourcesForApplication(ri.resolvePackageName), ri.icon);
                if (dr != null) {
                    return dr;
                }
            }
            final int iconRes = ri.getIconResource();
            if (iconRes != 0) {
                dr = getIcon(mPm.getResourcesForApplication(ri.activityInfo.packageName), iconRes);
                if (dr != null) {
                    return dr;
                }
            }
        } catch (NameNotFoundException e) {
            Log.e(TAG, "Couldn't find resources for package", e);
        }
        return ri.loadIcon(mPm);
    }

    private Intent makeMyIntent() {
        Intent intent = new Intent(getIntent());
        intent.setFlags(intent.getFlags() & ~Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
        return intent;
    }

    public void onButtonClick(View v) {
        final int id = v.getId();
        startSelected(mGrid.getCheckedItemPosition(), id == R.id.button_always);
        dismiss();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        onCreate(savedInstanceState, makeMyIntent(),
                getResources().getText(R.string.whichApplication),
                null, null, true);
    }

    protected void onCreate(Bundle savedInstanceState, Intent intent,
            CharSequence title, Intent[] initialIntents, List<ResolveInfo> rList,
            boolean alwaysUseOption) {
        super.onCreate(savedInstanceState);
        try {
            mLaunchedFromUid = getPackageManager().getApplicationInfo(getPackageName(), 0).uid;
        } catch (NameNotFoundException e) {
            e.printStackTrace();
            mLaunchedFromUid = -1;
        }
        mPm = getPackageManager();
        mAlwaysUseOption = alwaysUseOption;
        mMaxColumns = getResources().getInteger(R.integer.config_maxResolverActivityColumns);
        intent.setComponent(null);
        AlertController.AlertParams ap = mAlertParams;
        ap.mTitle = title;
        final ActivityManager am = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
        mIconDpi = getLauncherLargeIconDensity(am);
        mIconSize = getLauncherLargeIconSize(am);
        mAdapter = new ResolveListAdapter(this, intent, initialIntents, rList,
                mLaunchedFromUid);
        int count = mAdapter.getCount();
        if (mLaunchedFromUid < 0 || isIsolated(mLaunchedFromUid)) {
            finish();
            return;
        } else if (count > 1) {
            ap.mView = getLayoutInflater().inflate(R.layout.resolver_grid, null);
            mGrid = (GridView) ap.mView.findViewById(R.id.resolver_grid);
            mGrid.setAdapter(mAdapter);
            mGrid.setOnItemClickListener(this);
            mGrid.setOnItemLongClickListener(new ItemLongClickListener());
            if (alwaysUseOption) {
                mGrid.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
            }
            resizeGrid();
        } else if (count == 1) {
            startActivity(mAdapter.intentForPosition(0));
            finish();
            return;
        } else {
            ap.mMessage = getResources().getText(R.string.noApplications);
        }
        setupAlert();
        if (alwaysUseOption) {
            final ViewGroup buttonLayout = (ViewGroup) findViewById(R.id.button_bar);
            if (buttonLayout != null) {
                buttonLayout.setVisibility(View.VISIBLE);
                mAlwaysButton = (Button) buttonLayout.findViewById(R.id.button_always);
                mOnceButton = (Button) buttonLayout.findViewById(R.id.button_once);
            } else {
                mAlwaysUseOption = false;
            }
        }
    }

    @SuppressWarnings("deprecation")
    protected void onIntentSelected(ResolveInfo ri, Intent intent, boolean alwaysCheck) {
        if (alwaysCheck) {
            IntentFilter filter = new IntentFilter();
            if (intent.getAction() != null) {
                filter.addAction(intent.getAction());
            }
            Set<String> categories = intent.getCategories();
            if (categories != null) {
                for (String cat : categories) {
                    filter.addCategory(cat);
                }
            }
            filter.addCategory(Intent.CATEGORY_DEFAULT);
            int cat = ri.match & IntentFilter.MATCH_CATEGORY_MASK;
            Uri data = intent.getData();
            if (cat == IntentFilter.MATCH_CATEGORY_TYPE) {
                String mimeType = intent.resolveType(this);
                if (mimeType != null) {
                    try {
                        filter.addDataType(mimeType);
                    } catch (IntentFilter.MalformedMimeTypeException e) {
                        Log.w("ResolverActivity", e);
                        filter = null;
                    }
                }
            }
            if (data != null && data.getScheme() != null) {
                if (cat != IntentFilter.MATCH_CATEGORY_TYPE
                        || !"file".equals(data.getScheme())
                        && !"content".equals(data.getScheme())) {
                    filter.addDataScheme(data.getScheme());
                    Iterator<IntentFilter.AuthorityEntry> aIt = ri.filter.authoritiesIterator();
                    if (aIt != null) {
                        while (aIt.hasNext()) {
                            IntentFilter.AuthorityEntry a = aIt.next();
                            if (a.match(data) >= 0) {
                                int port = a.getPort();
                                filter.addDataAuthority(a.getHost(),
                                        port >= 0 ? Integer.toString(port) : null);
                                break;
                            }
                        }
                    }
                    Iterator<PatternMatcher> pIt = ri.filter.pathsIterator();
                    if (pIt != null) {
                        String path = data.getPath();
                        while (path != null && pIt.hasNext()) {
                            PatternMatcher p = pIt.next();
                            if (p.match(path)) {
                                filter.addDataPath(p.getPath(), p.getType());
                                break;
                            }
                        }
                    }
                }
            }
            if (filter != null) {
                final int N = mAdapter.mList.size();
                ComponentName[] set = new ComponentName[N];
                int bestMatch = 0;
                for (int i = 0; i < N; i++) {
                    ResolveInfo r = mAdapter.mList.get(i).ri;
                    set[i] = new ComponentName(r.activityInfo.packageName,
                            r.activityInfo.name);
                    if (r.match > bestMatch) {
                        bestMatch = r.match;
                    }
                }
                try {
                    getPackageManager().addPreferredActivity(filter, bestMatch, set,
                            intent.getComponent());
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        if (intent != null) {
            startActivity(intent);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        final int checkedPos = mGrid.getCheckedItemPosition();
        final boolean hasValidSelection = checkedPos != AdapterView.INVALID_POSITION;
        if (mAlwaysUseOption && (!hasValidSelection || mLastSelected != checkedPos)) {
            mAlwaysButton.setEnabled(hasValidSelection);
            mOnceButton.setEnabled(hasValidSelection);
            if (hasValidSelection) {
                mGrid.smoothScrollToPosition(checkedPos);
            }
            mLastSelected = checkedPos;
        } else {
            startSelected(position, false);
        }
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        mAdapter.handlePackagesChanged();
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        if (mAlwaysUseOption) {
            final int checkedPos = mGrid.getCheckedItemPosition();
            final boolean enabled = checkedPos != AdapterView.INVALID_POSITION;
            mLastSelected = checkedPos;
            mAlwaysButton.setEnabled(enabled);
            mOnceButton.setEnabled(enabled);
            if (enabled) {
                mGrid.setSelection(checkedPos);
            }
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        if ((getIntent().getFlags() & Intent.FLAG_ACTIVITY_NEW_TASK) != 0) {
            if (!isChangingConfigurations()) {
                finish();
            }
        }
    }

    void resizeGrid() {
        final int itemCount = mAdapter.getCount();
        mGrid.setNumColumns(Math.min(itemCount, mMaxColumns));
    }

    void showAppDetails(ResolveInfo ri) {
        Intent in = new Intent().setAction("android.settings.APPLICATION_DETAILS_SETTINGS")
                .setData(Uri.fromParts("package", ri.activityInfo.packageName, null))
                .addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
        startActivity(in);
    }

    void startSelected(int which, boolean always) {
        ResolveInfo ri = mAdapter.resolveInfoForPosition(which);
        Intent intent = mAdapter.intentForPosition(which);
        onIntentSelected(ri, intent, always);
        finish();
    }
}
