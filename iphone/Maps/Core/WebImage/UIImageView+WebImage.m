#import "UIImageView+WebImage.h"
#import "MWMWebImage.h"

#import <objc/runtime.h>

static char kAssociatedObjectKey;

@implementation UIImageView (WebImage)

- (void)wi_setImageWithUrl:(NSURL *)url {
  [self wi_setImageWithUrl:url transitionDuration:0 completion:nil];
}

- (void)wi_setImageWithUrl:(NSURL *)url
        transitionDuration:(NSTimeInterval)duration
                completion:(void (^)(UIImage *, NSError *))completion {
  id<IMWMImageTask> task = [[MWMWebImage defaultWebImage] imageWithUrl:url
                                                            completion:^(UIImage *image, NSError *error) {
    objc_setAssociatedObject(self, &kAssociatedObjectKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (!image) {
      if (completion) { completion(nil, error); }
      return;
    }

    if (duration > 0) {
      [UIView transitionWithView:self
                        duration:duration
                         options:UIViewAnimationOptionTransitionCrossDissolve
                      animations:^{ self.image = image; }
                      completion:nil];
    } else {
      self.image = image;
    }
                                                              
    if (completion) { completion(image, nil); }
  }];
  objc_setAssociatedObject(self, &kAssociatedObjectKey, task, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (void)wi_cancelImageRequest {
  id<IMWMImageTask> task = objc_getAssociatedObject(self, &kAssociatedObjectKey);
  [task cancel];
  objc_setAssociatedObject(self, &kAssociatedObjectKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end
