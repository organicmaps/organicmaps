package com.actionbarsherlock.internal;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

import static com.actionbarsherlock.internal.ActionBarSherlockCompat.cleanActivityName;
import static org.fest.assertions.api.Assertions.assertThat;

@RunWith(RobolectricTestRunner.class)
public class ResourcesCompatTest {
    @Test
    public void testCleanActivityName() {
        assertThat(cleanActivityName("com.jakewharton.test", "com.other.package.SomeClass")) //
            .isEqualTo("com.other.package.SomeClass");
        assertThat(cleanActivityName("com.jakewharton.test", "com.jakewharton.test.SomeClass")) //
            .isEqualTo("com.jakewharton.test.SomeClass");
        assertThat(cleanActivityName("com.jakewharton.test", "SomeClass")) //
            .isEqualTo("com.jakewharton.test.SomeClass");
        assertThat(cleanActivityName("com.jakewharton.test", ".ui.SomeClass")) //
            .isEqualTo("com.jakewharton.test.ui.SomeClass");
    }
}