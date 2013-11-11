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

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import com.facebook.FacebookException;
import com.facebook.LoggingBehavior;
import com.facebook.android.R;
import com.facebook.internal.*;

import java.net.URISyntaxException;

/**
 * View that displays the profile photo of a supplied profile ID, while conforming
 * to user specified dimensions.
 */
public class ProfilePictureView extends FrameLayout {

    /**
     * Callback interface that will be called when a network or other error is encountered
     * while retrieving profile pictures.
     */
    public interface OnErrorListener {
        /**
         * Called when a network or other error is encountered.
         * @param error     a FacebookException representing the error that was encountered.
         */
        void onError(FacebookException error);
    }

    /**
     * Tag used when logging calls are made by ProfilePictureView
     */
    public static final String TAG = ProfilePictureView.class.getSimpleName();

    /**
     * Indicates that the specific size of the View will be set via layout params.
     * ProfilePictureView will default to NORMAL X NORMAL, if the layout params set on
     * this instance do not have a fixed size.
     * Used in calls to setPresetSize() and getPresetSize().
     * Corresponds with the preset_size Xml attribute that can be set on ProfilePictureView.
     */
    public static final int CUSTOM = -1;

    /**
     * Indicates that the profile image should fit in a SMALL X SMALL space, regardless
     * of whether the cropped or un-cropped version is chosen.
     * Used in calls to setPresetSize() and getPresetSize().
     * Corresponds with the preset_size Xml attribute that can be set on ProfilePictureView.
     */
    public static final int SMALL = -2;

    /**
     * Indicates that the profile image should fit in a NORMAL X NORMAL space, regardless
     * of whether the cropped or un-cropped version is chosen.
     * Used in calls to setPresetSize() and getPresetSize().
     * Corresponds with the preset_size Xml attribute that can be set on ProfilePictureView.
     */
    public static final int NORMAL = -3;

    /**
     * Indicates that the profile image should fit in a LARGE X LARGE space, regardless
     * of whether the cropped or un-cropped version is chosen.
     * Used in calls to setPresetSize() and getPresetSize().
     * Corresponds with the preset_size Xml attribute that can be set on ProfilePictureView.
     */
    public static final int LARGE = -4;

    private static final int MIN_SIZE = 1;
    private static final boolean IS_CROPPED_DEFAULT_VALUE = true;
    private static final String SUPER_STATE_KEY = "ProfilePictureView_superState";
    private static final String PROFILE_ID_KEY = "ProfilePictureView_profileId";
    private static final String PRESET_SIZE_KEY = "ProfilePictureView_presetSize";
    private static final String IS_CROPPED_KEY = "ProfilePictureView_isCropped";
    private static final String BITMAP_KEY = "ProfilePictureView_bitmap";
    private static final String BITMAP_WIDTH_KEY = "ProfilePictureView_width";
    private static final String BITMAP_HEIGHT_KEY = "ProfilePictureView_height";
    private static final String PENDING_REFRESH_KEY = "ProfilePictureView_refresh";

    private String profileId;
    private int queryHeight = ImageRequest.UNSPECIFIED_DIMENSION;
    private int queryWidth = ImageRequest.UNSPECIFIED_DIMENSION;
    private boolean isCropped = IS_CROPPED_DEFAULT_VALUE;
    private Bitmap imageContents;
    private ImageView image;
    private int presetSizeType = CUSTOM;
    private ImageRequest lastRequest;
    private OnErrorListener onErrorListener;
    private Bitmap customizedDefaultProfilePicture = null;

    /**
     * Constructor
     *
     * @param context Context for this View
     */
    public ProfilePictureView(Context context) {
        super(context);
        initialize(context);
    }

