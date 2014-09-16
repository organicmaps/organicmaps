/**
 * Copyright 2014 Facebook, Inc.
 *
 * You are hereby granted a non-exclusive, worldwide, royalty-free license to
 * use, copy, modify, and distribute this software in source code or binary
 * form for use in connection with the web and mobile services and APIs
 * provided by Facebook.
 *
 * As with any software that integrates with the Facebook platform, your use
 * of this software is subject to the Facebook Developer Principles and
 * Policies [http://developers.facebook.com/policy/]. This copyright notice
 * shall be included in all copies or substantial portions of the software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

package com.facebook.samples.AdUnitsSample;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.facebook.ads.*;

public class AdUnitsSampleActivity extends Activity implements InterstitialAdListener {

    private RelativeLayout adViewContainer;
    private TextView adStatusLabel;
    private TextView interstitialAdStatusLabel;
    private Button loadInterstitialButton;
    private Button showInterstitialButton;

    private AdView adView;
    private InterstitialAd interstitialAd;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ad_sample);

        adViewContainer = (RelativeLayout) findViewById(R.id.adViewContainer);
        adStatusLabel = (TextView)findViewById(R.id.adStatusLabel);
        interstitialAdStatusLabel = (TextView)findViewById(R.id.interstitialAdStatusLabel);
        loadInterstitialButton = (Button)findViewById(R.id.loadInterstitialButton);
        showInterstitialButton = (Button)findViewById(R.id.showInterstitialButton);

        // When testing on a device, add its hashed ID to force test ads.
        // The hash ID is printed to log cat when running on a device and loading an ad.
        // AdSettings.addTestDevice("THE HASHED ID AS PRINTED TO LOG CAT");

        // Create a banner's ad view with a unique placement ID (generate your own on the Facebook app settings).
        // Use different ID for each ad placement in your app.
        boolean isTablet = getResources().getBoolean(R.bool.is_tablet);
        adView = new AdView(this, "YOUR_PLACEMENT_ID",
                isTablet ? AdSize.BANNER_HEIGHT_90 : AdSize.BANNER_HEIGHT_50);

        // Set a listener to get notified on changes or when the user interact with the ad.
        adView.setAdListener(this);

        // Reposition the ad and add it to the view hierarchy.
        adViewContainer.addView(adView);

        // Initiate a request to load an ad.
        adView.loadAd();

        loadInterstitialButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                interstitialAdStatusLabel.setText("Loading interstitial ad...");

                // Create the interstitial unit with a placement ID (generate your own on the Facebook app settings).
                // Use different ID for each ad placement in your app.
                interstitialAd = new InterstitialAd(AdUnitsSampleActivity.this, "YOUR_PLACEMENT_ID");

                // Set a listener to get notified on changes or when the user interact with the ad.
                interstitialAd.setAdListener(AdUnitsSampleActivity.this);

                // Load a new interstitial.
                interstitialAd.loadAd();
            }
        });

        showInterstitialButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (interstitialAd == null || !interstitialAd.isAdLoaded()) {
                    // Ad not ready to show.
                    interstitialAdStatusLabel.setText("Ad not loaded. Click load to request an ad.");
                } else {
                    // Ad was loaded, show it!
                    interstitialAd.show();
                    interstitialAdStatusLabel.setText("");
                }
            }
        });
    }

    @Override
    protected void onDestroy() {
        adView.destroy();
        adView = null;
        if (interstitialAd != null) {
            interstitialAd.destroy();
            interstitialAd = null;
        }
        super.onDestroy();
    }

    @Override
    public void onError(Ad ad, AdError error) {
        if (ad == adView) {
            adStatusLabel.setText("Ad failed to load: " + error.getErrorMessage());
        } else if (ad == interstitialAd) {
            interstitialAdStatusLabel.setText("Interstitial ad failed to load: " + error.getErrorMessage());
        }
    }

    @Override
    public void onAdLoaded(Ad ad) {
        if (ad == adView) {
            adStatusLabel.setText("");
        } else if (ad == interstitialAd) {
            interstitialAdStatusLabel.setText("Ad loaded. Click show to present!");
        }
    }

    @Override
    public void onInterstitialDisplayed(Ad ad) {
        Toast.makeText(this, "Interstitial Displayed", Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onInterstitialDismissed(Ad ad) {
        Toast.makeText(this, "Interstitial Dismissed", Toast.LENGTH_SHORT).show();

        // Cleanup.
        interstitialAd.destroy();
        interstitialAd = null;
    }

    @Override
    public void onAdClicked(Ad ad) {
        Toast.makeText(this, "Ad Clicked", Toast.LENGTH_SHORT).show();
    }
}
