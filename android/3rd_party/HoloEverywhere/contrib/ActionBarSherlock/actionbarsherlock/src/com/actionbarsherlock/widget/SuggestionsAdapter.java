/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.actionbarsherlock.widget;

import android.app.SearchManager;
import android.app.SearchableInfo;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.widget.ResourceCursorAdapter;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.TextAppearanceSpan;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import com.actionbarsherlock.R;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.WeakHashMap;

/**
 * Provides the contents for the suggestion drop-down list.
 *
 * @hide
 */
class SuggestionsAdapter extends ResourceCursorAdapter implements OnClickListener {

    private static final boolean DBG = false;
    private static final String LOG_TAG = "SuggestionsAdapter";
    private static final int QUERY_LIMIT = 50;

    static final int REFINE_NONE = 0;
    static final int REFINE_BY_ENTRY = 1;
    static final int REFINE_ALL = 2;

    private SearchManager mSearchManager;
    private SearchableInfo mSearchable;
    private SearchView mSearchView;
    private Context mProviderContext;
    private WeakHashMap<String, Drawable.ConstantState> mOutsideDrawablesCache;
    private boolean mClosed = false;
    private int mQueryRefinement = REFINE_BY_ENTRY;

    // URL color
    private ColorStateList mUrlColor;

    static final int INVALID_INDEX = -1;

    // Cached column indexes, updated when the cursor changes.
    private int mText1Col = INVALID_INDEX;
    private int mText2Col = INVALID_INDEX;
    private int mText2UrlCol = INVALID_INDEX;
    private int mIconName1Col = INVALID_INDEX;
    private int mIconName2Col = INVALID_INDEX;
    private int mFlagsCol = INVALID_INDEX;

    // private final Runnable mStartSpinnerRunnable;
    // private final Runnable mStopSpinnerRunnable;

    /**
     * The amount of time we delay in the filter when the user presses the delete key.
     */
    //private static final long DELETE_KEY_POST_DELAY = 500L;

    public SuggestionsAdapter(Context context, SearchView searchView,
                              SearchableInfo searchable, WeakHashMap<String, Drawable.ConstantState> outsideDrawablesCache) {
        super(context,
            R.layout.abs__search_dropdown_item_icons_2line,
            null,   // no initial cursor
            true);  // auto-requery
        mSearchManager = (SearchManager) mContext.getSystemService(Context.SEARCH_SERVICE);
        mSearchable = searchable;
        mProviderContext = mContext;
        mSearchView = searchView;

        mOutsideDrawablesCache = outsideDrawablesCache;

        // mStartSpinnerRunnable = new Runnable() {
        // public void run() {
        // // mSearchView.setWorking(true); // TODO:
        // }
        // };
        //
        // mStopSpinnerRunnable = new Runnable() {
        // public void run() {
        // // mSearchView.setWorking(false); // TODO:
        // }
        // };

        // delay 500ms when deleting
//  TODO  getFilter().setDelayer(new Filter.Delayer() {
//
//      private int mPreviousLength = 0;
//
//      public long getPostingDelay(CharSequence constraint) {
//        if (constraint == null) return 0;
//
//        long delay = constraint.length() < mPreviousLength ? DELETE_KEY_POST_DELAY : 0;
//        mPreviousLength = constraint.length();
//        return delay;
//      }
//    });
    }

    /**
     * Enables query refinement for all suggestions. This means that an additional icon
     * will be shown for each entry. When clicked, the suggested text on that line will be
     * copied to the query text field.
     * <p>
     *
     * @param refineWhat which queries to refine. Possible values are {@link #REFINE_NONE},
     * {@link #REFINE_BY_ENTRY}, and {@link #REFINE_ALL}.
     */
    public void setQueryRefinement(int refineWhat) {
        mQueryRefinement = refineWhat;
    }

    /**
     * Returns the current query refinement preference.
     * @return value of query refinement preference
     */
    public int getQueryRefinement() {
        return mQueryRefinement;
    }

    /**
     * Overridden to always return <code>false</code>, since we cannot be sure that
     * suggestion sources return stable IDs.
     */
    @Override
    public boolean hasStableIds() {
        return false;
    }

