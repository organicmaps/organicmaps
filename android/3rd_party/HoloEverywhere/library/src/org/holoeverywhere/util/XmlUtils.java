
package org.holoeverywhere.util;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import android.util.Xml;

@SuppressWarnings({
        "rawtypes", "unchecked"
})
public class XmlUtils {

    public static final void beginDocument(XmlPullParser parser,
            String firstElementName) throws XmlPullParserException, IOException {
        int type;
        while ((type = parser.next()) != XmlPullParser.START_TAG
                && type != XmlPullParser.END_DOCUMENT) {
            ;
        }

        if (type != XmlPullParser.START_TAG) {
            throw new XmlPullParserException("No start tag found");
        }

        if (!parser.getName().equals(firstElementName)) {
            throw new XmlPullParserException("Unexpected start tag: found "
                    + parser.getName() + ", expected " + firstElementName);
        }
    }

    public static final boolean convertValueToBoolean(CharSequence value,
            boolean defaultValue) {
        boolean result = false;

        if (null == value) {
            return defaultValue;
        }

        if (value.equals("1") || value.equals("true") || value.equals("TRUE")) {
            result = true;
        }

        return result;
    }

    public static final int convertValueToInt(CharSequence charSeq,
            int defaultValue) {
        if (null == charSeq) {
            return defaultValue;
        }

        String nm = charSeq.toString();

        int sign = 1;
        int index = 0;
        int len = nm.length();
        int base = 10;

        if ('-' == nm.charAt(0)) {
            sign = -1;
            index++;
        }

        if ('0' == nm.charAt(index)) {
            // Quick check for a zero by itself
            if (index == len - 1) {
                return 0;
            }

            char c = nm.charAt(index + 1);

            if ('x' == c || 'X' == c) {
                index += 2;
                base = 16;
            } else {
                index++;
                base = 8;
            }
        } else if ('#' == nm.charAt(index)) {
            index++;
            base = 16;
        }

        return Integer.parseInt(nm.substring(index), base) * sign;
    }

    public static final int convertValueToList(CharSequence value,
            String[] options, int defaultValue) {
        if (null != value) {
            for (int i = 0; i < options.length; i++) {
                if (value.equals(options[i])) {
                    return i;
                }
            }
        }

        return defaultValue;
    }

    public static final int convertValueToUnsignedInt(String value,
            int defaultValue) {
        if (null == value) {
            return defaultValue;
        }

        return XmlUtils.parseUnsignedIntAttribute(value);
    }

    public static final void nextElement(XmlPullParser parser)
            throws XmlPullParserException, IOException {
        int type;
        while ((type = parser.next()) != XmlPullParser.START_TAG
                && type != XmlPullParser.END_DOCUMENT) {
            ;
        }
    }

    public static boolean nextElementWithin(XmlPullParser parser, int outerDepth)
            throws IOException, XmlPullParserException {
        for (;;) {
            int type = parser.next();
            if (type == XmlPullParser.END_DOCUMENT
                    || type == XmlPullParser.END_TAG
                    && parser.getDepth() == outerDepth) {
                return false;
            }
            if (type == XmlPullParser.START_TAG
                    && parser.getDepth() == outerDepth + 1) {
                return true;
            }
        }
    }

    public static final int parseUnsignedIntAttribute(CharSequence charSeq) {
        String value = charSeq.toString();

        int index = 0;
        int len = value.length();
        int base = 10;

        if ('0' == value.charAt(index)) {
            // Quick check for zero by itself
            if (index == len - 1) {
                return 0;
            }

            char c = value.charAt(index + 1);

            if ('x' == c || 'X' == c) { // check for hex
                index += 2;
                base = 16;
            } else { // check for octal
                index++;
                base = 8;
            }
        } else if ('#' == value.charAt(index)) {
            index++;
            base = 16;
        }

        return (int) Long.parseLong(value.substring(index), base);
    }

    public static final ArrayList readListXml(InputStream in)
            throws XmlPullParserException, java.io.IOException {
        XmlPullParser parser = Xml.newPullParser();
        parser.setInput(in, null);
        return (ArrayList) XmlUtils.readValueXml(parser, new String[1]);
    }

    public static final HashMap readMapXml(InputStream in)
            throws XmlPullParserException, java.io.IOException {
        XmlPullParser parser = Xml.newPullParser();
        parser.setInput(in, null);
        return (HashMap) XmlUtils.readValueXml(parser, new String[1]);
    }

    public static final HashSet readSetXml(InputStream in)
            throws XmlPullParserException, java.io.IOException {
        XmlPullParser parser = Xml.newPullParser();
        parser.setInput(in, null);
        return (HashSet) XmlUtils.readValueXml(parser, new String[1]);
    }

