/**
 * Copyright 2010-present Facebook.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.facebook.widget;

import android.app.Activity;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.animation.AlphaAnimation;
import android.widget.*;
import com.facebook.*;
import com.facebook.android.R;
import com.facebook.model.GraphObject;
import com.facebook.internal.SessionTracker;

import java.util.*;

/**
 * Provides functionality common to SDK UI elements that allow the user to pick one or more
 * graph objects (e.g., places, friends) from a list of possibilities. The UI is exposed as a
 * Fragment to allow to it to be included in an Activity along with other Fragments. The Fragments
 * can be configured by passing parameters as part of their Intent bundle, or (for certain
 * properties) by specifying attributes in their XML layout files.
 * <br/>
 * PickerFragments support callbacks that will be called in the event of an error, when the
 * underlying data has been changed, or when the set of selected graph objects changes.
 */
public abstract class PickerFragment<T extends GraphObject> extends Fragment {
    /**
     * The key for a boolean parameter in the fragment's Intent bundle to indicate whether the
     * picker should show pictures (if available) for the graph objects.
     */
    public static final String SHOW_PICTURES_BUNDLE_KEY = "com.facebook.widget.PickerFragment.ShowPictures";
    /**
     * The key for a String parameter in the fragment's Intent bundle to indicate which extra fields
     * beyond the default fields should be retrieved for any graph objects in the results.
     */
    public static final String EXTRA_FIELDS_BUNDLE_KEY = "com.facebook.widget.PickerFragment.ExtraFields";
    /**
     * The key for a boolean parameter in the fragment's Intent bundle to indicate whether the
     * picker should display a title bar with a Done button.
     */
    public static final String SHOW_TITLE_BAR_BUNDLE_KEY = "com.facebook.widget.PickerFragment.ShowTitleBar";
    /**
     * The key for a String parameter in the fragment's Intent bundle to indicate the text to
     * display in the title bar.
     */
    public static final String TITLE_TEXT_BUNDLE_KEY = "com.facebook.widget.PickerFragment.TitleText";
    /**
     * The key for a String parameter in the fragment's Intent bundle to indicate the text to
     * display in the Done btuton.
     */
    public static final String DONE_BUTTON_TEXT_BUNDLE_KEY = "com.facebook.widget.PickerFragment.DoneButtonText";

    private static final String SELECTION_BUNDLE_KEY = "com.facebook.android.PickerFragment.Selection";
    private static final String ACTIVITY_CIRCLE_SHOW_KEY = "com.facebook.android.PickerFragment.ActivityCircleShown";
    private static final int PROFILE_PICTURE_PREFETCH_BUFFER = 5;

    private final int layout;
    private OnErrorListener onErrorListener;
    private OnDataChangedListener onDataChangedListener;
    private OnSelectionChangedListener onSelectionChangedListener;
    private OnDoneButtonClickedListener onDoneButtonClickedListener;
    private GraphObjectFilter<T> filter;
    private boolean showPictures = true;
    private boolean showTitleBar = true;
    private ListView listView;
    HashSet<String> extraFields = new HashSet<String>();
    GraphObjectAdapter<T> adapter;
    private final Class<T> graphObjectClass;
    private LoadingStrategy loadingStrategy;
    private SelectionStrategy selectionStrategy;
    private ProgressBar activityCircle;
    private SessionTracker sessionTracker;
    private String titleText;
    private String doneButtonText;
    private TextView titleTextView;
    private Button doneButton;
    private Drawable titleBarBackground;
    private Drawable doneButtonBackground;
    private boolean appEventsLogged;

    PickerFragment(Class<T> graphObjectClass, int layout, Bundle args) {
        this.graphObjectClass = graphObjectClass;
        this.layout = layout;

        setPickerFragmentSettingsFromBundle(args);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        adapter = createAdapter();
        adapter.setFilter(new GraphObjectAdapter.Filter<T>() {
            @Override
            public boolean includeItem(T graphObject) {
                return filterIncludesItem(graphObject);
            }
        });
    }

