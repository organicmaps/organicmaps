
package org.holoeverywhere.preference;

import java.util.Map;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONObject;

import android.os.Bundle;

public interface SharedPreferences extends android.content.SharedPreferences {
    public static interface Editor extends
            android.content.SharedPreferences.Editor {
        @Override
        public void apply();

        @Override
        public Editor clear();

        @Override
        public boolean commit();

        public Editor putBoolean(int id, boolean value);

        @Override
        public Editor putBoolean(String key, boolean value);

        public Editor putFloat(int id, float value);

        @Override
        public Editor putFloat(String key, float value);

        public Editor putFloatSet(int id, Set<Float> value);

        public Editor putFloatSet(String key, Set<Float> value);

        public Editor putInt(int id, int value);

        @Override
        public Editor putInt(String key, int value);

        public Editor putIntSet(int id, Set<Integer> value);

        public Editor putIntSet(String key, Set<Integer> value);

        public Editor putJSONArray(int id, JSONArray value);

        public Editor putJSONArray(String key, JSONArray value);

        public Editor putJSONObject(int id, JSONObject value);

        public Editor putJSONObject(String key, JSONObject value);

        public Editor putLong(int id, long value);

        @Override
        public Editor putLong(String key, long value);

        public Editor putLongSet(int id, Set<Long> value);

        public Editor putLongSet(String key, Set<Long> value);

        public Editor putString(int id, String value);

        @Override
        public Editor putString(String key, String value);

        public Editor putStringSet(int id, Set<String> value);

        @Override
        public Editor putStringSet(String key, Set<String> value);

        public Editor remove(int id);

        @Override
        public Editor remove(String key);
    }

    public static interface OnSharedPreferenceChangeListener {
        public void onSharedPreferenceChanged(SharedPreferences prefs, String key);
    }

    public boolean contains(int id);

    @Override
    public boolean contains(String key);

    @Override
    public Editor edit();

    @Override
    public Map<String, ?> getAll();

    public boolean getBoolean(int id, boolean defValue);

    @Override
    public boolean getBoolean(String key, boolean defValue);

    public float getFloat(int id, float defValue);

    @Override
    public float getFloat(String key, float defValue);

    public Set<Float> getFloatSet(int id, Set<Float> defValue);

    public Set<Float> getFloatSet(String key, Set<Float> defValue);

    public int getInt(int id, int defValue);

    @Override
    public int getInt(String key, int defValue);

    public Set<Integer> getIntSet(int id, Set<Integer> defValue);

    public Set<Integer> getIntSet(String key, Set<Integer> defValue);

    public JSONArray getJSONArray(int id, JSONArray defValue);

    public JSONArray getJSONArray(String key, JSONArray defValue);

    public JSONObject getJSONObject(int id, JSONObject defValue);

    public JSONObject getJSONObject(String key, JSONObject defValue);

    public long getLong(int id, long defValue);

    @Override
    public long getLong(String key, long defValue);

    public Set<Long> getLongSet(int id, Set<Long> defValue);

    public Set<Long> getLongSet(String key, Set<Long> defValue);

    public String getString(int id, String defValue);

    @Override
    public String getString(String key, String defValue);

    public Set<String> getStringSet(int id, Set<String> defValue);

    @Override
    public Set<String> getStringSet(String key, Set<String> defValue);

    public String makeNameById(int id);

    public void registerOnSharedPreferenceChangeListener(
            OnSharedPreferenceChangeListener listener);

    public void setDefaultValues(Bundle bundle);

    public void unregisterOnSharedPreferenceChangeListener(
            OnSharedPreferenceChangeListener listener);
}