    public static final int[] readThisIntArrayXml(XmlPullParser parser,
            String endTag, String[] name) throws XmlPullParserException,
            java.io.IOException {

        int num;
        try {
            num = Integer.parseInt(parser.getAttributeValue(null, "num"));
        } catch (NullPointerException e) {
            throw new XmlPullParserException("Need num attribute in byte-array");
        } catch (NumberFormatException e) {
            throw new XmlPullParserException(
                    "Not a number in num attribute in byte-array");
        }

        int[] array = new int[num];
        int i = 0;

        int eventType = parser.getEventType();
        do {
            if (eventType == XmlPullParser.START_TAG) {
                if (parser.getName().equals("item")) {
                    try {
                        array[i] = Integer.parseInt(parser.getAttributeValue(
                                null, "value"));
                    } catch (NullPointerException e) {
                        throw new XmlPullParserException(
                                "Need value attribute in item");
                    } catch (NumberFormatException e) {
                        throw new XmlPullParserException(
                                "Not a number in value attribute in item");
                    }
                } else {
                    throw new XmlPullParserException("Expected item tag at: "
                            + parser.getName());
                }
            } else if (eventType == XmlPullParser.END_TAG) {
                if (parser.getName().equals(endTag)) {
                    return array;
                } else if (parser.getName().equals("item")) {
                    i++;
                } else {
                    throw new XmlPullParserException("Expected " + endTag
                            + " end tag at: " + parser.getName());
                }
            }
            eventType = parser.next();
        } while (eventType != XmlPullParser.END_DOCUMENT);

        throw new XmlPullParserException("Document ended before " + endTag
                + " end tag");
    }

    public static final ArrayList readThisListXml(XmlPullParser parser,
            String endTag, String[] name) throws XmlPullParserException,
            java.io.IOException {
        ArrayList list = new ArrayList();

        int eventType = parser.getEventType();
        do {
            if (eventType == XmlPullParser.START_TAG) {
                Object val = XmlUtils.readThisValueXml(parser, name);
                list.add(val);
                // System.out.println("Adding to list: " + val);
            } else if (eventType == XmlPullParser.END_TAG) {
                if (parser.getName().equals(endTag)) {
                    return list;
                }
                throw new XmlPullParserException("Expected " + endTag
                        + " end tag at: " + parser.getName());
            }
            eventType = parser.next();
        } while (eventType != XmlPullParser.END_DOCUMENT);

        throw new XmlPullParserException("Document ended before " + endTag
                + " end tag");
    }

    public static final HashMap readThisMapXml(XmlPullParser parser,
            String endTag, String[] name) throws XmlPullParserException,
            java.io.IOException {
        HashMap map = new HashMap();

        int eventType = parser.getEventType();
        do {
            if (eventType == XmlPullParser.START_TAG) {
                Object val = XmlUtils.readThisValueXml(parser, name);
                if (name[0] != null) {
                    map.put(name[0], val);
                } else {
                    throw new XmlPullParserException(
                            "Map value without name attribute: "
                                    + parser.getName());
                }
            } else if (eventType == XmlPullParser.END_TAG) {
                if (parser.getName().equals(endTag)) {
                    return map;
                }
                throw new XmlPullParserException("Expected " + endTag
                        + " end tag at: " + parser.getName());
            }
            eventType = parser.next();
        } while (eventType != XmlPullParser.END_DOCUMENT);

        throw new XmlPullParserException("Document ended before " + endTag
                + " end tag");
    }

    public static final HashSet readThisSetXml(XmlPullParser parser,
            String endTag, String[] name) throws XmlPullParserException,
            java.io.IOException {
        HashSet set = new HashSet();

        int eventType = parser.getEventType();
        do {
            if (eventType == XmlPullParser.START_TAG) {
                Object val = XmlUtils.readThisValueXml(parser, name);
                set.add(val);
                // System.out.println("Adding to set: " + val);
            } else if (eventType == XmlPullParser.END_TAG) {
                if (parser.getName().equals(endTag)) {
                    return set;
                }
                throw new XmlPullParserException("Expected " + endTag
                        + " end tag at: " + parser.getName());
            }
            eventType = parser.next();
        } while (eventType != XmlPullParser.END_DOCUMENT);

        throw new XmlPullParserException("Document ended before " + endTag
                + " end tag");
    }