    /**
     * Use the search suggestions provider to obtain a live cursor.  This will be called
     * in a worker thread, so it's OK if the query is slow (e.g. round trip for suggestions).
     * The results will be processed in the UI thread and changeCursor() will be called.
     */
    @Override
    public Cursor runQueryOnBackgroundThread(CharSequence constraint) {
        if (DBG) Log.d(LOG_TAG, "runQueryOnBackgroundThread(" + constraint + ")");
        String query = (constraint == null) ? "" : constraint.toString();
        /**
         * for in app search we show the progress spinner until the cursor is returned with
         * the results.
         */
        Cursor cursor = null;
        if (mSearchView.getVisibility() != View.VISIBLE
                || mSearchView.getWindowVisibility() != View.VISIBLE) {
            return null;
        }
        //mSearchView.getWindow().getDecorView().post(mStartSpinnerRunnable); // TODO:
        try {
            cursor = getSuggestions(query, QUERY_LIMIT);
            // trigger fill window so the spinner stays up until the results are copied over and
            // closer to being ready
            if (cursor != null) {
                cursor.getCount();
                return cursor;
            }
        } catch (RuntimeException e) {
            Log.w(LOG_TAG, "Search suggestions query threw an exception.", e);
        }
        // If cursor is null or an exception was thrown, stop the spinner and return null.
        // changeCursor doesn't get called if cursor is null
        // mSearchView.getWindow().getDecorView().post(mStopSpinnerRunnable); // TODO:
        return null;
    }

    public Cursor getSuggestions(String query, int limit) {
        if (mSearchable == null) {
            return null;
        }

        String authority = mSearchable.getSuggestAuthority();
        if (authority == null) {
            return null;
        }

        Uri.Builder uriBuilder = new Uri.Builder()
                .scheme(ContentResolver.SCHEME_CONTENT)
                .authority(authority)
                .query("")  // TODO: Remove, workaround for a bug in Uri.writeToParcel()
                .fragment("");  // TODO: Remove, workaround for a bug in Uri.writeToParcel()

        // if content path provided, insert it now
        final String contentPath = mSearchable.getSuggestPath();
        if (contentPath != null) {
            uriBuilder.appendEncodedPath(contentPath);
        }

        // append standard suggestion query path
        uriBuilder.appendPath(SearchManager.SUGGEST_URI_PATH_QUERY);

        // get the query selection, may be null
        String selection = mSearchable.getSuggestSelection();
        // inject query, either as selection args or inline
        String[] selArgs = null;
        if (selection != null) {    // use selection if provided
            selArgs = new String[] { query };
        } else {                    // no selection, use REST pattern
            uriBuilder.appendPath(query);
        }

        if (limit > 0) {
            uriBuilder.appendQueryParameter("limit", String.valueOf(limit));
        }

        Uri uri = uriBuilder.build();

        // finally, make the query
        return mContext.getContentResolver().query(uri, null, selection, selArgs, null);
    }

    public void close() {
        if (DBG) Log.d(LOG_TAG, "close()");
        changeCursor(null);
        mClosed = true;
    }

    @Override
    public void notifyDataSetChanged() {
        if (DBG) Log.d(LOG_TAG, "notifyDataSetChanged");
        super.notifyDataSetChanged();

        // mSearchView.onDataSetChanged(); // TODO:

        updateSpinnerState(getCursor());
    }

    @Override
    public void notifyDataSetInvalidated() {
        if (DBG) Log.d(LOG_TAG, "notifyDataSetInvalidated");
        super.notifyDataSetInvalidated();

        updateSpinnerState(getCursor());
    }

    private void updateSpinnerState(Cursor cursor) {
        Bundle extras = cursor != null ? cursor.getExtras() : null;
        if (DBG) {
            Log.d(LOG_TAG, "updateSpinnerState - extra = "
                    + (extras != null
                    ? extras.getBoolean(SearchManager.CURSOR_EXTRA_KEY_IN_PROGRESS)
                    : null));
        }
        // Check if the Cursor indicates that the query is not complete and show the spinner
        if (extras != null
                && extras.getBoolean(SearchManager.CURSOR_EXTRA_KEY_IN_PROGRESS)) {
            // mSearchView.getWindow().getDecorView().post(mStartSpinnerRunnable); // TODO:
            return;
        }
        // If cursor is null or is done, stop the spinner
        // mSearchView.getWindow().getDecorView().post(mStopSpinnerRunnable); // TODO:
    }