    /**
     * Constructor
     *
     * @param context Context for this View
     * @param attrs   AttributeSet for this View.
     *                The attribute 'preset_size' is processed here
     */
    public ProfilePictureView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize(context);
        parseAttributes(attrs);
    }

    /**
     * Constructor
     *
     * @param context  Context for this View
     * @param attrs    AttributeSet for this View.
     *                 The attribute 'preset_size' is processed here
     * @param defStyle Default style for this View
     */
    public ProfilePictureView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize(context);
        parseAttributes(attrs);
    }

    /**
     * Gets the current preset size type
     *
     * @return The current preset size type, if set; CUSTOM if not
     */
    public final int getPresetSize() {
        return presetSizeType;
    }

    /**
     * Apply a preset size to this profile photo
     *
     * @param sizeType The size type to apply: SMALL, NORMAL or LARGE
     */
    public final void setPresetSize(int sizeType) {
        switch (sizeType) {
            case SMALL:
            case NORMAL:
            case LARGE:
            case CUSTOM:
                this.presetSizeType = sizeType;
                break;

            default:
                throw new IllegalArgumentException("Must use a predefined preset size");
        }

        requestLayout();
    }

    /**
     * Indicates whether the cropped version of the profile photo has been chosen
     *
     * @return True if the cropped version is chosen, false if not.
     */
    public final boolean isCropped() {
        return isCropped;
    }

    /**
     * Sets the profile photo to be the cropped version, or the original version
     *
     * @param showCroppedVersion True to select the cropped version
     *                           False to select the standard version
     */
    public final void setCropped(boolean showCroppedVersion) {
        isCropped = showCroppedVersion;
        // No need to force the refresh since we will catch the change in required dimensions
        refreshImage(false);
    }

    /**
     * Returns the profile Id for the current profile photo
     *
     * @return The profile Id
     */
    public final String getProfileId() {
        return profileId;
    }

    /**
     * Sets the profile Id for this profile photo
     *
     * @param profileId The profileId
     *               NULL/Empty String will show the blank profile photo
     */
    public final void setProfileId(String profileId) {
        boolean force = false;
        if (Utility.isNullOrEmpty(this.profileId) || !this.profileId.equalsIgnoreCase(profileId)) {
            // Clear out the old profilePicture before requesting for the new one.
            setBlankProfilePicture();
            force = true;
        }

        this.profileId = profileId;
        refreshImage(force);
    }

    /**
     * Returns the current OnErrorListener for this instance of ProfilePictureView
     *
     * @return The OnErrorListener
     */
    public final OnErrorListener getOnErrorListener() {
        return onErrorListener;
    }

    /**
     * Sets an OnErrorListener for this instance of ProfilePictureView to call into when
     * certain exceptions occur.
     *
     * @param onErrorListener The Listener object to set
     */
    public final void setOnErrorListener(OnErrorListener onErrorListener) {
      this.onErrorListener = onErrorListener;
    }

    /**
     * The ProfilePictureView will display the provided image while the specified
     * profile is being loaded, or if the specified profile is not available.
     *
     * @param inputBitmap The bitmap to render until the actual profile is loaded.
     */
    public final void setDefaultProfilePicture(Bitmap inputBitmap) {
        customizedDefaultProfilePicture = inputBitmap;
    }

    /**
     * Overriding onMeasure to handle the case where WRAP_CONTENT might be
     * specified in the layout. Since we don't know the dimensions of the profile
     * photo, we need to handle this case specifically.
     * <p/>
     * The approach is to default to a NORMAL sized amount of space in the case that
     * a preset size is not specified. This logic is applied to both width and height
     */
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        ViewGroup.LayoutParams params = getLayoutParams();
        boolean customMeasure = false;
        int newHeight = MeasureSpec.getSize(heightMeasureSpec);
        int newWidth = MeasureSpec.getSize(widthMeasureSpec);
        if (MeasureSpec.getMode(heightMeasureSpec) != MeasureSpec.EXACTLY &&
                params.height == ViewGroup.LayoutParams.WRAP_CONTENT) {
            newHeight = getPresetSizeInPixels(true); // Default to a preset size
            heightMeasureSpec = MeasureSpec.makeMeasureSpec(newHeight, MeasureSpec.EXACTLY);
            customMeasure = true;
        }

        if (MeasureSpec.getMode(widthMeasureSpec) != MeasureSpec.EXACTLY &&
                params.width == ViewGroup.LayoutParams.WRAP_CONTENT) {
            newWidth = getPresetSizeInPixels(true); // Default to a preset size
            widthMeasureSpec = MeasureSpec.makeMeasureSpec(newWidth, MeasureSpec.EXACTLY);
            customMeasure = true;
        }

        if (customMeasure) {
            // Since we are providing custom dimensions, we need to handle the measure
            // phase from here
            setMeasuredDimension(newWidth, newHeight);
            measureChildren(widthMeasureSpec, heightMeasureSpec);
        } else {
            // Rely on FrameLayout to do the right thing
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }
    }

    /**
     * In addition to calling super.Layout(), we also attempt to get a new image that
     * is properly size for the layout dimensions
     */
    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        // See if the image needs redrawing
        refreshImage(false);
    }

    /**
     * Some of the current state is returned as a Bundle to allow quick restoration
     * of the ProfilePictureView object in scenarios like orientation changes.
     * @return a Parcelable containing the current state
     */
    @Override
    protected Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        Bundle instanceState = new Bundle();
        instanceState.putParcelable(SUPER_STATE_KEY, superState);
        instanceState.putString(PROFILE_ID_KEY, profileId);
        instanceState.putInt(PRESET_SIZE_KEY, presetSizeType);
        instanceState.putBoolean(IS_CROPPED_KEY, isCropped);
        instanceState.putParcelable(BITMAP_KEY, imageContents);
        instanceState.putInt(BITMAP_WIDTH_KEY, queryWidth);
        instanceState.putInt(BITMAP_HEIGHT_KEY, queryHeight);
        instanceState.putBoolean(PENDING_REFRESH_KEY, lastRequest != null);

        return instanceState;
    }

    /**
     * If the passed in state is a Bundle, an attempt is made to restore from it.
     * @param state a Parcelable containing the current state
     */
    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        if (state.getClass() != Bundle.class) {
            super.onRestoreInstanceState(state);
        } else {
            Bundle instanceState = (Bundle)state;
            super.onRestoreInstanceState(instanceState.getParcelable(SUPER_STATE_KEY));

            profileId = instanceState.getString(PROFILE_ID_KEY);
            presetSizeType = instanceState.getInt(PRESET_SIZE_KEY);
            isCropped = instanceState.getBoolean(IS_CROPPED_KEY);
            queryWidth = instanceState.getInt(BITMAP_WIDTH_KEY);
            queryHeight = instanceState.getInt(BITMAP_HEIGHT_KEY);

            setImageBitmap((Bitmap)instanceState.getParcelable(BITMAP_KEY));

            if (instanceState.getBoolean(PENDING_REFRESH_KEY)) {
                refreshImage(true);
            }
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        // Null out lastRequest. This way, when the response is returned, we can ascertain
        // that the view is detached and hence should not attempt to update its contents.
        lastRequest = null;
    }

    private void initialize(Context context) {
        // We only want our ImageView in here. Nothing else is permitted
        removeAllViews();

        image = new ImageView(context);

        LayoutParams imageLayout = new LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT);

        image.setLayoutParams(imageLayout);

        // We want to prevent up-scaling the image, but still have it fit within
        // the layout bounds as best as possible.
        image.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        addView(image);
    }

    private void parseAttributes(AttributeSet attrs) {
        TypedArray a = getContext().obtainStyledAttributes(attrs, R.styleable.com_facebook_profile_picture_view);
        setPresetSize(a.getInt(R.styleable.com_facebook_profile_picture_view_preset_size, CUSTOM));
        isCropped = a.getBoolean(R.styleable.com_facebook_profile_picture_view_is_cropped, IS_CROPPED_DEFAULT_VALUE);
        a.recycle();
    }

    private void refreshImage(boolean force) {
        boolean changed = updateImageQueryParameters();
        // Note: do not use Utility.isNullOrEmpty here as this will cause the Eclipse
        // Graphical Layout editor to fail in some cases
        if (profileId == null || profileId.length() == 0 ||
                ((queryWidth == ImageRequest.UNSPECIFIED_DIMENSION) &&
                        (queryHeight == ImageRequest.UNSPECIFIED_DIMENSION))) {
            setBlankProfilePicture();
        } else if (changed || force) {
            sendImageRequest(true);
        }
    }

    private void setBlankProfilePicture() {
        if (customizedDefaultProfilePicture == null) {
          int blankImageResource = isCropped() ?
                  R.drawable.com_facebook_profile_picture_blank_square :
                  R.drawable.com_facebook_profile_picture_blank_portrait;
          setImageBitmap( BitmapFactory.decodeResource(getResources(), blankImageResource));
	} else {
          // Update profile image dimensions.
          updateImageQueryParameters();
          // Resize inputBitmap to new dimensions of queryWidth and queryHeight.
          Bitmap scaledBitmap = Bitmap.createScaledBitmap(customizedDefaultProfilePicture, queryWidth, queryHeight, false);
          setImageBitmap(scaledBitmap);
	}
    }

    private void setImageBitmap(Bitmap imageBitmap) {
        if (image != null && imageBitmap != null) {
            imageContents = imageBitmap; // Hold for save-restore cycles
            image.setImageBitmap(imageBitmap);
        }
    }

    private void sendImageRequest(boolean allowCachedResponse) {
        try {
            ImageRequest.Builder requestBuilder = new ImageRequest.Builder(
                    getContext(),
                    ImageRequest.getProfilePictureUrl(profileId, queryWidth,  queryHeight));

            ImageRequest request = requestBuilder.setAllowCachedRedirects(allowCachedResponse)
                    .setCallerTag(this)
                    .setCallback(
                    new ImageRequest.Callback() {
                        @Override
                        public void onCompleted(ImageResponse response) {
                            processResponse(response);
                        }
                    })
                    .build();

            // Make sure to cancel the old request before sending the new one to prevent
            // accidental cancellation of the new request. This could happen if the URL and
            // caller tag stayed the same.
            if (lastRequest != null) {
                ImageDownloader.cancelRequest(lastRequest);
            }
            lastRequest = request;

            ImageDownloader.downloadAsync(request);
        } catch (URISyntaxException e) {
            Logger.log(LoggingBehavior.REQUESTS, Log.ERROR, TAG, e.toString());
        }
    }

    private void processResponse(ImageResponse response) {
        // First check if the response is for the right request. We may have:
        // 1. Sent a new request, thus super-ceding this one.
        // 2. Detached this view, in which case the response should be discarded.
        if (response.getRequest() == lastRequest) {
            lastRequest = null;
            Bitmap responseImage = response.getBitmap();
            Exception error = response.getError();
            if (error != null) {
                OnErrorListener listener = onErrorListener;
                if (listener != null) {
                    listener.onError(new FacebookException(
                            "Error in downloading profile picture for profileId: " + getProfileId(), error));
                } else {
                    Logger.log(LoggingBehavior.REQUESTS, Log.ERROR, TAG, error.toString());
                }
            } else if (responseImage != null) {
                setImageBitmap(responseImage);

                if (response.isCachedRedirect()) {
                    sendImageRequest(false);
                }
            }
        }
    }

    private boolean updateImageQueryParameters() {
        int newHeightPx = getHeight();
        int newWidthPx = getWidth();
        if (newWidthPx < MIN_SIZE || newHeightPx < MIN_SIZE) {
            // Not enough space laid out for this View yet. Or something else is awry.
            return false;
        }

        int presetSize = getPresetSizeInPixels(false);
        if (presetSize != ImageRequest.UNSPECIFIED_DIMENSION) {
            newWidthPx = presetSize;
            newHeightPx = presetSize;
        }

        // The cropped version is square
        // If full version is desired, then only one dimension is required.
        if (newWidthPx <= newHeightPx) {
            newHeightPx = isCropped() ? newWidthPx : ImageRequest.UNSPECIFIED_DIMENSION;
        } else {
            newWidthPx = isCropped() ? newHeightPx : ImageRequest.UNSPECIFIED_DIMENSION;
        }

        boolean changed = (newWidthPx != queryWidth) || (newHeightPx != queryHeight);

        queryWidth = newWidthPx;
        queryHeight = newHeightPx;

        return changed;
    }

    private int getPresetSizeInPixels(boolean forcePreset) {
        int dimensionId;
        switch (presetSizeType) {
            case SMALL:
                dimensionId = R.dimen.com_facebook_profilepictureview_preset_size_small;
                break;
            case NORMAL:
                dimensionId = R.dimen.com_facebook_profilepictureview_preset_size_normal;
                break;
            case LARGE:
                dimensionId = R.dimen.com_facebook_profilepictureview_preset_size_large;
                break;
            case CUSTOM:
                if (!forcePreset) {
                    return ImageRequest.UNSPECIFIED_DIMENSION;
                } else {
                    dimensionId = R.dimen.com_facebook_profilepictureview_preset_size_normal;
                    break;
                }
            default:
                return ImageRequest.UNSPECIFIED_DIMENSION;
        }

        return getResources().getDimensionPixelSize(dimensionId);
    }
}
