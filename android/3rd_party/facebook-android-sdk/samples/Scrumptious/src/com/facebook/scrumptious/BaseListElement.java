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

package com.facebook.scrumptious;

import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.View;
import android.widget.BaseAdapter;
import com.facebook.model.OpenGraphAction;

/**
 * Base class for a list element in the Scrumptious main display, consisting of an
 * icon to the left, and a two line display to the right.
 */
public abstract class BaseListElement {

    private Drawable icon;
    private String text1;
    private String text2;
    private BaseAdapter adapter;
    private int requestCode;

    /**
     * Constructs a new list element.
     *
     * @param icon the drawable for the icon
     * @param text1 the first row of text
     * @param text2 the second row of text
     * @param requestCode the requestCode to start new Activities with
     */
    public BaseListElement(Drawable icon, String text1, String text2, int requestCode) {
        this.icon = icon;
        this.text1 = text1;
        this.text2 = text2;
        this.requestCode = requestCode;
    }

    /**
     * The Adapter associated with this list element (used for notifying that the
     * underlying dataset has changed).
     * @param adapter the adapter associated with this element
     */
    public void setAdapter(BaseAdapter adapter) {
        this.adapter = adapter;
    }

    /**
     * Returns the icon.
     *
     * @return the icon
     */
    public Drawable getIcon() {
        return icon;
    }

    /**
     * Returns the first row of text.
     *
     * @return the first row of text
     */
    public String getText1() {
        return text1;
    }

    /**
     * Returns the second row of text.
     *
     * @return the second row of text
     */
    public String getText2() {
        return text2;
    }

    /**
     * Returns the requestCode for starting new Activities.
     *
     * @return the requestCode
     */
    public int getRequestCode() {
        return requestCode;
    }

    /**
     * Sets the first row of text.
     *
     * @param text1 text to set on the first row
     */
    public void setText1(String text1) {
        this.text1 = text1;
        if (adapter != null) {
            adapter.notifyDataSetChanged();
        }
    }

    /**
     * Sets the second row of text.
     *
     * @param text2 text to set on the second row
     */
    public void setText2(String text2) {
        this.text2 = text2;
        if (adapter != null) {
            adapter.notifyDataSetChanged();
        }
    }

    /**
     * Returns the OnClickListener associated with this list element. To be
     * overridden by the subclasses.
     *
     * @return the OnClickListener associated with this list element
     */
    protected abstract View.OnClickListener getOnClickListener();

    /**
     * Populate an OpenGraphAction with the results of this list element.
     *
     * @param action the action to populate with data
     */
    protected abstract void populateOGAction(OpenGraphAction action);

    /**
     * Callback if the OnClickListener happens to launch a new Activity.
     *
     * @param data the data associated with the result
     */
    protected void onActivityResult(Intent data) {}

    /**
     * Save the state of the current element.
     *
     * @param bundle the bundle to save to
     */
    protected void onSaveInstanceState(Bundle bundle) {}

    /**
     * Restore the state from the saved bundle. Returns true if the
     * state was restored.
     *
     * @param savedState the bundle to restore from
     * @return true if state was restored
     */
    protected boolean restoreState(Bundle savedState) {
        return false;
    }

    /**
     * Notifies the associated Adapter that the underlying data has changed,
     * and to re-layout the view.
     */
    protected void notifyDataChanged() {
        adapter.notifyDataSetChanged();
    }

}
