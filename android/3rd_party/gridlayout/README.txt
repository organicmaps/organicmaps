Library Project including compatibility GridLayout.

This can be used by an Android project to provide
access to GridLayout on applications running on API 7+.

There is technically no source, but the src folder is necessary
to ensure that the build system works.  The content is actually
located in libs/android-support-v7-gridlayout.jar.
The accompanying resources must also be included in the application.


USAGE:

Make sure you use <android.support.v7.widget.GridLayout> in your
layouts instead of <GridLayout>.
Same for <android.support.v7.widget.Space> instead of <Space>.

Additionally, all of GridLayout's attributes should be put in the
namespace of the app, as those attributes have been redefined in
the library so that it can run on older platforms that don't offer
those attributes in their namespace.

To know which attributes need the application namespace, look at
the two declare-styleable declared in res/values/attrs.xml



For instance:

<?xml version="1.0" encoding="utf-8"?>
<android.support.v7.widget.GridLayout
    xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:app="http://schemas.android.com/apk/res-auto"  <==== the namespace used for the library project
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    app:columnCount="6" >                                <===== notice how we're using app:columnCount here, not android:columnCount!

    <Button
        android:id="@+id/button1"
        app:layout_column="1"                            <=== again, note the app: namespace
        app:layout_columnSpan="2"
        app:layout_gravity="left"
        app:layout_row="1"
        android:text="Button" />

    <CheckBox
        android:id="@+id/checkBox1"
        app:layout_column="4"
        app:layout_gravity="left"
        app:layout_row="2"
        android:text="CheckBox" />

    <Button
        android:id="@+id/button2"
        app:layout_column="5"
        app:layout_gravity="left"
        app:layout_row="3"
        android:text="Button" />

    <android.support.v7.widget.Space                    <=== space widgets also need the full support package path
        android:layout_width="21dp"                     <=== use the android namespace for width, height etc -- only use app: for the grid layout library's new resources
        android:layout_height="1dp"
        app:layout_column="0"
        app:layout_gravity="fill_horizontal"
        app:layout_row="0" />
