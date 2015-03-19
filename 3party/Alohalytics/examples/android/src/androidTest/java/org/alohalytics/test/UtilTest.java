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

import java.io.File;
import java.io.FileNotFoundException;

public class UtilTest extends InstrumentationTestCase {

  @Override
  protected void setUp() {
    new File(org.alohalytics.test.HttpTransportTest.CACHE_DIR).mkdirs();
  }

  private String getFullWritablePathForFile(String fileName) {
    return org.alohalytics.test.HttpTransportTest.CACHE_DIR + fileName;
  }

  public void testReadAndWriteStringToFile() throws Exception {
    final String testFileName = getFullWritablePathForFile("test_write_string_to_file");
    try {
      Util.WriteStringToFile(testFileName, testFileName);
      assertEquals(testFileName, Util.ReadFileAsUtf8String(testFileName));
    } finally {
      new File(testFileName).delete();
    }
  }

  public void testFailedWriteStringToFile() throws Exception {
    final String invalidFileName = getFullWritablePathForFile("invalid?path\\with/slashes");
    boolean caughtException = false;
    try {
      Util.WriteStringToFile(invalidFileName, invalidFileName);
      assertFalse(true);
    } catch (FileNotFoundException ex) {
      caughtException = true;
    } finally {
      new File(invalidFileName).delete();
    }
    assertTrue(caughtException);
  }

  public void testFailedReadFileAsString() throws Exception {
    final String notExistingFileName = getFullWritablePathForFile("this_file_should_not_exist_for_the_test");
    boolean caughtException = false;
    try {
      final String s = Util.ReadFileAsUtf8String(notExistingFileName);
      assertFalse(true);
    } catch (FileNotFoundException ex) {
      caughtException = true;
    }
    assertTrue(caughtException);
  }

}
