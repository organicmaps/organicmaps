/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

package org.alohalytics.test;

import android.test.InstrumentationTestCase;

import org.alohalytics.HttpTransport;

import java.io.File;
import java.io.FileNotFoundException;

// <a href="http://d.android.com/tools/testing/testing_android.html">Testing Fundamentals</a>
public class HttpTransportTest extends InstrumentationTestCase {

  static {
    // For faster unit testing on the real-world servers.
    HttpTransport.TIMEOUT_IN_MILLISECONDS = 3000;
  }

  public static final String CACHE_DIR = "/data/data/org.alohalytics.demoapp/cache/";
  public static final String HTTPBIN_POST_URL = "http://httpbin.org/post";

  @Override
  protected void setUp() {
    new File(CACHE_DIR).mkdirs();
  }

  private String getFullWritablePathForFile(String fileName) {
    return CACHE_DIR + fileName;
  }

  public void testGetIntoMemory() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("http://httpbin.org/drip?numbytes=7");
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(200, r.httpResponseCode);
    assertEquals(p.url, r.receivedUrl);
    assertEquals("*******", new String(r.data));
  }

  public void testGetIntoFile() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("http://httpbin.org/drip?numbytes=5");
    try {
      p.outputFilePath = getFullWritablePathForFile("some_test_file_for_http_get");
      final HttpTransport.Params r = HttpTransport.run(p);
      assertEquals(200, r.httpResponseCode);
      assertNull(r.data);
      assertEquals("*****", Util.ReadFileAsUtf8String(p.outputFilePath));
    } finally {
      (new File(p.outputFilePath)).delete();
    }
  }

  public void testPostFromMemoryIntoMemory() throws Exception {
    final String postBody = "Hello, World!";
    final HttpTransport.Params p = new HttpTransport.Params(HTTPBIN_POST_URL);
    p.setData(postBody.getBytes(), "application/octet-stream", "POST");
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(200, r.httpResponseCode);
    final String receivedBody = new String(r.data);
    // Server mirrors our content.
    assertTrue(receivedBody, receivedBody.contains(postBody));
  }

  public void testPutFromMemoryIntoMemory() throws Exception {
    final String putBody = "This is HTTP PUT request";
    final HttpTransport.Params p = new HttpTransport.Params("http://httpbin.org/put");
    p.setData(putBody.getBytes(), "application/octet-stream", "PUT");
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(200, r.httpResponseCode);
    final String receivedBody = new String(r.data);
    // Server mirrors our content.
    assertTrue(receivedBody, receivedBody.contains(putBody));
  }

  public void testPostMissingContentType() throws Exception {
    // here is missing p.contentType = "application/octet-stream";
    final HttpTransport.Params p = new HttpTransport.Params(HTTPBIN_POST_URL);
    p.setData("Hello, World!".getBytes(), null, "POST");
    boolean caughtException = false;
    try {
      final HttpTransport.Params r = HttpTransport.run(p);
      assertFalse(true);
    } catch (NullPointerException ex) {
      caughtException = true;
    }
    assertTrue(caughtException);
  }

  public void testPostFromInvalidFile() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params(HTTPBIN_POST_URL);
    p.setInputFilePath(getFullWritablePathForFile("this_file_should_not_exist"), "text/plain", "POST");
    boolean caughtException = false;
    try {
      final HttpTransport.Params r = HttpTransport.run(p);
      assertFalse(true);
    } catch (FileNotFoundException ex) {
      caughtException = true;
    }
    assertTrue(caughtException);
  }

  public void testPostFromFileIntoMemory() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params(HTTPBIN_POST_URL);
    p.setInputFilePath(getFullWritablePathForFile("some_input_test_file_for_http_post"), "text/plain", "POST");
    try {
      // Use file name as a test string for the post body.
      Util.WriteStringToFile(p.inputFilePath, p.inputFilePath);
      final HttpTransport.Params r = HttpTransport.run(p);
      assertEquals(200, r.httpResponseCode);
      final String receivedBody = new String(p.data);
      assertTrue(receivedBody, receivedBody.contains(p.inputFilePath));
    } finally {
      (new File(p.inputFilePath)).delete();
    }
  }

  public void testPostFromMemoryIntoFile() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params(HTTPBIN_POST_URL);
    p.setData(HTTPBIN_POST_URL.getBytes(), "text/plain", "POST");
    p.outputFilePath = getFullWritablePathForFile("some_output_test_file_for_http_post");
    try {
      final HttpTransport.Params r = HttpTransport.run(p);
      assertEquals(200, r.httpResponseCode);
      // TODO(AlexZ): Think about using data field in the future for error pages (404 etc)
      //assertNull(r.data);
      final String receivedBody = Util.ReadFileAsUtf8String(p.outputFilePath);
      assertTrue(receivedBody, receivedBody.contains("\"data\": \"" + HTTPBIN_POST_URL + "\""));
    } finally {
      (new File(p.outputFilePath)).delete();
    }
  }

  public void testPostFromFileIntoFile() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params(HTTPBIN_POST_URL);
    p.setInputFilePath(getFullWritablePathForFile("some_complex_input_test_file_for_http_post"), "text/plain", "POST");
    p.outputFilePath = getFullWritablePathForFile("some_complex_output_test_file_for_http_post");
    final String postBodyToSend = "Aloha, this text should pass from one file to another. Mahalo!";
    try {
      Util.WriteStringToFile(postBodyToSend, p.inputFilePath);
      final HttpTransport.Params r = HttpTransport.run(p);
      assertEquals(200, r.httpResponseCode);
      final String receivedBody = Util.ReadFileAsUtf8String(p.outputFilePath);
      assertTrue(receivedBody, receivedBody.contains(postBodyToSend));
    } finally {
      (new File(p.inputFilePath)).delete();
      (new File(p.outputFilePath)).delete();
    }
  }

  public void testErrorCodes() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("http://httpbin.org/status/403");
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(403, r.httpResponseCode);
  }

  public void testHttps() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("https://httpbin.org/get?Aloha=Mahalo");
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(200, r.httpResponseCode);
    assertEquals(r.url, r.receivedUrl);
    final String receivedBody = new String(r.data);
    assertTrue(receivedBody.contains("\"Aloha\": \"Mahalo\""));
  }

  public void testHttpRedirect302() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("http://httpbin.org/redirect-to?url=http%3A%2F%2Falohalytics.org%2F");
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(302, r.httpResponseCode);
    assertEquals(r.receivedUrl, "http://alohalytics.org/");
    assertTrue(!r.url.equals(r.receivedUrl));
  }

  public void testUserAgent() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("http://httpbin.org/user-agent");
    p.userAgent = "Aloha User Agent";
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(200, r.httpResponseCode);
    final String receivedBody = new String(r.data);
    assertTrue(receivedBody.contains(p.userAgent));
  }

  public void testHttpRedirect301() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("http://maps.me");
    final HttpTransport.Params r = HttpTransport.run(p);
    // Client should not follow redirects automatically.
    assertEquals(301, r.httpResponseCode);
    assertEquals(r.receivedUrl, "http://maps.me/en/");
    assertTrue(!r.url.equals(r.receivedUrl));
  }

  public void testInvalidHost() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("http://very.bad.host");
    boolean caughtException = false;
    try {
      final HttpTransport.Params r = HttpTransport.run(p);
      assertEquals(-1, r.httpResponseCode);
      assertFalse(true);
    } catch (java.net.UnknownHostException ex) {
      caughtException = true;
    }
    assertTrue(caughtException);
  }

  public void testPostFromEmptyFileIntoMemory() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params(HTTPBIN_POST_URL);
    p.setInputFilePath(getFullWritablePathForFile("empty_input_test_file_for_http_post"), "text/plain", "POST");
    try {
      Util.WriteStringToFile("", p.inputFilePath);
      final HttpTransport.Params r = HttpTransport.run(p);
      assertEquals(200, r.httpResponseCode);
      final String receivedBody = new String(p.data);
      assertTrue(receivedBody, receivedBody.contains("\"data\": \"\""));
      assertTrue(receivedBody, receivedBody.contains("\"form\": {}"));
    } finally {
      (new File(p.inputFilePath)).delete();
    }
  }

  public void testResponseContentType() throws Exception {
    final HttpTransport.Params p = new HttpTransport.Params("http://httpbin.org/get");
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(200, r.httpResponseCode);
    assertEquals(p.url, r.receivedUrl);
    assertEquals("application/json", r.contentType);
  }

  public void testHttpBasicAuth() throws Exception {
    final String user = "user";
    final String password = "password";
    final HttpTransport.Params p = new HttpTransport.Params("http://httpbin.org/basic-auth/" + user + "/" + password);
    p.basicAuthUser = user;
    p.basicAuthPassword = password;
    final HttpTransport.Params r = HttpTransport.run(p);
    assertEquals(200, r.httpResponseCode);
    assertEquals(p.url, r.receivedUrl);
    final String receivedBody = new String(r.data);
    assertTrue(receivedBody, receivedBody.contains("\"authenticated\": true"));
  }

}
