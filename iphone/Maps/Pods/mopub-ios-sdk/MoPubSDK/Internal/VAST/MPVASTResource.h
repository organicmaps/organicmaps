//
//  MPVASTResource.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

/**
 Per VAST 3.0 spec, section 2.3.3.2 "Companion Resource Elements":

 Companion resource types are described below:
 • StaticResource: Describes non-html creative where an attribute for creativeType is used to
 identify the creative resource platform. The video player uses the creativeType information to
 determine how to display the resource:
    o Image/gif,image/jpeg,image/png:displayedusingtheHTMLtag<img>andthe resource URI as the src attribute.
    o Application/x-javascript:displayedusingtheHTMLtag<script>andtheresource URI as the src attribute.
    o application/x-shockwave-flash:displayedusingaFlashTMplayer
 • IFrameResource: Describes a resource that is an HTML page that can be displayed within an Iframe
 on the publisher’s page.
 • HTMLResource: Describes a “snippet” of HTML code to be inserted directly within the publisher’s
 HTML page code.
 */
@interface MPVASTResource : MPVASTModel

@property (nonatomic, strong, readonly) NSString *content;
@property (nonatomic, strong, readonly) NSString *staticCreativeType;

- (BOOL)isStaticCreativeTypeImage;

- (BOOL)isStaticCreativeTypeJavaScript;

@end
