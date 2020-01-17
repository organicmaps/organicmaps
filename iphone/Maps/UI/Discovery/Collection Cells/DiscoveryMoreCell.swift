@objc(MWMDiscoveryMoreCell)
final class DiscoveryMoreCell: UICollectionViewCell {
  override var isHighlighted: Bool {
    didSet {
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     delay: 0,
                     options: [.allowUserInteraction, .beginFromCurrentState],
                     animations: { self.alpha = self.isHighlighted ? 0.3 : 1 },
                     completion: nil)
    }
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
  }
}
