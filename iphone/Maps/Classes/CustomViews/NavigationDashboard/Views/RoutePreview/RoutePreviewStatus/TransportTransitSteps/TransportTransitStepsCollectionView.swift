final class TransportTransitStepsCollectionView: UICollectionView {
  var steps: [MWMRouterTransitStepInfo] = [] {
    didSet {
      reloadData()
    }
  }

  override var frame: CGRect {
    didSet {
      collectionViewLayout.invalidateLayout()
    }
  }

  override var bounds: CGRect {
    didSet {
      collectionViewLayout.invalidateLayout()
    }
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    dataSource = self
    [TransportTransitIntermediatePoint.self, TransportTransitPedestrian.self,
        TransportTransitTrain.self, TransportRuler.self].forEach {
      register(cellClass: $0)
    }
  }

  private func cellClass(item: Int) -> TransportTransitCell.Type {
    let step = steps[item]
    switch step.type {
    case .intermediatePoint: return TransportTransitIntermediatePoint.self
    case .pedestrian: return TransportTransitPedestrian.self
    case .train: fallthrough
    case .subway: fallthrough
    case .lightRail: fallthrough
    case .monorail: return TransportTransitTrain.self
    case .ruler: return TransportRuler.self
    }
  }

  func estimatedCellSize(item: Int) -> CGSize {
    return cellClass(item: item).estimatedCellSize(step: steps[item])
  }
}

extension TransportTransitStepsCollectionView: UICollectionViewDataSource {
  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    return steps.count
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let item = indexPath.item
    let cellClass = self.cellClass(item: item)
    let cell = collectionView.dequeueReusableCell(withCellClass: cellClass, indexPath: indexPath) as! TransportTransitCell
    cell.config(step: steps[item])
    return cell
  }
}
