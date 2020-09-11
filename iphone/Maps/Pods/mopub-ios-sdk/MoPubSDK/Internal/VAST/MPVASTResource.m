//
//  MPVASTResource.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTResource.h"

@implementation MPVASTResource

+ (NSDictionary *)modelMap
{
    return @{@"content":            @"text",
             @"staticCreativeType": @"creativeType"};
}

- (BOOL)isStaticCreativeTypeImage {
    return ([self.staticCreativeType.lowercaseString isEqualToString:@"image/gif"]
            || [self.staticCreativeType.lowercaseString isEqualToString:@"image/jpeg"]
            || [self.staticCreativeType.lowercaseString isEqualToString:@"image/png"]);
}

- (BOOL)isStaticCreativeTypeJavaScript {
    return [self.staticCreativeType.lowercaseString isEqualToString:@"application/x-javascript"];
}

+ (NSString *)fullHTMLRespresentationForContent:(NSString *)content
                                           type:(MPVASTResourceType)type
                                  containerSize:(CGSize)containerSize {
    switch (type) {
        case MPVASTResourceType_Undetermined:
        case MPVASTResourceType_HTML:
            return content;
        case MPVASTResourceType_StaticImage: {
            NSString *staticImageResourceFormat =
@"<html>\
    <head>\
        <title>Static Image Resource</title>\
        <meta name=\"viewport\" content=\"initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">\
        <style type=\"text/css\">\
            html, body { margin: 0; padding: 0; overflow: hidden; }\
            #content { width: %.0fpx; height: %.0fpx; }\
        </style>\
    </head>\
    <body scrolling=\"no\">\
        <div id=\"content\">\
            <img src=\"%@\">\
        </div>\
    </body>\
</html>";
            return [NSString stringWithFormat:staticImageResourceFormat,
                    containerSize.width, containerSize.height, content];
        }
        case MPVASTResourceType_StaticScript: {
            /*
            For simple localhost setup to serve a JavaScript file:
               * In Terminal, run `sudo apachectl start` to start localhost
               * Creat a sample.js file in /Library/WebServer/Documents
               * Put the following Javascript code in the sample.js file:
                   document.write('Hello World! Javascript is loaded');
               * Check it's working by opening "http://localhost/sample.js" in a browser
               * Now run the app
            */
            NSString *javaScriptResourceFormat =
@"<html>\
    <head>\
        <title>Static JavaScript Resource</title>\
        <meta name=\"viewport\" content=\"initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">\
        <style type=\"text/css\">\
            html, body { margin: 0; padding: 0; overflow: hidden; }\
        </style>\
        <script type=\"text/javascript\" src=\"%@\"></script>\
    </head>\
    <body scrolling=\"no\"></body>\
</html>";
            return [NSString stringWithFormat:javaScriptResourceFormat, content];
        }
        case MPVASTResourceType_Iframe: {
            /*
             To make the iframe taking full body area, margin and padding have to set to 0 to override the
             original value; the iframe tag has to be contained in a div that position itself absolutely;
             set "marginwidth" and "marginheight" as 0 since these two are not controlled by CSS styling.

             WARNING: For the format string, remember to escape the "%" character as "%%", not "\%".
             */
            NSString *iframeResourceFormat =
@"<html>\
    <head>\
        <title>Iframe Resource</title>\
        <meta name=\"viewport\" content=\"initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">\
        <style type=\"text/css\">\
            html, body { margin: 0; padding: 0; overflow: hidden; }\
        </style>\
    </head>\
    <body scrolling=\"no\">\
        <div id=\"content\">\
            <iframe src=\"%@\" width=\"100%%\" height=\"100%%\"\
            frameborder=0 marginwidth=0 marginheight=0 scrolling=\"no\">\
            </iframe>\
        </div>\
    </body>\
</html>";
            return [NSString stringWithFormat:iframeResourceFormat, content];
        }
    }
}

@end
