package org.nanohttpd.protocols.http.content;

/*
 * #%L
 * NanoHttpd-Core
 * %%
 * Copyright (C) 2012 - 2016 nanohttpd
 * %%
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the nanohttpd nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * #L%
 */

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class ContentType {

    private static final String ASCII_ENCODING = "US-ASCII";

    private static final String MULTIPART_FORM_DATA_HEADER = "multipart/form-data";

    private static final String CONTENT_REGEX = "[ |\t]*([^/^ ^;^,]+/[^ ^;^,]+)";

    private static final Pattern MIME_PATTERN = Pattern.compile(CONTENT_REGEX, Pattern.CASE_INSENSITIVE);

    private static final String CHARSET_REGEX = "[ |\t]*(charset)[ |\t]*=[ |\t]*['|\"]?([^\"^'^;^,]*)['|\"]?";

    private static final Pattern CHARSET_PATTERN = Pattern.compile(CHARSET_REGEX, Pattern.CASE_INSENSITIVE);

    private static final String BOUNDARY_REGEX = "[ |\t]*(boundary)[ |\t]*=[ |\t]*['|\"]?([^\"^'^;^,]*)['|\"]?";

    private static final Pattern BOUNDARY_PATTERN = Pattern.compile(BOUNDARY_REGEX, Pattern.CASE_INSENSITIVE);

    private final String contentTypeHeader;

    private final String contentType;

    private final String encoding;

    private final String boundary;

    public ContentType(String contentTypeHeader) {
        this.contentTypeHeader = contentTypeHeader;
        if (contentTypeHeader != null) {
            contentType = getDetailFromContentHeader(contentTypeHeader, MIME_PATTERN, "", 1);
            encoding = getDetailFromContentHeader(contentTypeHeader, CHARSET_PATTERN, null, 2);
        } else {
            contentType = "";
            encoding = "UTF-8";
        }
        if (MULTIPART_FORM_DATA_HEADER.equalsIgnoreCase(contentType)) {
            boundary = getDetailFromContentHeader(contentTypeHeader, BOUNDARY_PATTERN, null, 2);
        } else {
            boundary = null;
        }
    }

    private String getDetailFromContentHeader(String contentTypeHeader, Pattern pattern, String defaultValue, int group) {
        Matcher matcher = pattern.matcher(contentTypeHeader);
        return matcher.find() ? matcher.group(group) : defaultValue;
    }

    public String getContentTypeHeader() {
        return contentTypeHeader;
    }

    public String getContentType() {
        return contentType;
    }

    public String getEncoding() {
        return encoding == null ? ASCII_ENCODING : encoding;
    }

    public String getBoundary() {
        return boundary;
    }

    public boolean isMultipart() {
        return MULTIPART_FORM_DATA_HEADER.equalsIgnoreCase(contentType);
    }

    public ContentType tryUTF8() {
        if (encoding == null) {
            return new ContentType(this.contentTypeHeader + "; charset=UTF-8");
        }
        return this;
    }
}
