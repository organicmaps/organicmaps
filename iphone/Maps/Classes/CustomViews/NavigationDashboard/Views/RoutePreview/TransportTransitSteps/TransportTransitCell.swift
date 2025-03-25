class TransportTransitCell: UICollectionViewCell {
  enum Config {
    static let cellSize = CGSize(width: 20, height: 20)
  }

  class func estimatedCellSize(step _: MWMRouterTransitStepInfo) -> CGSize {
    return Config.cellSize
  }

  func config(step _: MWMRouterTransitStepInfo) {}
}
