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

package com.facebook.samples.switchuser;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.*;
import android.widget.*;
import com.facebook.model.GraphUser;
import com.facebook.widget.ProfilePictureView;
import com.facebook.SessionLoginBehavior;

import java.util.ArrayList;
import java.util.Arrays;

public class SettingsFragment extends ListFragment {

    public static final String TAG = "SettingsFragment";

    private static final String CURRENT_SLOT_KEY = "CurrentSlot";

    private SlotManager slotManager;
    private OnSlotChangedListener slotChangedListener;
    private boolean hasPendingNotifySlotChanged;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        slotManager = new SlotManager();
        slotManager.restore(
                getActivity(),
                savedInstanceState != null ?
                        savedInstanceState.getInt(CURRENT_SLOT_KEY, SlotManager.NO_SLOT) :
                        SlotManager.NO_SLOT);
        ArrayList<Slot> slotList = new ArrayList<Slot>(
                Arrays.asList(slotManager.getAllSlots()));

        setListAdapter(new SlotAdapter(slotList));
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup parent, Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, parent, savedInstanceState);
        registerForContextMenu(view.findViewById(android.R.id.list));

        return view;
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenu.ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, view, menuInfo);
        getActivity().getMenuInflater().inflate(R.menu.context_settings, menu);
    }

    @Override
    public void onListItemClick(ListView l, View view, int position, long id) {
        slotManager.toggleSlot(position);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();
        SlotAdapter adapter = (SlotAdapter) getListAdapter();
        Slot slot = adapter.getItem(info.position);

        switch (item.getItemId()) {
            case R.id.menu_item_clear_slot:
                if (slot.getUserId() != null) {
                    // Clear out data that this app stored in the cache
                    // Not calling Session.closeAndClearTokenInformation() because we have additional
                    // data stored in the cache.
                    slot.clear();
                    if (slot == slotManager.getSelectedSlot()) {
                        slotManager.toggleSlot(info.position);
                    }

                    updateListView();
                }
                return true;
        }

        return super.onContextItemSelected(item);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putInt(CURRENT_SLOT_KEY, slotManager.getSelectedSlotNumber());
    }

    public void setSlotChangedListener(OnSlotChangedListener listener) {
        slotChangedListener = listener;
        if (listener != null && hasPendingNotifySlotChanged) {
            notifySlotChanged();
            hasPendingNotifySlotChanged = false;
        }
    }

    public void updateViewForUser(GraphUser user) {
        if (slotManager == null) {
            // Fragment has not had onCreate called yet.
            return;
        }

        if (user != null) {
            Slot s = slotManager.getSelectedSlot();
            if (s != null) {
                s.update(user);
            }
        } else {
            // If the user is null, then there isn't an actively selected
            // user. This can happen if the user cancelled a login. So make sure that
            // SlotManager is updated properly.
            slotManager.setSelectedSlotNumber(SlotManager.NO_SLOT);
        }

        updateListView();
    }

    private void notifySlotChanged() {
        OnSlotChangedListener listener = slotChangedListener;
        if (listener != null) {
            Slot newSlot = slotManager.getSelectedSlot();
            listener.onSlotChanged(newSlot);
        } else {
            hasPendingNotifySlotChanged = true;
        }
    }

    private void updateListView() {
        SlotAdapter adapter = (SlotAdapter) getListAdapter();
        adapter.notifyDataSetChanged();
    }

    public interface OnSlotChangedListener {
        void onSlotChanged(Slot newSlot);
    }

    private class SlotAdapter extends ArrayAdapter<Slot> {

        public SlotAdapter(ArrayList<Slot> slots) {
            super(getActivity(), android.R.layout.simple_list_item_1, slots);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (null == convertView) {
                convertView = getActivity().getLayoutInflater()
                        .inflate(R.layout.list_item_user, null);
            }

            Slot slot = getItem(position);
            String userName = slot.getUserName();
            if (userName == null) {
                userName = getString(R.string.empty_slot);
            }

            ProfilePictureView profilePictureView = (ProfilePictureView) convertView.findViewById(
                    R.id.slotPic);
            profilePictureView.setCropped(true);
            profilePictureView.setProfileId(slot.getUserId());

            TextView userNameTextView = (TextView) convertView.findViewById(
                    R.id.slotUserName);
            userNameTextView.setText(userName);

            CheckBox currentUserCheckBox = (CheckBox) convertView.findViewById(
                    R.id.currentUserIndicator);
            currentUserCheckBox.setChecked(slotManager.getSelectedSlot() == slot);

            return convertView;
        }

    }

    private class SlotManager {
        static final int NO_SLOT = -1;

        private final static int MAX_SLOTS = 4;

        private static final String SETTINGS_CURRENT_SLOT_KEY = "CurrentSlot";
        private static final String SETTINGS_NAME = "UserManagerSettings";

        private SharedPreferences settings;
        private int selectedSlotNumber = NO_SLOT;

        private Slot[] slots;

        void restore(Context context, int oldSelectedSlot) {
            if (context == null) {
                throw new IllegalArgumentException("context cannot be null");
            }

            Context applicationContext = context.getApplicationContext();
            applicationContext = (applicationContext == null) ? context : applicationContext;

            slots = new Slot[MAX_SLOTS];
            for (int i = 0; i < MAX_SLOTS; i++) {
                SessionLoginBehavior loginBehavior = (i == 0) ?
                        SessionLoginBehavior.SSO_WITH_FALLBACK :
                        SessionLoginBehavior.SUPPRESS_SSO;
                slots[i] = new Slot(applicationContext, i, loginBehavior);
            }

            // Restore the last known state from when the app ran last.
            settings = applicationContext.getSharedPreferences(SETTINGS_NAME, Context.MODE_PRIVATE);
            int savedSlotNumber = settings.getInt(SETTINGS_CURRENT_SLOT_KEY, NO_SLOT);
            if (savedSlotNumber != NO_SLOT && savedSlotNumber != oldSelectedSlot) {
                // This will trigger the full flow of creating and opening the right session
                toggleSlot(savedSlotNumber);
            } else {
                // We already knew which slot was selected. So don't notify that a new slot was
                // selected since that will close out the old session and recreate a new one. And
                // doing so will have the effect of clearing out state like the profile pic.
                setSelectedSlotNumber(savedSlotNumber);
            }
        }

        void toggleSlot(int slot) {
            validateSlot(slot);

            if (slot == selectedSlotNumber) {
                setSelectedSlotNumber(NO_SLOT);
            } else {
                setSelectedSlotNumber(slot);
            }

            notifySlotChanged();
        }

        Slot getSelectedSlot() {
            if (selectedSlotNumber == NO_SLOT) {
                return null;
            } else {
                return getSlot(selectedSlotNumber);
            }
        }

        int getSelectedSlotNumber() {
            return selectedSlotNumber;
        }

        Slot[] getAllSlots() {
            return slots;
        }

        Slot getSlot(int slot) {
            validateSlot(slot);

            return slots[slot];
        }

        private void setSelectedSlotNumber(int slot) {
            // Store the selected slot number for when the app is closed and restarted
            settings.edit().putInt(SETTINGS_CURRENT_SLOT_KEY, slot).commit();
            selectedSlotNumber = slot;
        }

        private void validateSlot(int slot) {
            if (slot <= NO_SLOT || slot >= MAX_SLOTS) {
                throw new IllegalArgumentException(
                        String.format("Choose a slot between 0 and %d inclusively", MAX_SLOTS-1));
            }
        }
    }
}
