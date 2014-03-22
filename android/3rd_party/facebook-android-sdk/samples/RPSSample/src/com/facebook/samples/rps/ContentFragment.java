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

package com.facebook.samples.rps;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

public class ContentFragment extends Fragment {
    public static final String CONTENT_INDEX_KEY = "content";

    private TextView title;
    private ImageView image;
    private Button playButton;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.content_fragment, container, false);
        title = (TextView) view.findViewById(R.id.content_title);
        image = (ImageView) view.findViewById(R.id.content_image);
        playButton = (Button) view.findViewById(R.id.content_play_button);

        playButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                MainActivity activity = (MainActivity) getActivity();
                activity.showFragment(MainActivity.RPS, false);
            }
        });
        return view;
    }

    public void setContentIndex(int index) {
        title.setText(RpsGameUtils.STRINGS_TITLES[index]);
        image.setImageResource(RpsGameUtils.DRAWABLES_HUMAN[index]);
    }
}
