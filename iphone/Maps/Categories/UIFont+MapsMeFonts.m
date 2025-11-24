@implementation UIFont (MapsMeFonts)

+ (UIFont *)regular10
{
  return [UIFont systemFontOfSize:10];
}
+ (UIFont *)regular12
{
  return [UIFont systemFontOfSize:12];
}
+ (UIFont *)regular13
{
  return [UIFont systemFontOfSize:13];
}
+ (UIFont *)regular14
{
  return [UIFont systemFontOfSize:14];
}
+ (UIFont *)regular16
{
  return [UIFont systemFontOfSize:16];
}
+ (UIFont *)regular17
{
  return [UIFont systemFontOfSize:17];
}
+ (UIFont *)regular18
{
  return [UIFont systemFontOfSize:18];
}
+ (UIFont *)regular24
{
  return [UIFont systemFontOfSize:24];
}
+ (UIFont *)regular32
{
  return [UIFont systemFontOfSize:32];
}
+ (UIFont *)regular52
{
  return [UIFont systemFontOfSize:52];
}
+ (UIFont *)medium10
{
  return [UIFont systemFontOfSize:10 weight:UIFontWeightMedium];
}
+ (UIFont *)medium14
{
  return [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
}
+ (UIFont *)medium16
{
  return [UIFont systemFontOfSize:16 weight:UIFontWeightMedium];
}
+ (UIFont *)medium17
{
  return [UIFont systemFontOfSize:17 weight:UIFontWeightMedium];
}
+ (UIFont *)light12
{
  return [UIFont systemFontOfSize:12 weight:UIFontWeightLight];
}
+ (UIFont *)bold12
{
  return [UIFont boldSystemFontOfSize:12];
}
+ (UIFont *)bold14
{
  return [UIFont boldSystemFontOfSize:14];
}
+ (UIFont *)bold16
{
  return [UIFont boldSystemFontOfSize:16];
}
+ (UIFont *)bold17
{
  return [UIFont boldSystemFontOfSize:17];
}
+ (UIFont *)bold24
{
  return [UIFont boldSystemFontOfSize:24];
}
+ (UIFont *)bold28
{
  return [UIFont boldSystemFontOfSize:28];
}
+ (UIFont *)bold36
{
  return [UIFont boldSystemFontOfSize:26];
}
+ (UIFont *)semibold16
{
  return [UIFont systemFontOfSize:16 weight:UIFontWeightSemibold];
}
+ (UIFont *)semibold17
{
  return [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
}
+ (UIFont *)emojiRegular14
{
  static UIFont *font = nil;
  if (font == nil)
  {
    UIFont *emojiFont = [UIFont fontWithName:@"OrganicMapsEmoji" size:14];
    UIFont *fallbackFont = [UIFont systemFontOfSize:14 weight:UIFontWeightRegular];
    UIFontDescriptor *cascadeDescriptor = [emojiFont.fontDescriptor
      fontDescriptorByAddingAttributes:@{ UIFontDescriptorCascadeListAttribute: @[ fallbackFont.fontDescriptor ]}];
    font = [UIFont fontWithDescriptor:cascadeDescriptor size:14];
  }
  return font;
}
+ (UIFont *)emojiMedium13
{
  static UIFont *font = nil;
  if (font == nil)
  {
    UIFont *emojiFont = [UIFont fontWithName:@"OrganicMapsEmoji" size:13];
    UIFont *fallbackFont = [UIFont systemFontOfSize:13 weight:UIFontWeightMedium];
    UIFontDescriptor *cascadeDescriptor = [emojiFont.fontDescriptor
      fontDescriptorByAddingAttributes:@{ UIFontDescriptorCascadeListAttribute: @[ fallbackFont.fontDescriptor ]}];
    font = [UIFont fontWithDescriptor:cascadeDescriptor size:13];
  }
  return font;
}

@end