    private static final Object readThisValueXml(XmlPullParser parser,
            String[] name) throws XmlPullParserException, java.io.IOException {
        final String valueName = parser.getAttributeValue(null, "name");
        final String tagName = parser.getName();
        Object res;

        if (tagName.equals("null")) {
            res = null;
        } else if (tagName.equals("string")) {
            String value = "";
            int eventType;
            while ((eventType = parser.next()) != XmlPullParser.END_DOCUMENT) {
                if (eventType == XmlPullParser.END_TAG) {
                    if (parser.getName().equals("string")) {
                        name[0] = valueName;
                        // System.out.println("Returning value for " + valueName
                        // + ": " + value);
                        return value;
                    }
                    throw new XmlPullParserException(
                            "Unexpected end tag in <string>: "
                                    + parser.getName());
                } else if (eventType == XmlPullParser.TEXT) {
                    value += parser.getText();
                } else if (eventType == XmlPullParser.START_TAG) {
                    throw new XmlPullParserException(
                            "Unexpected start tag in <string>: "
                                    + parser.getName());
                }
            }
            throw new XmlPullParserException(
                    "Unexpected end of document in <string>");
        } else if (tagName.equals("int")) {
            res = Integer.parseInt(parser.getAttributeValue(null, "value"));
        } else if (tagName.equals("long")) {
            res = Long.valueOf(parser.getAttributeValue(null, "value"));
        } else if (tagName.equals("float")) {
            res = Float.valueOf(parser.getAttributeValue(null, "value"));
        } else if (tagName.equals("double")) {
            res = Double.valueOf(parser.getAttributeValue(null, "value"));
        } else if (tagName.equals("boolean")) {
            res = Boolean.valueOf(parser.getAttributeValue(null, "value"));
        } else if (tagName.equals("int-array")) {
            parser.next();
            res = XmlUtils.readThisIntArrayXml(parser, "int-array", name);
            name[0] = valueName;
            return res;
        } else if (tagName.equals("map")) {
            parser.next();
            res = XmlUtils.readThisMapXml(parser, "map", name);
            name[0] = valueName;
            return res;
        } else if (tagName.equals("list")) {
            parser.next();
            res = XmlUtils.readThisListXml(parser, "list", name);
            name[0] = valueName;
            return res;
        } else if (tagName.equals("set")) {
            parser.next();
            res = XmlUtils.readThisSetXml(parser, "set", name);
            name[0] = valueName;
            return res;
        } else {
            throw new XmlPullParserException("Unknown tag: " + tagName);
        }

        int eventType;
        while ((eventType = parser.next()) != XmlPullParser.END_DOCUMENT) {
            if (eventType == XmlPullParser.END_TAG) {
                if (parser.getName().equals(tagName)) {
                    name[0] = valueName;
                    return res;
                }
                throw new XmlPullParserException("Unexpected end tag in <"
                        + tagName + ">: " + parser.getName());
            } else if (eventType == XmlPullParser.TEXT) {
                throw new XmlPullParserException("Unexpected text in <"
                        + tagName + ">: " + parser.getName());
            } else if (eventType == XmlPullParser.START_TAG) {
                throw new XmlPullParserException("Unexpected start tag in <"
                        + tagName + ">: " + parser.getName());
            }
        }
        throw new XmlPullParserException("Unexpected end of document in <"
                + tagName + ">");
    }

    public static final Object readValueXml(XmlPullParser parser, String[] name)
            throws XmlPullParserException, java.io.IOException {
        int eventType = parser.getEventType();
        do {
            if (eventType == XmlPullParser.START_TAG) {
                return XmlUtils.readThisValueXml(parser, name);
            } else if (eventType == XmlPullParser.END_TAG) {
                throw new XmlPullParserException("Unexpected end tag at: "
                        + parser.getName());
            } else if (eventType == XmlPullParser.TEXT) {
                throw new XmlPullParserException("Unexpected text: "
                        + parser.getText());
            }
            eventType = parser.next();
        } while (eventType != XmlPullParser.END_DOCUMENT);

        throw new XmlPullParserException("Unexpected end of document");
    }

