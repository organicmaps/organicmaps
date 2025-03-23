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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.nanohttpd.protocols.http.response.Response;

/**
 * Provides rudimentary support for cookies. Doesn't support 'path', 'secure'
 * nor 'httpOnly'. Feel free to improve it and/or add unsupported features. This
 * is old code and it's flawed in many ways.
 * 
 * @author LordFokas
 */
public class CookieHandler implements Iterable<String> {

    private final HashMap<String, String> cookies = new HashMap<String, String>();

    private final ArrayList<Cookie> queue = new ArrayList<Cookie>();

    public CookieHandler(Map<String, String> httpHeaders) {
        String raw = httpHeaders.get("cookie");
        if (raw != null) {
            String[] tokens = raw.split(";");
            for (String token : tokens) {
                String[] data = token.trim().split("=");
                if (data.length == 2) {
                    this.cookies.put(data[0], data[1]);
                }
            }
        }
    }

    /**
     * Set a cookie with an expiration date from a month ago, effectively
     * deleting it on the client side.
     * 
     * @param name
     *            The cookie name.
     */
    public void delete(String name) {
        set(name, "-delete-", -30);
    }

    @Override
    public Iterator<String> iterator() {
        return this.cookies.keySet().iterator();
    }

    /**
     * Read a cookie from the HTTP Headers.
     * 
     * @param name
     *            The cookie's name.
     * @return The cookie's value if it exists, null otherwise.
     */
    public String read(String name) {
        return this.cookies.get(name);
    }

    public void set(Cookie cookie) {
        this.queue.add(cookie);
    }

    /**
     * Sets a cookie.
     * 
     * @param name
     *            The cookie's name.
     * @param value
     *            The cookie's value.
     * @param expires
     *            How many days until the cookie expires.
     */
    public void set(String name, String value, int expires) {
        this.queue.add(new Cookie(name, value, Cookie.getHTTPTime(expires)));
    }

    /**
     * Internally used by the webserver to add all queued cookies into the
     * Response's HTTP Headers.
     * 
     * @param response
     *            The Response object to which headers the queued cookies will
     *            be added.
     */
    public void unloadQueue(Response response) {
        for (Cookie cookie : this.queue) {
            response.addCookieHeader(cookie.getHTTPHeader());
        }
    }
}
