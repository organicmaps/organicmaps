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

package com.facebook.samples.profilepicture;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.*;
import com.facebook.widget.ProfilePictureView;

import java.util.Date;
import java.util.Random;

public class ProfilePictureSampleFragment extends Fragment {

    // Keeping the number of custom sizes low to prevent excessive network chatter.
    private static final int MAX_CUSTOM_SIZES = 6;
    private static final int DEFAULT_SIZE_INCREMENT = MAX_CUSTOM_SIZES / 2;
    private static final String PICTURE_SIZE_TYPE_KEY = "PictureSizeType";

    private static final String[] INTERESTING_IDS = {
        "zuck",
        // Recent Presidents and nominees
        "barackobama",
        "mittromney",
        "johnmccain",
        "johnkerry",
        "georgewbush",
        "algore",
        // Places too!
        "Disneyland",
        "SpaceNeedle",
        "TourEiffel",
        "sydneyoperahouse",
        // A selection of 1986 Mets
        "166020963458360",
        "108084865880237",
        "140447466087679",
        "111825495501392",
        // The cast of Saved by the Bell
        "108168249210849",
        "TiffaniThiessen",
        "108126672542534",
        "112886105391693",
        "MarioLopezExtra",
        "108504145837165",
        "dennishaskins",
        // Eighties bands that have been to Moscow
        "7220821999",
        "31938132882",
        "108023262558391",
        "209263392372",
        "104132506290482",
        "9721897972",
        "5461947317",
        "57084011597",
        // Three people that have never been in my kitchen
        "24408579964",
        "111980872152571",
        "112427772106500",
        // Trusted anchormen
        "113415525338717",
        "105628452803615",
        "105533779480538",
    };

    private int pictureSizeType = ProfilePictureView.CUSTOM;
    private String firstUserId;
    private Random randomGenerator;

