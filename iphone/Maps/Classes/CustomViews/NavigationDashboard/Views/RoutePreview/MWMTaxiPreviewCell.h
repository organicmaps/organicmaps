namespace uber
{
  struct Product;
}  // namespace uber;

@interface MWMTaxiPreviewCell : UICollectionViewCell

- (void)configWithProduct:(uber::Product const &)product;

@end
