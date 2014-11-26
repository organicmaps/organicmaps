#!/usr/bin/env gawk -f
# ----------------------------------------------------------------------------
# "THE BEER-WARE LICENSE" (Revision 42):
# <mort.yao@gmail.com> wrote this file. As long as you retain this notice you
# can do whatever you want with this stuff. If we meet some day, and you think
# this stuff is worth it, you can buy me a beer in return Mort Yao
# ----------------------------------------------------------------------------
BEGIN {
    if (ARGC < 2) {
        help()
        exit
    }
    
    RS = ORS = "\r\n"
    FS = OFS = "\n"
    httpHost = "translate.google.com"
    httpPort = 80
    httpService = "/inet/tcp/0/" httpHost "/" httpPort
    percentEncoding["\n"]      = "%0A"
    percentEncoding[" "]       = "%20"
    percentEncoding["\""]      = "%22"
    percentEncoding["&"]       = "%26"
    
    match(ARGV[1], /{.*=.*}/)
    if (RSTART) {
        match(ARGV[1], /{.*=/)
        sl = substr(ARGV[1], RSTART + 1, RLENGTH - 2)
        if (sl == "") sl = "auto"
        
        match(ARGV[1], /=.*}/)
        split(substr(ARGV[1], RSTART + 1, RLENGTH - 2), tls, "+")
        if (length(tls) == 0) tls[1] = ""
        
        text_p = 1
    } else {
        sl = tls[1] = "auto"
        text_p = 0
    }
    
    while (ARGV[++text_p]) {
        $0 = ""
        getline < ARGV[text_p]
        if (NF)
            split($0, texts)
        else
            split(ARGV[text_p], texts)
        n = length(texts)
        
        for (tls_i = 1; tls_i <= length(tls); tls_i++) {
            tl = ((tls[tls_i] == "") ? "auto" : tls[tls_i])
            tl = substr(tl, 1 + (showPhonetically = index(tl, "@")))
            
            for (texts_i = 1; texts_i <= n; texts_i++) {
                text = texts[texts_i]
                for (key in percentEncoding)
                    gsub(key, percentEncoding[key], text)
                
                url = "http://translate.google.com/translate_a/t?client=t&ie=UTF-8&oe=UTF-8" \
                    "&sl=" sl "&tl=" tl "&text=" text
                
                print "GET " url |& httpService
                while ((httpService |& getline) > 0)
                    content = $0
                close(httpService)
                
                translation = phonetics = ""
                match(content, /\]\]/)
                s0 = substr(content, 3, RSTART - 2)
                while (s0 != "") {
                    match(s0, /\[[^\]]*\]/)
                    t0 = substr(s0, RSTART + 1, RLENGTH - 2)
                    s0 = substr(s0, RLENGTH + 2)
                    
                    for (i = 0; t0 != ""; i++) {
                        match(t0, /[^\\]","/)
                        f[i] = substr(t0, 2, RSTART - 1)
                        t0 = substr(t0, RSTART + 3)
                    }
                    
                    translation = translation f[0]
                    phonetics = phonetics ((f[2] != "") ? (f[2] " ") : f[0])
                }
                
                output = showPhonetically ? phonetics : translation
                output = gensub(/\\( |\\|\")/, "\\1", "g", output)
                output = gensub(/\\n/, "\n", "g", output)
                output = gensub(/(\") /, "\\1", "g", output)
                output = gensub(/ ('|;|:|\.|\,|\!|\?)/, "\\1", "g", output)
                printf tl ":" output "\n"
            }
        }
    }
}

function help() {
    print "Usage: translate {[SL]=[TL]} TEXT|TEXT_FILENAME"
    print "       translate {[SL]=[TL1]+[TL2]+...} TEXT|TEXT_FILENAME"
    print "       translate TEXT1 TEXT2 ..."
    print
    print "TEXT: Source text (The text to be translated)"
    print "      Can also be the filename of a plain text file."
    print "  SL: Source language (The language of the source text)"
    print "  TL: Target language (The language to translate the source text into)"
    print "      Language codes as listed here:"
    print "    * http://developers.google.com/translate/v2/using_rest#language-params"
    print "      Ignore the code where you want the system to identify it for you."
    print "      Prefix the code with an ampersat @ to show the result phonetically."
}
