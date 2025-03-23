package org.nanohttpd.protocols.http.tempfiles;

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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import org.nanohttpd.protocols.http.NanoHTTPD;

/**
 * Default strategy for creating and cleaning up temporary files.
 * <p/>
 * <p>
 * By default, files are created by <code>File.createTempFile()</code> in the
 * directory specified.
 * </p>
 */
public class DefaultTempFile implements ITempFile {

    private final File file;

    private final OutputStream fstream;

    public DefaultTempFile(File tempdir) throws IOException {
        this.file = File.createTempFile("NanoHTTPD-", "", tempdir);
        this.fstream = new FileOutputStream(this.file);
    }

    @Override
    public void delete() throws Exception {
        NanoHTTPD.safeClose(this.fstream);
        if (!this.file.delete()) {
            throw new Exception("could not delete temporary file: " + this.file.getAbsolutePath());
        }
    }

    @Override
    public String getName() {
        return this.file.getAbsolutePath();
    }

    @Override
    public OutputStream open() throws Exception {
        return this.fstream;
    }
}
