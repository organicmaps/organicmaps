final class TransportTransitIntermediatePoint: TransportTransitCell {
  enum Config {
    static let imageColor = UIColor.primary()!
  }

  @IBOutlet private weak var image: UIImageView!
  override func config(step: MWMRouterTransitStepInfo) {
    super.config(step: step)
    switch step.intermediateIndex {
    case 0: image.image = #imageLiteral(resourceName: "ic_route_manager_stop_a")
    case 1: image.image = #imageLiteral(resourceName: "ic_route_manager_stop_b")
    case 2: image.image = #imageLiteral(resourceName: "ic_route_manager_stop_c")
    default: fatalError("Unsupported route point intermediateIndex.")
    }
    image.tintColor = Config.imageColor
  }
}
