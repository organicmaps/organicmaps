
package org.holoeverywhere.addon;

import org.holoeverywhere.app.Activity;

import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;

public abstract class IAddonActivity extends IAddonBase<Activity> {
    public boolean closeOptionsMenu() {
        return false;
    }

    public boolean dispatchKeyEvent(KeyEvent event) {
        return false;
    }

    public View findViewById(int id) {
        return null;
    }

    public boolean installDecorView(View view, LayoutParams params) {
        return false;
    }

    public boolean invalidateOptionsMenu() {
        return false;
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {
    }

    public void onConfigurationChanged(Configuration oldConfig, Configuration newConfig) {

    }

    public void onContentChanged() {

    }

    public void onCreate(Bundle savedInstanceState) {

    }

    public boolean onCreatePanelMenu(int featureId, android.view.Menu menu) {
        return false;
    }

    public void onDestroy() {

    }

    public boolean onMenuItemSelected(int featureId, android.view.MenuItem item) {
        return false;
    }

    public boolean onMenuOpened(int featureId, android.view.Menu menu) {
        return false;
    }

    public void onNewIntent(Intent intent) {

    }

    public void onPanelClosed(int featureId, android.view.Menu menu) {

    }

    public void onPause() {

    }

    public void onPostCreate(Bundle savedInstanceState) {

    }

    public void onPostResume() {

    }

    public void onPreCreate(Bundle savedInstanceState) {

    }

    public boolean onPreparePanel(int featureId, View view, android.view.Menu menu) {
        return false;
    }

    public void onRestart() {

    }

    public void onResume() {

    }

    public void onSaveInstanceState(Bundle outState) {

    }

    public void onStart() {

    }

    public void onStop() {

    }

    public void onTitleChanged(CharSequence title, int color) {

    }

    public boolean openOptionsMenu() {
        return false;
    }

    public boolean requestWindowFeature(int featureId) {
        return false;
    }
}