    /**
     * Cache columns.
     */
    @Override
    public void changeCursor(Cursor c) {
        if (DBG) Log.d(LOG_TAG, "changeCursor(" + c + ")");

        if (mClosed) {
            Log.w(LOG_TAG, "Tried to change cursor after adapter was closed.");
            if (c != null) c.close();
            return;
        }

        try {
            super.changeCursor(c);

            if (c != null) {
                mText1Col = c.getColumnIndex(SearchManager.SUGGEST_COLUMN_TEXT_1);
                mText2Col = c.getColumnIndex(SearchManager.SUGGEST_COLUMN_TEXT_2);
                mText2UrlCol = c.getColumnIndex(SearchManager.SUGGEST_COLUMN_TEXT_2_URL);
                mIconName1Col = c.getColumnIndex(SearchManager.SUGGEST_COLUMN_ICON_1);
                mIconName2Col = c.getColumnIndex(SearchManager.SUGGEST_COLUMN_ICON_2);
                mFlagsCol = c.getColumnIndex(SearchManager.SUGGEST_COLUMN_FLAGS);
            }
        } catch (Exception e) {
            Log.e(LOG_TAG, "error changing cursor and caching columns", e);
        }
    }

    /**
     * Tags the view with cached child view look-ups.
     */
    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
        View v = super.newView(context, cursor, parent);
        v.setTag(new ChildViewCache(v));
        return v;
    }

    /**
     * Cache of the child views of drop-drown list items, to avoid looking up the children
     * each time the contents of a list item are changed.
     */
    private final static class ChildViewCache {
        public final TextView mText1;
        public final TextView mText2;
        public final ImageView mIcon1;
        public final ImageView mIcon2;
        public final ImageView mIconRefine;

        public ChildViewCache(View v) {
            mText1 = (TextView) v.findViewById(android.R.id.text1);
            mText2 = (TextView) v.findViewById(android.R.id.text2);
            mIcon1 = (ImageView) v.findViewById(android.R.id.icon1);
            mIcon2 = (ImageView) v.findViewById(android.R.id.icon2);
            mIconRefine = (ImageView) v.findViewById(R.id.edit_query);
        }
    }

    @Override
    public void bindView(View view, Context context, Cursor cursor) {
        ChildViewCache views = (ChildViewCache) view.getTag();

        int flags = 0;
        if (mFlagsCol != INVALID_INDEX) {
            flags = cursor.getInt(mFlagsCol);
        }
        if (views.mText1 != null) {
            String text1 = getStringOrNull(cursor, mText1Col);
            setViewText(views.mText1, text1);
        }
        if (views.mText2 != null) {
            // First check TEXT_2_URL
            CharSequence text2 = getStringOrNull(cursor, mText2UrlCol);
            if (text2 != null) {
                text2 = formatUrl(text2);
            } else {
                text2 = getStringOrNull(cursor, mText2Col);
            }

            // If no second line of text is indicated, allow the first line of text
            // to be up to two lines if it wants to be.
            if (TextUtils.isEmpty(text2)) {
                if (views.mText1 != null) {
                    views.mText1.setSingleLine(false);
                    views.mText1.setMaxLines(2);
                }
            } else {
                if (views.mText1 != null) {
                    views.mText1.setSingleLine(true);
                    views.mText1.setMaxLines(1);
                }
            }
            setViewText(views.mText2, text2);
        }

        if (views.mIcon1 != null) {
            setViewDrawable(views.mIcon1, getIcon1(cursor), View.INVISIBLE);
        }
        if (views.mIcon2 != null) {
            setViewDrawable(views.mIcon2, getIcon2(cursor), View.GONE);
        }
        if (mQueryRefinement == REFINE_ALL
                || (mQueryRefinement == REFINE_BY_ENTRY
                && (flags & SearchManager.FLAG_QUERY_REFINEMENT) != 0)) {
            views.mIconRefine.setVisibility(View.VISIBLE);
            views.mIconRefine.setTag(views.mText1.getText());
            views.mIconRefine.setOnClickListener(this);
        } else {
            views.mIconRefine.setVisibility(View.GONE);
        }
    }

    public void onClick(View v) {
        Object tag = v.getTag();
        if (tag instanceof CharSequence) {
            mSearchView.onQueryRefine((CharSequence) tag);
        }
    }

    private CharSequence formatUrl(CharSequence url) {
        if (mUrlColor == null) {
            // Lazily get the URL color from the current theme.
            TypedValue colorValue = new TypedValue();
            mContext.getTheme().resolveAttribute(R.attr.textColorSearchUrl, colorValue, true);
            mUrlColor = mContext.getResources().getColorStateList(colorValue.resourceId);
        }

        SpannableString text = new SpannableString(url);
        text.setSpan(new TextAppearanceSpan(null, 0, 0, mUrlColor, null),
                0, url.length(),
                Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        return text;
    }

    private void setViewText(TextView v, CharSequence text) {
        // Set the text even if it's null, since we need to clear any previous text.
        v.setText(text);

        if (TextUtils.isEmpty(text)) {
            v.setVisibility(View.GONE);
        } else {
            v.setVisibility(View.VISIBLE);
        }
    }

    private Drawable getIcon1(Cursor cursor) {
        if (mIconName1Col == INVALID_INDEX) {
            return null;
        }
        String value = cursor.getString(mIconName1Col);
        Drawable drawable = getDrawableFromResourceValue(value);
        if (drawable != null) {
            return drawable;
        }
        return getDefaultIcon1(cursor);
    }

    private Drawable getIcon2(Cursor cursor) {
        if (mIconName2Col == INVALID_INDEX) {
            return null;
        }
        String value = cursor.getString(mIconName2Col);
        return getDrawableFromResourceValue(value);
    }

    /**
     * Sets the drawable in an image view, makes sure the view is only visible if there
     * is a drawable.
     */
    private void setViewDrawable(ImageView v, Drawable drawable, int nullVisibility) {
        // Set the icon even if the drawable is null, since we need to clear any
        // previous icon.
        v.setImageDrawable(drawable);

        if (drawable == null) {
            v.setVisibility(nullVisibility);
        } else {
            v.setVisibility(View.VISIBLE);

            // This is a hack to get any animated drawables (like a 'working' spinner)
            // to animate. You have to setVisible true on an AnimationDrawable to get
            // it to start animating, but it must first have been false or else the
            // call to setVisible will be ineffective. We need to clear up the story
            // about animated drawables in the future, see http://b/1878430.
            drawable.setVisible(false, false);
            drawable.setVisible(true, false);
        }
    }

    /**
     * Gets the text to show in the query field when a suggestion is selected.
     *
     * @param cursor The Cursor to read the suggestion data from. The Cursor should already
     *        be moved to the suggestion that is to be read from.
     * @return The text to show, or <code>null</code> if the query should not be
     *         changed when selecting this suggestion.
     */
    @Override
    public CharSequence convertToString(Cursor cursor) {
        if (cursor == null) {
            return null;
        }

        String query = getColumnString(cursor, SearchManager.SUGGEST_COLUMN_QUERY);
        if (query != null) {
            return query;
        }

        return null;
    }

    /**
     * This method is overridden purely to provide a bit of protection against
     * flaky content providers.
     *
     * @see android.widget.ListAdapter#getView(int, View, ViewGroup)
     */
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        try {
            return super.getView(position, convertView, parent);
        } catch (RuntimeException e) {
            Log.w(LOG_TAG, "Search suggestions cursor threw exception.", e);
            // Put exception string in item title
            View v = newView(mContext, mCursor, parent);
            if (v != null) {
                ChildViewCache views = (ChildViewCache) v.getTag();
                TextView tv = views.mText1;
                tv.setText(e.toString());
            }
            return v;
        }
    }

    /**
     * Gets a drawable given a value provided by a suggestion provider.
     *
     * This value could be just the string value of a resource id
     * (e.g., "2130837524"), in which case we will try to retrieve a drawable from
     * the provider's resources. If the value is not an integer, it is
     * treated as a Uri and opened with
     * {@link ContentResolver#openOutputStream(android.net.Uri, String)}.
     *
     * All resources and URIs are read using the suggestion provider's context.
     *
     * If the string is not formatted as expected, or no drawable can be found for
     * the provided value, this method returns null.
     *
     * @param drawableId a string like "2130837524",
     *        "android.resource://com.android.alarmclock/2130837524",
     *        or "content://contacts/photos/253".
     * @return a Drawable, or null if none found
     */
    private Drawable getDrawableFromResourceValue(String drawableId) {
        if (drawableId == null || drawableId.length() == 0 || "0".equals(drawableId)) {
            return null;
        }
        try {
            // First, see if it's just an integer
            int resourceId = Integer.parseInt(drawableId);
            // It's an int, look for it in the cache
            String drawableUri = ContentResolver.SCHEME_ANDROID_RESOURCE
                    + "://" + mProviderContext.getPackageName() + "/" + resourceId;
            // Must use URI as cache key, since ints are app-specific
            Drawable drawable = checkIconCache(drawableUri);
            if (drawable != null) {
                return drawable;
            }
            // Not cached, find it by resource ID
            drawable = mProviderContext.getResources().getDrawable(resourceId);
            // Stick it in the cache, using the URI as key
            storeInIconCache(drawableUri, drawable);
            return drawable;
        } catch (NumberFormatException nfe) {
            // It's not an integer, use it as a URI
            Drawable drawable = checkIconCache(drawableId);
            if (drawable != null) {
                return drawable;
            }
            Uri uri = Uri.parse(drawableId);
            drawable = getDrawable(uri);
            storeInIconCache(drawableId, drawable);
            return drawable;
        } catch (Resources.NotFoundException nfe) {
            // It was an integer, but it couldn't be found, bail out
            Log.w(LOG_TAG, "Icon resource not found: " + drawableId);
            return null;
        }
    }

    /**
     * Gets a drawable by URI, without using the cache.
     *
     * @return A drawable, or {@code null} if the drawable could not be loaded.
     */
    private Drawable getDrawable(Uri uri) {
        try {
            String scheme = uri.getScheme();
            if (ContentResolver.SCHEME_ANDROID_RESOURCE.equals(scheme)) {
                // Load drawables through Resources, to get the source density information
                try {
                    return getTheDrawable(uri);
                } catch (Resources.NotFoundException ex) {
                    throw new FileNotFoundException("Resource does not exist: " + uri);
                }
            } else {
                // Let the ContentResolver handle content and file URIs.
                InputStream stream = mProviderContext.getContentResolver().openInputStream(uri);
                if (stream == null) {
                    throw new FileNotFoundException("Failed to open " + uri);
                }
                try {
                    return Drawable.createFromStream(stream, null);
                } finally {
                    try {
                        stream.close();
                    } catch (IOException ex) {
                        Log.e(LOG_TAG, "Error closing icon stream for " + uri, ex);
                    }
                }
            }
        } catch (FileNotFoundException fnfe) {
            Log.w(LOG_TAG, "Icon not found: " + uri + ", " + fnfe.getMessage());
            return null;
        }
    }

    public Drawable getTheDrawable(Uri uri) throws FileNotFoundException {
        String authority = uri.getAuthority();
        Resources r;
        if (TextUtils.isEmpty(authority)) {
            throw new FileNotFoundException("No authority: " + uri);
        } else {
            try {
                r = mContext.getPackageManager().getResourcesForApplication(authority);
            } catch (NameNotFoundException ex) {
                throw new FileNotFoundException("No package found for authority: " + uri);
            }
        }
        List<String> path = uri.getPathSegments();
        if (path == null) {
            throw new FileNotFoundException("No path: " + uri);
        }
        int len = path.size();
        int id;
        if (len == 1) {
            try {
                id = Integer.parseInt(path.get(0));
            } catch (NumberFormatException e) {
                throw new FileNotFoundException("Single path segment is not a resource ID: " + uri);
            }
        } else if (len == 2) {
            id = r.getIdentifier(path.get(1), path.get(0), authority);
        } else {
            throw new FileNotFoundException("More than two path segments: " + uri);
        }
        if (id == 0) {
            throw new FileNotFoundException("No resource found for: " + uri);
        }
        return r.getDrawable(id);
    }

    private Drawable checkIconCache(String resourceUri) {
        Drawable.ConstantState cached = mOutsideDrawablesCache.get(resourceUri);
        if (cached == null) {
            return null;
        }
        if (DBG) Log.d(LOG_TAG, "Found icon in cache: " + resourceUri);
        return cached.newDrawable();
    }

    private void storeInIconCache(String resourceUri, Drawable drawable) {
        if (drawable != null) {
            mOutsideDrawablesCache.put(resourceUri, drawable.getConstantState());
        }
    }

    /**
     * Gets the left-hand side icon that will be used for the current suggestion
     * if the suggestion contains an icon column but no icon or a broken icon.
     *
     * @param cursor A cursor positioned at the current suggestion.
     * @return A non-null drawable.
     */
    private Drawable getDefaultIcon1(Cursor cursor) {
        // Fall back to a default icon
        return mContext.getPackageManager().getDefaultActivityIcon();
    }

    /**
     * Gets the activity or application icon for an activity.
     * Uses the local icon cache for fast repeated lookups.
     *
     * @param component Name of an activity.
     * @return A drawable, or {@code null} if neither the activity nor the application
     *         has an icon set.
     */
    private Drawable getActivityIconWithCache(ComponentName component) {
        // First check the icon cache
        String componentIconKey = component.flattenToShortString();
        // Using containsKey() since we also store null values.
        if (mOutsideDrawablesCache.containsKey(componentIconKey)) {
            Drawable.ConstantState cached = mOutsideDrawablesCache.get(componentIconKey);
            return cached == null ? null : cached.newDrawable(mProviderContext.getResources());
        }
        // Then try the activity or application icon
        Drawable drawable = getActivityIcon(component);
        // Stick it in the cache so we don't do this lookup again.
        Drawable.ConstantState toCache = drawable == null ? null : drawable.getConstantState();
        mOutsideDrawablesCache.put(componentIconKey, toCache);
        return drawable;
    }

    /**
     * Gets the activity or application icon for an activity.
     *
     * @param component Name of an activity.
     * @return A drawable, or {@code null} if neither the acitivy or the application
     *         have an icon set.
     */
    private Drawable getActivityIcon(ComponentName component) {
        PackageManager pm = mContext.getPackageManager();
        final ActivityInfo activityInfo;
        try {
            activityInfo = pm.getActivityInfo(component, PackageManager.GET_META_DATA);
        } catch (NameNotFoundException ex) {
            Log.w(LOG_TAG, ex.toString());
            return null;
        }
        int iconId = activityInfo.getIconResource();
        if (iconId == 0) return null;
        String pkg = component.getPackageName();
        Drawable drawable = pm.getDrawable(pkg, iconId, activityInfo.applicationInfo);
        if (drawable == null) {
            Log.w(LOG_TAG, "Invalid icon resource " + iconId + " for "
                    + component.flattenToShortString());
            return null;
        }
        return drawable;
    }

    /**
     * Gets the value of a string column by name.
     *
     * @param cursor Cursor to read the value from.
     * @param columnName The name of the column to read.
     * @return The value of the given column, or <code>null</null>
     *         if the cursor does not contain the given column.
     */
    public static String getColumnString(Cursor cursor, String columnName) {
        int col = cursor.getColumnIndex(columnName);
        return getStringOrNull(cursor, col);
    }

    private static String getStringOrNull(Cursor cursor, int col) {
        if (col == INVALID_INDEX) {
            return null;
        }
        try {
            return cursor.getString(col);
        } catch (Exception e) {
            Log.e(LOG_TAG,
                    "unexpected error retrieving valid column from cursor, "
                            + "did the remote process die?", e);
            return null;
        }
    }
}
