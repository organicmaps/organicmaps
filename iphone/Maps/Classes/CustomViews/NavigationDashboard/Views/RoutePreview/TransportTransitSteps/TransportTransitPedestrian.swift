final class TransportTransitPedestrian: TransportTransitCell {
  enum Config {
    static var backgroundColor: UIColor { UIColor.blackOpaque() }
    static var imageColor: UIColor { UIColor.blackSecondaryText() }
  }

  @IBOutlet private var background: UIView! {
    didSet {
      background.layer.setCornerRadius(.buttonSmall)
      background.backgroundColor = Config.backgroundColor
    }
  }

  @IBOutlet private var image: UIImageView! {
    didSet {
      image.image = UIImage(resource: .icWalk)
      image.tintColor = Config.imageColor
      image.contentMode = .scaleAspectFit
    }
  }
}
