final class TransportTransitIntermediatePoint: TransportTransitCell {
  enum Config {
    static var imageColor: UIColor { UIColor.primary() }
  }

  @IBOutlet private var image: UIImageView!
  override func config(step: MWMRouterTransitStepInfo) {
    super.config(step: step)
    let i = step.intermediateIndex + 1
    // TODO: Properly support more than 20 icons.
    var iconName = "route-point-20"
    if i >= 1, i < 20 {
      iconName = "route-point-" + String(i)
    }
    image.image = #imageLiteral(resourceName: iconName)
    image.tintColor = Config.imageColor
  }
}
