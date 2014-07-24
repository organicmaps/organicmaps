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

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.widget.LinearLayout;
import com.facebook.FacebookActivityTestCase;

public class FragmentTestCase<T extends FragmentTestCase.TestFragmentActivity<?>> extends FacebookActivityTestCase<T> {
    public FragmentTestCase(Class<T> activityClass) {
        super(activityClass);
    }

    protected T getTestActivity() {
        return (T) getActivity();
    }

    public static class TestFragmentActivity<T extends Fragment> extends FragmentActivity {
        public static final int FRAGMENT_ID = 0xFACE;

        private Class<T> fragmentClass;
        private int fragmentId;

        protected TestFragmentActivity(Class<T> fragmentClass) {
            this.fragmentClass = fragmentClass;
        }

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            if (getAutoCreateUI()) {
                setContentToFragment(null);
            }
        }

        protected boolean getAutoCreateUI() {
            return true;
        }

        void setContentToFragment(T fragment) {
            if (fragment == null) {
                try {
                    fragment = createFragment();
                } catch (InstantiationException e) {
                    return;
                } catch (IllegalAccessException e) {
                    return;
                }
            }

            LinearLayout layout = new LinearLayout(this);
            layout.setOrientation(LinearLayout.VERTICAL);
            layout.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT,
                    LinearLayout.LayoutParams.FILL_PARENT));
            layout.setId(FRAGMENT_ID);

            getSupportFragmentManager().beginTransaction()
                    .add(FRAGMENT_ID, fragment)
                    .commit();

            fragmentId = FRAGMENT_ID;

            setContentView(layout);
        }

        void setContentToLayout(int i, int fragmentId) {
            this.fragmentId = fragmentId;
            setContentView(i);
        }

        protected T createFragment() throws InstantiationException, IllegalAccessException {
            return fragmentClass.newInstance();
        }

        T getFragment() {
            @SuppressWarnings("unchecked")
            T fragment = (T) getSupportFragmentManager().findFragmentById(fragmentId);
            return fragment;
        }
    }
}
