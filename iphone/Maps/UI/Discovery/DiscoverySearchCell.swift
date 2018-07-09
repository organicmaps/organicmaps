@objc(MWMDiscoverySearchCell)
final class DiscoverySearchCell: UICollectionViewCell {
  @IBOutlet private weak var title: UILabel!
  @IBOutlet private weak var subtitle: UILabel!
  @IBOutlet private weak var distance: UILabel!
  
  typealias Tap = () -> ()
  private var tap: Tap?

  @objc func config(title: String, subtitle: String, distance: String, tap: @escaping Tap) {
    self.title.text = title
    self.subtitle.text = subtitle
    self.distance.text = distance
    self.tap = tap
  }

  @IBAction private func routeTo() {
    tap?()
  }

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
    layer.borderColor = UIColor.blackDividers().cgColor
  }
}
