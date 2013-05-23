ActionBarSherlock
=================

ActionBarSherlock is an standalone library designed to facilitate the use of
the action bar design pattern across all versions of Android through a single
API.

The library will automatically use the [native ActionBar][2] implementation on
Android 4.0 or later. For previous versions which do not include ActionBar, a
custom action bar implementation based on the sources of Ice Cream Sandwich
will automatically be wrapped around the layout. This allows you to easily
develop an application with an action bar for every version of Android from 2.x
and up.

**See http://actionbarsherlock.com for more information.**

![Example Image][3]

Try out the sample applications on the Android Market: [Feature Demos][4],
[Fragments][5], and [RoboGuice][6].

Continuous integration is provided by [Travis CI][7].



Developed By
============

* Jake Wharton - <jakewharton@gmail.com>



License
=======

    Copyright 2012 Jake Wharton

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.





 [1]: http://android-developers.blogspot.com/2011/03/fragments-for-all.html
 [2]: http://developer.android.com/guide/topics/ui/actionbar.html
 [3]: http://actionbarsherlock.com/static/feature.png
 [4]: https://play.google.com/store/apps/details?id=com.actionbarsherlock.sample.demos
 [5]: https://play.google.com/store/apps/details?id=com.actionbarsherlock.sample.fragments
 [6]: https://play.google.com/store/apps/details?id=com.actionbarsherlock.sample.roboguice
 [7]: https://travis-ci.org/JakeWharton/ActionBarSherlock