    private ProfilePictureView profilePic;
    private Button smallerButton;
    private Button largerButton;
    private TextView sizeLabel;
    private View presetSizeView;
    private SeekBar customSizeView;
    private CheckBox cropToggle;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup parent, Bundle savedInstanceState) {
        View fragmentView = inflater.inflate(R.layout.fragment_profile_picture_sample, parent, false);

        randomGenerator = new Random((new Date()).getTime());

        profilePic = (ProfilePictureView) fragmentView.findViewById(R.id.profilepic);
        smallerButton = (Button) fragmentView.findViewById(R.id.smallerButton);
        largerButton = (Button) fragmentView.findViewById(R.id.largerButton);
        sizeLabel = (TextView) fragmentView.findViewById(R.id.sizeLabel);
        presetSizeView = fragmentView.findViewById(R.id.presetSizeView);
        customSizeView = (SeekBar) fragmentView.findViewById(R.id.customSizeView);
        cropToggle = (CheckBox) fragmentView.findViewById(R.id.squareCropToggle);

        LinearLayout container = (LinearLayout) fragmentView.findViewById(R.id.userbuttoncontainer);
        int numChildren = container.getChildCount();
        for (int i = 0; i < numChildren; i++) {
            View childView = container.getChildAt(i);
            Object tag = childView.getTag();
            if (childView instanceof Button) {
                setupUserButton((Button)childView);
                if (i == 0) {
                    // Initialize the image to the first user
                    firstUserId = tag.toString();
                }
            }
        }

        cropToggle.setOnCheckedChangeListener(new CheckBox.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton checkbox, boolean checked) {
                profilePic.setCropped(checked);
            }
        });

        final Button sizeToggle = (Button) fragmentView.findViewById(R.id.sizeToggle);
        sizeToggle.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (pictureSizeType != ProfilePictureView.CUSTOM) {
                    sizeToggle.setText(R.string.preset_size_button_text);
                    switchToCustomSize();
                } else {
                    sizeToggle.setText(R.string.custom_size_button_text);
                    switchToPresetSize(ProfilePictureView.LARGE);
                }
            }
        });

        smallerButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                switch(profilePic.getPresetSize()) {
                    case ProfilePictureView.LARGE:
                        switchToPresetSize(ProfilePictureView.NORMAL);
                        break;
                    case ProfilePictureView.NORMAL:
                        switchToPresetSize(ProfilePictureView.SMALL);
                        break;
                }
            }
        });

        largerButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                switch(profilePic.getPresetSize()) {
                    case ProfilePictureView.NORMAL:
                        switchToPresetSize(ProfilePictureView.LARGE);
                        break;
                    case ProfilePictureView.SMALL:
                        switchToPresetSize(ProfilePictureView.NORMAL);
                        break;
                }
            }
        });

        // We will fetch a new image for each change in the SeekBar. So keeping the count low
        // to prevent too much network chatter. SeekBar reports 0-max, so we will get max+1
        // notifications of change.
        customSizeView.setMax(MAX_CUSTOM_SIZES);
        customSizeView.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                updateProfilePicForCustomSizeIncrement(i);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                // NO-OP
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                // NO-OP
            }
        });

        restoreState(savedInstanceState);

        return fragmentView;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        // Store the size type since we will use that to switch the Fragment's UI
        // between CUSTOM & PRESET modes
        // Other state (userId & isCropped) will be saved/restored directly by
        // ProfilePictureView
        outState.putInt(PICTURE_SIZE_TYPE_KEY, pictureSizeType);
    }

    private void restoreState(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            // Is we have saved state, restore the Fragment to it.
            // UserId & isCropped will be restored directly by ProfilePictureView
            pictureSizeType = savedInstanceState.getInt(
                    PICTURE_SIZE_TYPE_KEY, ProfilePictureView.LARGE);

            if (pictureSizeType == ProfilePictureView.CUSTOM) {
                switchToCustomSize();
            } else {
                switchToPresetSize(pictureSizeType);
            }
        } else {
            // No saved state. Let's go to a default state
            switchToPresetSize(ProfilePictureView.LARGE);
            profilePic.setCropped(cropToggle.isChecked());

            // Setting userId last so that only one network request is sent
            profilePic.setProfileId(firstUserId);
        }
    }

    private void setupUserButton(Button b) {
        b.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Object tag = v.getTag();
                String userId = null;
                if (tag != null) {
                    userId = tag.toString();
                } else {
                    // Random id.
                    userId = INTERESTING_IDS[randomGenerator.nextInt(INTERESTING_IDS.length)];
                }
                profilePic.setProfileId(userId);
            }
        });
    }

    private void switchToCustomSize() {
        pictureSizeType = ProfilePictureView.CUSTOM;
        presetSizeView.setVisibility(View.GONE);
        customSizeView.setVisibility(View.VISIBLE);

        profilePic.setPresetSize(pictureSizeType);

        customSizeView.setProgress(DEFAULT_SIZE_INCREMENT);
        updateProfilePicForCustomSizeIncrement(DEFAULT_SIZE_INCREMENT);
    }

    private void switchToPresetSize(int sizeType) {
        customSizeView.setVisibility(View.GONE);
        presetSizeView.setVisibility(View.VISIBLE);

        switch(sizeType) {
            case ProfilePictureView.SMALL:
                largerButton.setEnabled(true);
                smallerButton.setEnabled(false);
                sizeLabel.setText(R.string.small_image_size);
                pictureSizeType = sizeType;
                break;
            case ProfilePictureView.NORMAL:
                largerButton.setEnabled(true);
                smallerButton.setEnabled(true);
                sizeLabel.setText(R.string.normal_image_size);
                pictureSizeType = sizeType;
                break;
            case ProfilePictureView.LARGE:
            default:
                largerButton.setEnabled(false);
                smallerButton.setEnabled(true);
                sizeLabel.setText(R.string.large_image_size);
                pictureSizeType = ProfilePictureView.LARGE;
                break;
        }

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1
        );

        profilePic.setLayoutParams(params);
        profilePic.setPresetSize(pictureSizeType);
    }

    private void updateProfilePicForCustomSizeIncrement(int i) {
        if (pictureSizeType != ProfilePictureView.CUSTOM) {
            return;
        }

        // This will ensure a minimum size of 51x68 and will scale the image at
        // a ratio of 3:4 (w:h) as the SeekBar is moved.
        //
        // Completely arbitrary
        //
        // NOTE: The numbers are in dips.
        float width = (i * 21) + 51;
        float height = (i * 28) + 68;

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                (int)(width * getResources().getDisplayMetrics().density),
                (int)(height * getResources().getDisplayMetrics().density));
        profilePic.setLayoutParams(params);
    }
}
