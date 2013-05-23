package com.actionbarsherlock.internal;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.XmlResourceParser;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.Log;
import com.actionbarsherlock.BuildConfig;
import com.actionbarsherlock.R;
import org.xmlpull.v1.XmlPullParser;

public final class ResourcesCompat {
    private static final String TAG = "ResourcesCompat";

    //No instances
    private ResourcesCompat() {}


    /**
     * Support implementation of {@code getResources().getBoolean()} that we
     * can use to simulate filtering based on width and smallest width
     * qualifiers on pre-3.2.
     *
     * @param context Context to load booleans from on 3.2+ and to fetch the
     * display metrics.
     * @param id Id of boolean to load.
     * @return Associated boolean value as reflected by the current display
     * metrics.
     */
    public static boolean getResources_getBoolean(Context context, int id) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2) {
            return context.getResources().getBoolean(id);
        }

        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        float widthDp = metrics.widthPixels / metrics.density;
        float heightDp = metrics.heightPixels / metrics.density;
        float smallestWidthDp = (widthDp < heightDp) ? widthDp : heightDp;

        if (id == R.bool.abs__action_bar_embed_tabs) {
            if (widthDp >= 480) {
                return true; //values-w480dp
            }
            return false; //values
        }
        if (id == R.bool.abs__split_action_bar_is_narrow) {
            if (widthDp >= 480) {
                return false; //values-w480dp
            }
            return true; //values
        }
        if (id == R.bool.abs__action_bar_expanded_action_views_exclusive) {
            if (smallestWidthDp >= 600) {
                return false; //values-sw600dp
            }
            return true; //values
        }
        if (id == R.bool.abs__config_allowActionMenuItemTextWithIcon) {
            if (widthDp >= 480) {
                return true; //values-w480dp
            }
            return false; //values
        }

        throw new IllegalArgumentException("Unknown boolean resource ID " + id);
    }

    /**
     * Support implementation of {@code getResources().getInteger()} that we
     * can use to simulate filtering based on width qualifiers on pre-3.2.
     *
     * @param context Context to load integers from on 3.2+ and to fetch the
     * display metrics.
     * @param id Id of integer to load.
     * @return Associated integer value as reflected by the current display
     * metrics.
     */
    public static int getResources_getInteger(Context context, int id) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2) {
            return context.getResources().getInteger(id);
        }

        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        float widthDp = metrics.widthPixels / metrics.density;

        if (id == R.integer.abs__max_action_buttons) {
            if (widthDp >= 600) {
                return 5; //values-w600dp
            }
            if (widthDp >= 500) {
                return 4; //values-w500dp
            }
            if (widthDp >= 360) {
                return 3; //values-w360dp
            }
            return 2; //values
        }

        throw new IllegalArgumentException("Unknown integer resource ID " + id);
    }

    /**
     * Attempt to programmatically load the logo from the manifest file of an
     * activity by using an XML pull parser. This should allow us to read the
     * logo attribute regardless of the platform it is being run on.
     *
     * @param activity Activity instance.
     * @return Logo resource ID.
     */
    public static int loadLogoFromManifest(Activity activity) {
        int logo = 0;
        try {
            final String thisPackage = activity.getClass().getName();
            if (BuildConfig.DEBUG) Log.i(TAG, "Parsing AndroidManifest.xml for " + thisPackage);

            final String packageName = activity.getApplicationInfo().packageName;
            final AssetManager am = activity.createPackageContext(packageName, 0).getAssets();
            final XmlResourceParser xml = am.openXmlResourceParser("AndroidManifest.xml");

            int eventType = xml.getEventType();
            while (eventType != XmlPullParser.END_DOCUMENT) {
                if (eventType == XmlPullParser.START_TAG) {
                    String name = xml.getName();

                    if ("application".equals(name)) {
                        //Check if the <application> has the attribute
                        if (BuildConfig.DEBUG) Log.d(TAG, "Got <application>");

                        for (int i = xml.getAttributeCount() - 1; i >= 0; i--) {
                            if (BuildConfig.DEBUG) Log.d(TAG, xml.getAttributeName(i) + ": " + xml.getAttributeValue(i));

                            if ("logo".equals(xml.getAttributeName(i))) {
                                logo = xml.getAttributeResourceValue(i, 0);
                                break; //out of for loop
                            }
                        }
                    } else if ("activity".equals(name)) {
                        //Check if the <activity> is us and has the attribute
                        if (BuildConfig.DEBUG) Log.d(TAG, "Got <activity>");
                        Integer activityLogo = null;
                        String activityPackage = null;
                        boolean isOurActivity = false;

                        for (int i = xml.getAttributeCount() - 1; i >= 0; i--) {
                            if (BuildConfig.DEBUG) Log.d(TAG, xml.getAttributeName(i) + ": " + xml.getAttributeValue(i));

                            //We need both uiOptions and name attributes
                            String attrName = xml.getAttributeName(i);
                            if ("logo".equals(attrName)) {
                                activityLogo = xml.getAttributeResourceValue(i, 0);
                            } else if ("name".equals(attrName)) {
                                activityPackage = ActionBarSherlockCompat.cleanActivityName(packageName, xml.getAttributeValue(i));
                                if (!thisPackage.equals(activityPackage)) {
                                    break; //on to the next
                                }
                                isOurActivity = true;
                            }

                            //Make sure we have both attributes before processing
                            if ((activityLogo != null) && (activityPackage != null)) {
                                //Our activity, logo specified, override with our value
                                logo = activityLogo.intValue();
                            }
                        }
                        if (isOurActivity) {
                            //If we matched our activity but it had no logo don't
                            //do any more processing of the manifest
                            break;
                        }
                    }
                }
                eventType = xml.nextToken();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (BuildConfig.DEBUG) Log.i(TAG, "Returning " + Integer.toHexString(logo));
        return logo;
    }
}