    public static void skipCurrentTag(XmlPullParser parser)
            throws XmlPullParserException, IOException {
        int outerDepth = parser.getDepth();
        int type;
        while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
        }
    }

    public static final void writeByteArrayXml(byte[] val, String name,
            XmlSerializer out) throws XmlPullParserException,
            java.io.IOException {

        if (val == null) {
            out.startTag(null, "null");
            out.endTag(null, "null");
            return;
        }

        out.startTag(null, "byte-array");
        if (name != null) {
            out.attribute(null, "name", name);
        }

        final int N = val.length;
        out.attribute(null, "num", Integer.toString(N));

        StringBuilder sb = new StringBuilder(val.length * 2);
        for (int i = 0; i < N; i++) {
            int b = val[i];
            int h = b >> 4;
            sb.append(h >= 10 ? 'a' + h - 10 : '0' + h);
            h = b & 0xff;
            sb.append(h >= 10 ? 'a' + h - 10 : '0' + h);
        }

        out.text(sb.toString());

        out.endTag(null, "byte-array");
    }

    public static final void writeIntArrayXml(int[] val, String name,
            XmlSerializer out) throws XmlPullParserException,
            java.io.IOException {

        if (val == null) {
            out.startTag(null, "null");
            out.endTag(null, "null");
            return;
        }

        out.startTag(null, "int-array");
        if (name != null) {
            out.attribute(null, "name", name);
        }

        final int N = val.length;
        out.attribute(null, "num", Integer.toString(N));

        for (int i = 0; i < N; i++) {
            out.startTag(null, "item");
            out.attribute(null, "value", Integer.toString(val[i]));
            out.endTag(null, "item");
        }

        out.endTag(null, "int-array");
    }

    public static final void writeListXml(List val, OutputStream out)
            throws XmlPullParserException, java.io.IOException {
        XmlSerializer serializer = Xml.newSerializer();
        serializer.setOutput(out, "utf-8");
        serializer.startDocument(null, true);
        serializer.setFeature(
                "http://xmlpull.org/v1/doc/features.html#indent-output", true);
        XmlUtils.writeListXml(val, null, serializer);
        serializer.endDocument();
    }

    public static final void writeListXml(List val, String name,
            XmlSerializer out) throws XmlPullParserException,
            java.io.IOException {
        if (val == null) {
            out.startTag(null, "null");
            out.endTag(null, "null");
            return;
        }

        out.startTag(null, "list");
        if (name != null) {
            out.attribute(null, "name", name);
        }

        int N = val.size();
        int i = 0;
        while (i < N) {
            XmlUtils.writeValueXml(val.get(i), null, out);
            i++;
        }

        out.endTag(null, "list");
    }

    public static final void writeMapXml(Map val, OutputStream out)
            throws XmlPullParserException, java.io.IOException {
        XmlSerializer serializer = new FastXmlSerializer();
        serializer.setOutput(out, "utf-8");
        serializer.startDocument(null, true);
        serializer.setFeature(
                "http://xmlpull.org/v1/doc/features.html#indent-output", true);
        XmlUtils.writeMapXml(val, null, serializer);
        serializer.endDocument();
    }

    public static final void writeMapXml(Map val, String name, XmlSerializer out)
            throws XmlPullParserException, java.io.IOException {
        if (val == null) {
            out.startTag(null, "null");
            out.endTag(null, "null");
            return;
        }

        Set s = val.entrySet();
        Iterator i = s.iterator();

        out.startTag(null, "map");
        if (name != null) {
            out.attribute(null, "name", name);
        }

        while (i.hasNext()) {
            Map.Entry e = (Map.Entry) i.next();
            XmlUtils.writeValueXml(e.getValue(), (String) e.getKey(), out);
        }

        out.endTag(null, "map");
    }

    public static final void writeSetXml(Set val, String name, XmlSerializer out)
            throws XmlPullParserException, java.io.IOException {
        if (val == null) {
            out.startTag(null, "null");
            out.endTag(null, "null");
            return;
        }

        out.startTag(null, "set");
        if (name != null) {
            out.attribute(null, "name", name);
        }

        for (Object v : val) {
            XmlUtils.writeValueXml(v, null, out);
        }

        out.endTag(null, "set");
    }

    public static final void writeValueXml(Object v, String name,
            XmlSerializer out) throws XmlPullParserException,
            java.io.IOException {
        String typeStr;
        if (v == null) {
            out.startTag(null, "null");
            if (name != null) {
                out.attribute(null, "name", name);
            }
            out.endTag(null, "null");
            return;
        } else if (v instanceof String) {
            out.startTag(null, "string");
            if (name != null) {
                out.attribute(null, "name", name);
            }
            out.text(v.toString());
            out.endTag(null, "string");
            return;
        } else if (v instanceof Integer) {
            typeStr = "int";
        } else if (v instanceof Long) {
            typeStr = "long";
        } else if (v instanceof Float) {
            typeStr = "float";
        } else if (v instanceof Double) {
            typeStr = "double";
        } else if (v instanceof Boolean) {
            typeStr = "boolean";
        } else if (v instanceof byte[]) {
            XmlUtils.writeByteArrayXml((byte[]) v, name, out);
            return;
        } else if (v instanceof int[]) {
            XmlUtils.writeIntArrayXml((int[]) v, name, out);
            return;
        } else if (v instanceof Map) {
            XmlUtils.writeMapXml((Map) v, name, out);
            return;
        } else if (v instanceof List) {
            XmlUtils.writeListXml((List) v, name, out);
            return;
        } else if (v instanceof Set) {
            XmlUtils.writeSetXml((Set) v, name, out);
            return;
        } else if (v instanceof CharSequence) {
            out.startTag(null, "string");
            if (name != null) {
                out.attribute(null, "name", name);
            }
            out.text(v.toString());
            out.endTag(null, "string");
            return;
        } else {
            throw new RuntimeException("writeValueXml: unable to write value "
                    + v);
        }

        out.startTag(null, typeStr);
        if (name != null) {
            out.attribute(null, "name", name);
        }
        out.attribute(null, "value", v.toString());
        out.endTag(null, typeStr);
    }
}
