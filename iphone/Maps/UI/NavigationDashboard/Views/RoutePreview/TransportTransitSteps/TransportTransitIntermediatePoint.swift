final class TransportTransitIntermediatePoint: TransportTransitCell {
  @IBOutlet private var image: UIImageView!

  override func config(step: MWMRouterTransitStepInfo) {
    image.image = RoutePointIconRenderer.image(number: step.intermediateIndex + 1)
  }
}
