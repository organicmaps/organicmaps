final class TransportTransitPedestrian: TransportTransitCell {
  enum Config {
    static let backgroundCornerRadius: CGFloat = 4
    static var backgroundColor: UIColor { return UIColor.blackOpaque() }
    static var imageColor: UIColor { return UIColor.blackSecondaryText() }
  }

  @IBOutlet private weak var background: UIView! {
    didSet {
      background.layer.cornerRadius = Config.backgroundCornerRadius
      background.backgroundColor = Config.backgroundColor
    }
  }

  @IBOutlet private weak var image: UIImageView! {
    didSet {
      image.image = #imageLiteral(resourceName: "ic_walk")
      image.tintColor = Config.imageColor
      image.contentMode = .scaleAspectFit
    }
  }
}
