package com.mapswithme.maps.unittests;
// @todo(vbykoianko) This file should be generated based on tests_list.h

public class AllTestsActivity extends android.app.NativeActivity {

    static {
       System.loadLibrary("routing_integration_tests");
       System.loadLibrary("indexer_tests");
       System.loadLibrary("all_tests");
    }
 }
 