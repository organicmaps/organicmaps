namespace taxi {
struct Product;
}  // namespace uber;

@interface MWMTaxiPreviewCell : UICollectionViewCell

- (void)configWithProduct:(taxi::Product const &)product;

@end
