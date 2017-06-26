namespace taxi
{
struct Product;
}  // namespace taxi;

@interface MWMTaxiPreviewCell : UICollectionViewCell

- (void)configWithProduct:(taxi::Product const &)product;

@end
