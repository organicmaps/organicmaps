static NSTimeInterval const kMenuViewHideFramesCount = 4.0;

static inline NSTimeInterval framesDuration(NSTimeInterval const framesCount)
{
  static NSTimeInterval const kFPS = 30.0;
  static NSTimeInterval const kFrameDuration = 1.0 / kFPS;
  return kFrameDuration * framesCount;
}

static CGFloat const kViewControlsOffsetToBounds = 6;
