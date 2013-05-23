
package org.holoeverywhere.drawable;

import java.io.IOException;
import java.io.InputStream;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.support.v4.util.LongSparseArray;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.util.Xml;

public final class DrawableCompat {
    private static final Map<String, Class<? extends Drawable>> CLASS_MAP = new HashMap<String, Class<? extends Drawable>>();
    private static final LongSparseArray<WeakReference<Drawable.ConstantState>> sDrawableCache = new LongSparseArray<WeakReference<Drawable.ConstantState>>();

    static {
        CLASS_MAP.put("rotate", RotateDrawable.class);
        CLASS_MAP.put("layer-list", LayerDrawable.class);
        CLASS_MAP.put("selector", StateListDrawable.class);
        CLASS_MAP.put("color", ColorDrawable.class);
    }

    public static Drawable createFromPath(String pathName) {
        return Drawable.createFromPath(pathName);
    }

    public static Drawable createFromResourceStream(Resources res, TypedValue value,
            InputStream is, String srcName) {
        return createFromResourceStream(res, value, is, srcName, null);
    }

    public static Drawable createFromResourceStream(Resources res, TypedValue value,
            InputStream is, String srcName, BitmapFactory.Options opts) {
        return Drawable.createFromResourceStream(res, value, is, srcName, opts);
    }

    public static Drawable createFromStream(InputStream is, String srcName) {
        return createFromResourceStream(null, null, is, srcName, null);
    }

    public static Drawable createFromXml(Resources r, XmlPullParser parser)
            throws XmlPullParserException, IOException {
        AttributeSet attrs = Xml.asAttributeSet(parser);
        int type;
        while ((type = parser.next()) != XmlPullParser.START_TAG &&
                type != XmlPullParser.END_DOCUMENT) {
        }
        if (type != XmlPullParser.START_TAG) {
            throw new XmlPullParserException("No start tag found");
        }
        Drawable drawable = createFromXmlInner(r, parser, attrs);
        if (drawable == null) {
            throw new RuntimeException("Unknown initial tag: " + parser.getName());
        }
        return drawable;
    }

    public static Drawable createFromXmlInner(Resources r, XmlPullParser parser, AttributeSet attrs)
            throws XmlPullParserException, IOException {
        Drawable drawable = null;
        final String name = parser.getName();
        try {
            Class<? extends Drawable> clazz = CLASS_MAP.get(name);
            if (clazz != null) {
                drawable = clazz.newInstance();
            } else if (name.indexOf('.') > 0) {
                drawable = (Drawable) Class.forName(name).newInstance();
            }
        } catch (Exception e) {
            throw new XmlPullParserException("Error while inflating drawable resource", parser, e);
        }
        if (drawable == null) {
            return Drawable.createFromXmlInner(r, parser, attrs);
        }
        drawable.inflate(r, parser, attrs);
        return drawable;
    }

    private static Drawable getCachedDrawable(long key, Resources res) {
        WeakReference<Drawable.ConstantState> wr = sDrawableCache.get(key);
        if (wr != null) {
            Drawable.ConstantState entry = wr.get();
            if (entry != null) {
                return entry.newDrawable(res);
            } else {
                sDrawableCache.delete(key);
            }
        }
        return null;
    }

    public static Drawable getDrawable(Resources res, int resid) {
        TypedValue value = new TypedValue();
        res.getValue(resid, value, true);
        return loadDrawable(res, value);
    }

    public static Drawable getDrawable(TypedArray array, int index) {
        TypedValue value = new TypedValue();
        array.getValue(index, value);
        return loadDrawable(array.getResources(), value);
    }

    public static Drawable loadDrawable(Resources res, TypedValue value)
            throws NotFoundException {
        if (value == null || value.resourceId <= 0) {
            return null;
        }
        final long key = (long) value.assetCookie << 32 | value.data;
        Drawable dr = getCachedDrawable(key, res);
        if (dr != null) {
            return dr;
        }
        Drawable.ConstantState cs = null;
        if (value.string == null) {
            throw new NotFoundException("Resource is not a Drawable (color or path): " + value);
        }
        String file = value.string.toString();
        if (file.endsWith(".xml")) {
            try {
                XmlResourceParser rp = res.getAssets().openXmlResourceParser(value.assetCookie,
                        file);
                dr = DrawableCompat.createFromXml(res, rp);
                rp.close();
            } catch (Exception e) {
                NotFoundException rnf = new NotFoundException(
                        "File " + file + " from drawable resource ID #0x"
                                + Integer.toHexString(value.resourceId));
                rnf.initCause(e);
                throw rnf;
            }

        } else {
            try {
                InputStream is = res.getAssets().openNonAssetFd(value.assetCookie, file)
                        .createInputStream();
                dr = DrawableCompat.createFromResourceStream(res, value, is, file, null);
                is.close();
            } catch (Exception e) {
                NotFoundException rnf = new NotFoundException("File " + file
                        + " from drawable resource ID #0x"
                        + Integer.toHexString(value.resourceId));
                rnf.initCause(e);
                throw rnf;
            }
        }
        if (dr != null) {
            dr.setChangingConfigurations(value.changingConfigurations);
            cs = dr.getConstantState();
            if (cs != null) {
                sDrawableCache.put(key, new WeakReference<Drawable.ConstantState>(cs));
            }
        }
        return dr;
    }

    private DrawableCompat() {
    }

}
