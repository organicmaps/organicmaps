final class TransportTransitPedestrian: TransportTransitCell {
  enum Config {
    static var backgroundColor: UIColor { return UIColor.blackOpaque() }
    static var imageColor: UIColor { return UIColor.blackSecondaryText() }
  }

  @IBOutlet private weak var background: UIView! {
    didSet {
      background.layer.setCornerRadius(.buttonSmall)
      background.backgroundColor = Config.backgroundColor
    }
  }

  @IBOutlet private weak var image: UIImageView! {
    didSet {
      image.image = UIImage(resource: .icWalk)
      image.tintColor = Config.imageColor
      image.contentMode = .scaleAspectFit
    }
  }
}
