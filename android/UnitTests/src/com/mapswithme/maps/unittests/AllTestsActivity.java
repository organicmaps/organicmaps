package com.mapswithme.maps.unittests;
// @todo(VB) This file should be generaged based on tests_list.h

public class AllTestsActivity extends android.app.NativeActivity {

    static {
       System.loadLibrary("integration_tests");
       System.loadLibrary("indexer_tests");
       System.loadLibrary("all_tests");
    }
 }
 