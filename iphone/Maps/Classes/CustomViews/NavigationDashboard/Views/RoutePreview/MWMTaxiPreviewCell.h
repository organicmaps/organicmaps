namespace taxi
{
struct Product;
}

@interface MWMTaxiPreviewCell : UICollectionViewCell

- (void)configWithProduct:(taxi::Product const &)product;

@end