    @Override
    public void onInflate(Activity activity, AttributeSet attrs, Bundle savedInstanceState) {
        super.onInflate(activity, attrs, savedInstanceState);
        TypedArray a = activity.obtainStyledAttributes(attrs, R.styleable.com_facebook_picker_fragment);

        setShowPictures(a.getBoolean(R.styleable.com_facebook_picker_fragment_show_pictures, showPictures));
        String extraFieldsString = a.getString(R.styleable.com_facebook_picker_fragment_extra_fields);
        if (extraFieldsString != null) {
            String[] strings = extraFieldsString.split(",");
            setExtraFields(Arrays.asList(strings));
        }

        showTitleBar = a.getBoolean(R.styleable.com_facebook_picker_fragment_show_title_bar, showTitleBar);
        titleText = a.getString(R.styleable.com_facebook_picker_fragment_title_text);
        doneButtonText = a.getString(R.styleable.com_facebook_picker_fragment_done_button_text);
        titleBarBackground = a.getDrawable(R.styleable.com_facebook_picker_fragment_title_bar_background);
        doneButtonBackground = a.getDrawable(R.styleable.com_facebook_picker_fragment_done_button_background);

        a.recycle();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        ViewGroup view = (ViewGroup) inflater.inflate(layout, container, false);

        listView = (ListView) view.findViewById(R.id.com_facebook_picker_list_view);
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
                onListItemClick((ListView) parent, v, position);
            }
        });
        listView.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                // We don't actually do anything differently on long-clicks, but setting the listener
                // enables the selector transition that we have for visual consistency with the
                // Facebook app's pickers.
                return false;
            }
        });
        listView.setOnScrollListener(onScrollListener);

        activityCircle = (ProgressBar) view.findViewById(R.id.com_facebook_picker_activity_circle);

        setupViews(view);

        listView.setAdapter(adapter);

        return view;
    }

    @Override
    public void onActivityCreated(final Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        sessionTracker = new SessionTracker(getActivity(), new Session.StatusCallback() {
            @Override
            public void call(Session session, SessionState state, Exception exception) {
                if (!session.isOpened()) {
                    // When a session is closed, we want to clear out our data so it is not visible to subsequent users
                    clearResults();
                }
            }
        });

        setSettingsFromBundle(savedInstanceState);

        loadingStrategy = createLoadingStrategy();
        loadingStrategy.attach(adapter);

        selectionStrategy = createSelectionStrategy();
        selectionStrategy.readSelectionFromBundle(savedInstanceState, SELECTION_BUNDLE_KEY);

        // Should we display a title bar? (We need to do this after we've retrieved our bundle settings.)
        if (showTitleBar) {
            inflateTitleBar((ViewGroup) getView());
        }

        if (activityCircle != null && savedInstanceState != null) {
            boolean shown = savedInstanceState.getBoolean(ACTIVITY_CIRCLE_SHOW_KEY, false);
            if (shown) {
                displayActivityCircle();
            } else {
                // Should be hidden already, but just to be sure.
                hideActivityCircle();
            }
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();

        listView.setOnScrollListener(null);
        listView.setAdapter(null);

        loadingStrategy.detach();
        sessionTracker.stopTracking();
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        saveSettingsToBundle(outState);
        selectionStrategy.saveSelectionToBundle(outState, SELECTION_BUNDLE_KEY);
        if (activityCircle != null) {
            outState.putBoolean(ACTIVITY_CIRCLE_SHOW_KEY, activityCircle.getVisibility() == View.VISIBLE);
        }
    }

    @Override
    public void onStop() {
        if (!appEventsLogged) {
            logAppEvents(false);
        }
        super.onStop();
    }

    @Override
    public void setArguments(Bundle args) {
        super.setArguments(args);
        setSettingsFromBundle(args);
    }

    /**
     * Gets the current OnDataChangedListener for this fragment, which will be called whenever
     * the underlying data being displaying in the picker has changed.
     *
     * @return the OnDataChangedListener, or null if there is none
     */
    public OnDataChangedListener getOnDataChangedListener() {
        return onDataChangedListener;
    }

    /**
     * Sets the current OnDataChangedListener for this fragment, which will be called whenever
     * the underlying data being displaying in the picker has changed.
     *
     * @param onDataChangedListener the OnDataChangedListener, or null if there is none
     */
    public void setOnDataChangedListener(OnDataChangedListener onDataChangedListener) {
        this.onDataChangedListener = onDataChangedListener;
    }

    /**
     * Gets the current OnSelectionChangedListener for this fragment, which will be called
     * whenever the user selects or unselects a graph object in the list.
     *
     * @return the OnSelectionChangedListener, or null if there is none
     */
    public OnSelectionChangedListener getOnSelectionChangedListener() {
        return onSelectionChangedListener;
    }

    /**
     * Sets the current OnSelectionChangedListener for this fragment, which will be called
     * whenever the user selects or unselects a graph object in the list.
     *
     * @param onSelectionChangedListener the OnSelectionChangedListener, or null if there is none
     */
    public void setOnSelectionChangedListener(
            OnSelectionChangedListener onSelectionChangedListener) {
        this.onSelectionChangedListener = onSelectionChangedListener;
    }

    /**
     * Gets the current OnDoneButtonClickedListener for this fragment, which will be called
     * when the user clicks the Done button.
     *
     * @return the OnDoneButtonClickedListener, or null if there is none
     */
    public OnDoneButtonClickedListener getOnDoneButtonClickedListener() {
        return onDoneButtonClickedListener;
    }

    /**
     * Sets the current OnDoneButtonClickedListener for this fragment, which will be called
     * when the user clicks the Done button. This will only be possible if the title bar is
     * being shown in this fragment.
     *
     * @param onDoneButtonClickedListener the OnDoneButtonClickedListener, or null if there is none
     */
    public void setOnDoneButtonClickedListener(OnDoneButtonClickedListener onDoneButtonClickedListener) {
        this.onDoneButtonClickedListener = onDoneButtonClickedListener;
    }

    /**
     * Gets the current OnErrorListener for this fragment, which will be called in the event
     * of network or other errors encountered while populating the graph objects in the list.
     *
     * @return the OnErrorListener, or null if there is none
     */
    public OnErrorListener getOnErrorListener() {
        return onErrorListener;
    }

    /**
     * Sets the current OnErrorListener for this fragment, which will be called in the event
     * of network or other errors encountered while populating the graph objects in the list.
     *
     * @param onErrorListener the OnErrorListener, or null if there is none
     */
    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    /**
     * Gets the current filter for this fragment, which will be called for each graph object
     * returned from the service to determine if it should be displayed in the list.
     * If no filter is specified, all retrieved graph objects will be displayed.
     *
     * @return the GraphObjectFilter, or null if there is none
     */
    public GraphObjectFilter<T> getFilter() {
        return filter;
    }

    /**
     * Sets the current filter for this fragment, which will be called for each graph object
     * returned from the service to determine if it should be displayed in the list.
     * If no filter is specified, all retrieved graph objects will be displayed.
     *
     * @param filter the GraphObjectFilter, or null if there is none
     */
    public void setFilter(GraphObjectFilter<T> filter) {
        this.filter = filter;
    }

    /**
     * Gets the Session to use for any Facebook requests this fragment will make.
     *
     * @return the Session that will be used for any Facebook requests, or null if there is none
     */
    public Session getSession() {
        return sessionTracker.getSession();
    }

    /**
     * Sets the Session to use for any Facebook requests this fragment will make. If the
     * parameter is null, the fragment will use the current active session, if any.
     *
     * @param session the Session to use for Facebook requests, or null to use the active session
     */
    public void setSession(Session session) {
        sessionTracker.setSession(session);
    }

    /**
     * Gets whether to display pictures, if available, for displayed graph objects.
     *
     * @return true if pictures should be displayed, false if not
     */
    public boolean getShowPictures() {
        return showPictures;
    }

    /**
     * Sets whether to display pictures, if available, for displayed graph objects.
     *
     * @param showPictures true if pictures should be displayed, false if not
     */
    public void setShowPictures(boolean showPictures) {
        this.showPictures = showPictures;
    }

    /**
     * Gets the extra fields to request for the retrieved graph objects.
     *
     * @return the extra fields to request
     */
    public Set<String> getExtraFields() {
        return new HashSet<String>(extraFields);
    }

    /**
     * Sets the extra fields to request for the retrieved graph objects.
     *
     * @param fields the extra fields to request
     */
    public void setExtraFields(Collection<String> fields) {
        extraFields = new HashSet<String>();
        if (fields != null) {
            extraFields.addAll(fields);
        }
    }

    /**
     * Sets whether to show a title bar with a Done button. This must be
     * called prior to the Fragment going through its creation lifecycle to have an effect.
     *
     * @param showTitleBar true if a title bar should be displayed, false if not
     */
    public void setShowTitleBar(boolean showTitleBar) {
        this.showTitleBar = showTitleBar;
    }

    /**
     * Gets whether to show a title bar with a Done button. The default is true.
     *
     * @return true if a title bar will be shown, false if not.
     */
    public boolean getShowTitleBar() {
        return showTitleBar;
    }

    /**
     * Sets the text to show in the title bar, if a title bar is to be shown. This must be
     * called prior to the Fragment going through its creation lifecycle to have an effect, or
     * the default will be used.
     *
     * @param titleText the text to show in the title bar
     */
    public void setTitleText(String titleText) {
        this.titleText = titleText;
    }

    /**
     * Gets the text to show in the title bar, if a title bar is to be shown.
     *
     * @return the text to show in the title bar
     */
    public String getTitleText() {
        if (titleText == null) {
            titleText = getDefaultTitleText();
        }
        return titleText;
    }

    /**
     * Sets the text to show in the Done button, if a title bar is to be shown. This must be
     * called prior to the Fragment going through its creation lifecycle to have an effect, or
     * the default will be used.
     *
     * @param doneButtonText the text to show in the Done button
     */
    public void setDoneButtonText(String doneButtonText) {
        this.doneButtonText = doneButtonText;
    }

    /**
     * Gets the text to show in the Done button, if a title bar is to be shown.
     *
     * @return the text to show in the Done button
     */
    public String getDoneButtonText() {
        if (doneButtonText == null) {
            doneButtonText = getDefaultDoneButtonText();
        }
        return doneButtonText;
    }

    /**
     * Causes the picker to load data from the service and display it to the user.
     *
     * @param forceReload if true, data will be loaded even if there is already data being displayed (or loading);
     *                    if false, data will not be re-loaded if it is already displayed (or loading)
     */
    public void loadData(boolean forceReload) {
        if (!forceReload && loadingStrategy.isDataPresentOrLoading()) {
            return;
        }
        loadDataSkippingRoundTripIfCached();
    }

    /**
     * Updates the properties of the PickerFragment based on the contents of the supplied Bundle;
     * calling Activities may use this to pass additional configuration information to the
     * PickerFragment beyond what is specified in its XML layout.
     *
     * @param inState a Bundle containing keys corresponding to properties of the PickerFragment
     */
    public void setSettingsFromBundle(Bundle inState) {
        setPickerFragmentSettingsFromBundle(inState);
    }

    void setupViews(ViewGroup view) {
    }

    boolean filterIncludesItem(T graphObject) {
        if (filter != null) {
            return filter.includeItem(graphObject);
        }
        return true;
    }

    List<T> getSelectedGraphObjects() {
        return adapter.getGraphObjectsById(selectionStrategy.getSelectedIds());
    }

    void setSelectedGraphObjects(List<String> objectIds) {
        for(String objectId : objectIds) {
            if(!this.selectionStrategy.isSelected(objectId)) {
                this.selectionStrategy.toggleSelection(objectId);
            }
        }
    }

    void saveSettingsToBundle(Bundle outState) {
        outState.putBoolean(SHOW_PICTURES_BUNDLE_KEY, showPictures);
        if (!extraFields.isEmpty()) {
            outState.putString(EXTRA_FIELDS_BUNDLE_KEY, TextUtils.join(",", extraFields));
        }
        outState.putBoolean(SHOW_TITLE_BAR_BUNDLE_KEY, showTitleBar);
        outState.putString(TITLE_TEXT_BUNDLE_KEY, titleText);
        outState.putString(DONE_BUTTON_TEXT_BUNDLE_KEY, doneButtonText);
    }

    abstract Request getRequestForLoadData(Session session);

    abstract PickerFragmentAdapter<T> createAdapter();

    abstract LoadingStrategy createLoadingStrategy();

    abstract SelectionStrategy createSelectionStrategy();

    void onLoadingData() {
    }

    String getDefaultTitleText() {
        return null;
    }

    String getDefaultDoneButtonText() {
        return getString(R.string.com_facebook_picker_done_button_text);
    }

    void displayActivityCircle() {
        if (activityCircle != null) {
            layoutActivityCircle();
            activityCircle.setVisibility(View.VISIBLE);
        }
    }

    void layoutActivityCircle() {
        // If we've got no data, make the activity circle full-opacity. Otherwise we'll dim it to avoid
        //  cluttering the UI.
        float alpha = (!adapter.isEmpty()) ? .25f : 1.0f;
        setAlpha(activityCircle, alpha);
    }

    void hideActivityCircle() {
        if (activityCircle != null) {
            // We use an animation to dim the activity circle; need to clear this or it will remain visible.
            activityCircle.clearAnimation();
            activityCircle.setVisibility(View.INVISIBLE);
        }
    }

    void setSelectionStrategy(SelectionStrategy selectionStrategy) {
        if (selectionStrategy != this.selectionStrategy) {
            this.selectionStrategy = selectionStrategy;
            if (adapter != null) {
                // Adapter should cause a re-render.
                adapter.notifyDataSetChanged();
            }
        }
    }

    void logAppEvents(boolean doneButtonClicked) {
    }

    private static void setAlpha(View view, float alpha) {
        // Set the alpha appropriately (setAlpha is API >= 11, this technique works on all API levels).
        AlphaAnimation alphaAnimation = new AlphaAnimation(alpha, alpha);
        alphaAnimation.setDuration(0);
        alphaAnimation.setFillAfter(true);
        view.startAnimation(alphaAnimation);
    }


    private void setPickerFragmentSettingsFromBundle(Bundle inState) {
        // We do this in a separate non-overridable method so it is safe to call from the constructor.
        if (inState != null) {
            showPictures = inState.getBoolean(SHOW_PICTURES_BUNDLE_KEY, showPictures);
            String extraFieldsString = inState.getString(EXTRA_FIELDS_BUNDLE_KEY);
            if (extraFieldsString != null) {
                String[] strings = extraFieldsString.split(",");
                setExtraFields(Arrays.asList(strings));
            }
            showTitleBar = inState.getBoolean(SHOW_TITLE_BAR_BUNDLE_KEY, showTitleBar);
            String titleTextString = inState.getString(TITLE_TEXT_BUNDLE_KEY);
            if (titleTextString != null) {
                titleText = titleTextString;
                if (titleTextView != null) {
                    titleTextView.setText(titleText);
                }
            }
            String doneButtonTextString = inState.getString(DONE_BUTTON_TEXT_BUNDLE_KEY);
            if (doneButtonTextString != null) {
                doneButtonText = doneButtonTextString;
                if (doneButton != null) {
                    doneButton.setText(doneButtonText);
                }
            }
        }
    }

    private void inflateTitleBar(ViewGroup view) {
        ViewStub stub = (ViewStub) view.findViewById(R.id.com_facebook_picker_title_bar_stub);
        if (stub != null) {
            View titleBar = stub.inflate();

            final RelativeLayout.LayoutParams layoutParams = new RelativeLayout.LayoutParams(
                    RelativeLayout.LayoutParams.MATCH_PARENT,
                    RelativeLayout.LayoutParams.MATCH_PARENT);
            layoutParams.addRule(RelativeLayout.BELOW, R.id.com_facebook_picker_title_bar);
            listView.setLayoutParams(layoutParams);

            if (titleBarBackground != null) {
                titleBar.setBackgroundDrawable(titleBarBackground);
            }

            doneButton = (Button) view.findViewById(R.id.com_facebook_picker_done_button);
            if (doneButton != null) {
                doneButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        logAppEvents(true);
                        appEventsLogged = true;

                        if (onDoneButtonClickedListener != null) {
                            onDoneButtonClickedListener.onDoneButtonClicked(PickerFragment.this);
                        }
                    }
                });

                if (getDoneButtonText() != null) {
                    doneButton.setText(getDoneButtonText());
                }

                if (doneButtonBackground != null) {
                    doneButton.setBackgroundDrawable(doneButtonBackground);
                }
            }

            titleTextView = (TextView) view.findViewById(R.id.com_facebook_picker_title);
            if (titleTextView != null) {
                if (getTitleText() != null) {
                    titleTextView.setText(getTitleText());
                }
            }
        }
    }

    private void onListItemClick(ListView listView, View v, int position) {
        @SuppressWarnings("unchecked")
        T graphObject = (T) listView.getItemAtPosition(position);
        String id = adapter.getIdOfGraphObject(graphObject);
        selectionStrategy.toggleSelection(id);
        adapter.notifyDataSetChanged();

        if (onSelectionChangedListener != null) {
            onSelectionChangedListener.onSelectionChanged(PickerFragment.this);
        }
    }

    private void loadDataSkippingRoundTripIfCached() {
        clearResults();

        Request request = getRequestForLoadData(getSession());
        if (request != null) {
            onLoadingData();
            loadingStrategy.startLoading(request);
        }
    }

    private void clearResults() {
        if (adapter != null) {
            boolean wasSelection = !selectionStrategy.isEmpty();
            boolean wasData = !adapter.isEmpty();

            loadingStrategy.clearResults();
            selectionStrategy.clear();
            adapter.notifyDataSetChanged();

            // Tell anyone who cares the data and selection has changed, if they have.
            if (wasData && onDataChangedListener != null) {
                onDataChangedListener.onDataChanged(PickerFragment.this);
            }
            if (wasSelection && onSelectionChangedListener != null) {
                onSelectionChangedListener.onSelectionChanged(PickerFragment.this);
            }
        }
    }

    void updateAdapter(SimpleGraphObjectCursor<T> data) {
        if (adapter != null) {
            // As we fetch additional results and add them to the table, we do not
            // want the items displayed jumping around seemingly at random, frustrating the user's
            // attempts at scrolling, etc. Since results may be added anywhere in
            // the table, we choose to try to keep the first visible row in a fixed
            // position (from the user's perspective). We try to keep it positioned at
            // the same offset from the top of the screen so adding new items seems
            // smoother, as opposed to having it "snap" to a multiple of row height

            // We use the second row, to give context above and below it and avoid
            // cases where the first row is only barely visible, thus providing little context.
            // The exception is where the very first row is visible, in which case we use that.
            View view = listView.getChildAt(1);
            int anchorPosition = listView.getFirstVisiblePosition();
            if (anchorPosition > 0) {
                anchorPosition++;
            }
            GraphObjectAdapter.SectionAndItem<T> anchorItem = adapter.getSectionAndItem(anchorPosition);
            final int top = (view != null &&
                    anchorItem.getType() != GraphObjectAdapter.SectionAndItem.Type.ACTIVITY_CIRCLE) ?
                    view.getTop() : 0;

            // Now actually add the results.
            boolean dataChanged = adapter.changeCursor(data);

            if (view != null && anchorItem != null) {
                // Put the item back in the same spot it was.
                final int newPositionOfItem = adapter.getPosition(anchorItem.sectionKey, anchorItem.graphObject);
                if (newPositionOfItem != -1) {
                    listView.setSelectionFromTop(newPositionOfItem, top);
                }
            }

            if (dataChanged && onDataChangedListener != null) {
                onDataChangedListener.onDataChanged(PickerFragment.this);
            }
        }
    }

    private void reprioritizeDownloads() {
        int lastVisibleItem = listView.getLastVisiblePosition();
        if (lastVisibleItem >= 0) {
            int firstVisibleItem = listView.getFirstVisiblePosition();
            adapter.prioritizeViewRange(firstVisibleItem, lastVisibleItem, PROFILE_PICTURE_PREFETCH_BUFFER);
        }
    }

    private ListView.OnScrollListener onScrollListener = new ListView.OnScrollListener() {
        @Override
        public void onScrollStateChanged(AbsListView view, int scrollState) {
        }

        @Override
        public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
            reprioritizeDownloads();
        }
    };

    /**
     * Callback interface that will be called when a network or other error is encountered
     * while retrieving graph objects.
     */
    public interface OnErrorListener {
        /**
         * Called when a network or other error is encountered.
         *
         * @param error a FacebookException representing the error that was encountered.
         */
        void onError(PickerFragment<?> fragment, FacebookException error);
    }

    /**
     * Callback interface that will be called when the underlying data being displayed in the
     * picker has been updated.
     */
    public interface OnDataChangedListener {
        /**
         * Called when the set of data being displayed in the picker has changed.
         */
        void onDataChanged(PickerFragment<?> fragment);
    }

    /**
     * Callback interface that will be called when the user selects or unselects graph objects
     * in the picker.
     */
    public interface OnSelectionChangedListener {
        /**
         * Called when the user selects or unselects graph objects in the picker.
         */
        void onSelectionChanged(PickerFragment<?> fragment);
    }

    /**
     * Callback interface that will be called when the user clicks the Done button on the
     * title bar.
     */
    public interface OnDoneButtonClickedListener {
        /**
         * Called when the user clicks the Done button.
         */
        void onDoneButtonClicked(PickerFragment<?> fragment);
    }

    /**
     * Callback interface that will be called to determine if a graph object should be displayed.
     *
     * @param <T>
     */
    public interface GraphObjectFilter<T> {
        /**
         * Called to determine if a graph object should be displayed.
         *
         * @param graphObject the graph object
         * @return true to display the graph object, false to hide it
         */
        boolean includeItem(T graphObject);
    }

    abstract class LoadingStrategy {
        protected final static int CACHED_RESULT_REFRESH_DELAY = 2 * 1000;

        protected GraphObjectPagingLoader<T> loader;
        protected GraphObjectAdapter<T> adapter;

        public void attach(GraphObjectAdapter<T> adapter) {
            loader = (GraphObjectPagingLoader<T>) getLoaderManager().initLoader(0, null,
                    new LoaderManager.LoaderCallbacks<SimpleGraphObjectCursor<T>>() {
                        @Override
                        public Loader<SimpleGraphObjectCursor<T>> onCreateLoader(int id, Bundle args) {
                            return LoadingStrategy.this.onCreateLoader();
                        }

                        @Override
                        public void onLoadFinished(Loader<SimpleGraphObjectCursor<T>> loader,
                                SimpleGraphObjectCursor<T> data) {
                            if (loader != LoadingStrategy.this.loader) {
                                throw new FacebookException("Received callback for unknown loader.");
                            }
                            LoadingStrategy.this.onLoadFinished((GraphObjectPagingLoader<T>) loader, data);
                        }

                        @Override
                        public void onLoaderReset(Loader<SimpleGraphObjectCursor<T>> loader) {
                            if (loader != LoadingStrategy.this.loader) {
                                throw new FacebookException("Received callback for unknown loader.");
                            }
                            LoadingStrategy.this.onLoadReset((GraphObjectPagingLoader<T>) loader);
                        }
                    });

            loader.setOnErrorListener(new GraphObjectPagingLoader.OnErrorListener() {
                @Override
                public void onError(FacebookException error, GraphObjectPagingLoader<?> loader) {
                    hideActivityCircle();
                    if (onErrorListener != null) {
                        onErrorListener.onError(PickerFragment.this, error);
                    }
                }
            });

            this.adapter = adapter;
            // Tell the adapter about any data we might already have.
            this.adapter.changeCursor(loader.getCursor());
            this.adapter.setOnErrorListener(new GraphObjectAdapter.OnErrorListener() {
                @Override
                public void onError(GraphObjectAdapter<?> adapter, FacebookException error) {
                    if (onErrorListener != null) {
                        onErrorListener.onError(PickerFragment.this, error);
                    }
                }
            });
        }

        public void detach() {
            adapter.setDataNeededListener(null);
            adapter.setOnErrorListener(null);
            loader.setOnErrorListener(null);

            loader = null;
            adapter = null;
        }

        public void clearResults() {
            if (loader != null) {
                loader.clearResults();
            }
        }

        public void startLoading(Request request) {
            if (loader != null) {
                loader.startLoading(request, canSkipRoundTripIfCached());
                onStartLoading(loader, request);
            }
        }

        public boolean isDataPresentOrLoading() {
            return !adapter.isEmpty() || loader.isLoading();
        }

        protected GraphObjectPagingLoader<T> onCreateLoader() {
            return new GraphObjectPagingLoader<T>(getActivity(), graphObjectClass);
        }

        protected void onStartLoading(GraphObjectPagingLoader<T> loader, Request request) {
            displayActivityCircle();
        }

        protected void onLoadReset(GraphObjectPagingLoader<T> loader) {
            adapter.changeCursor(null);
        }

        protected void onLoadFinished(GraphObjectPagingLoader<T> loader, SimpleGraphObjectCursor<T> data) {
            updateAdapter(data);
        }

        protected boolean canSkipRoundTripIfCached() {
            return true;
        }
    }

    abstract class SelectionStrategy {
        abstract boolean isSelected(String id);

        abstract void toggleSelection(String id);

        abstract Collection<String> getSelectedIds();

        abstract void clear();

        abstract boolean isEmpty();

        abstract boolean shouldShowCheckBoxIfUnselected();

        abstract void saveSelectionToBundle(Bundle outBundle, String key);

        abstract void readSelectionFromBundle(Bundle inBundle, String key);
    }

    class SingleSelectionStrategy extends SelectionStrategy {
        private String selectedId;

        public Collection<String> getSelectedIds() {
            return Arrays.asList(new String[]{selectedId});
        }

        @Override
        boolean isSelected(String id) {
            return selectedId != null && id != null && selectedId.equals(id);
        }

        @Override
        void toggleSelection(String id) {
            if (selectedId != null && selectedId.equals(id)) {
                selectedId = null;
            } else {
                selectedId = id;
            }
        }

        @Override
        void saveSelectionToBundle(Bundle outBundle, String key) {
            if (!TextUtils.isEmpty(selectedId)) {
                outBundle.putString(key, selectedId);
            }
        }

        @Override
        void readSelectionFromBundle(Bundle inBundle, String key) {
            if (inBundle != null) {
                selectedId = inBundle.getString(key);
            }
        }

        @Override
        public void clear() {
            selectedId = null;
        }

        @Override
        boolean isEmpty() {
            return selectedId == null;
        }

        @Override
        boolean shouldShowCheckBoxIfUnselected() {
            return false;
        }
    }

    class MultiSelectionStrategy extends SelectionStrategy {
        private Set<String> selectedIds = new HashSet<String>();

        public Collection<String> getSelectedIds() {
            return selectedIds;
        }

        @Override
        boolean isSelected(String id) {
            return id != null && selectedIds.contains(id);
        }

        @Override
        void toggleSelection(String id) {
            if (id != null) {
                if (selectedIds.contains(id)) {
                    selectedIds.remove(id);
                } else {
                    selectedIds.add(id);
                }
            }
        }

        @Override
        void saveSelectionToBundle(Bundle outBundle, String key) {
            if (!selectedIds.isEmpty()) {
                String ids = TextUtils.join(",", selectedIds);
                outBundle.putString(key, ids);
            }
        }

        @Override
        void readSelectionFromBundle(Bundle inBundle, String key) {
            if (inBundle != null) {
                String ids = inBundle.getString(key);
                if (ids != null) {
                    String[] splitIds = TextUtils.split(ids, ",");
                    selectedIds.clear();
                    Collections.addAll(selectedIds, splitIds);
                }
            }
        }

        @Override
        public void clear() {
            selectedIds.clear();
        }

        @Override
        boolean isEmpty() {
            return selectedIds.isEmpty();
        }

        @Override
        boolean shouldShowCheckBoxIfUnselected() {
            return true;
        }
    }

    abstract class PickerFragmentAdapter<U extends GraphObject> extends GraphObjectAdapter<T> {
        public PickerFragmentAdapter(Context context) {
            super(context);
        }

        @Override
        boolean isGraphObjectSelected(String graphObjectId) {
            return selectionStrategy.isSelected(graphObjectId);
        }

        @Override
        void updateCheckboxState(CheckBox checkBox, boolean graphObjectSelected) {
            checkBox.setChecked(graphObjectSelected);
            int visible = (graphObjectSelected || selectionStrategy
                    .shouldShowCheckBoxIfUnselected()) ? View.VISIBLE : View.GONE;
            checkBox.setVisibility(visible);
        }
    }
}
