final class TransportTransitStepsCollectionView: UICollectionView {
  var steps: [MWMRouterTransitStepInfo] = [] {
    didSet {
      updateDisplayedSteps()
    }
  }

  private struct DisplayedStep {
    let info: MWMRouterTransitStepInfo
    let number: String
    let isContinuation: Bool
  }

  private var displayedSteps = [DisplayedStep]()
  private var displayedWidth: CGFloat = 0
  private(set) var stepFrames: (cells: [CGRect], separators: [CGRect]) = ([], [])

  override func layoutSubviews() {
    if displayedWidth != bounds.width {
      updateDisplayedSteps()
    }
    super.layoutSubviews()
  }

  override var frame: CGRect {
    didSet {
      if oldValue.size != frame.size {
        collectionViewLayout.invalidateLayout()
      }
    }
  }

  override var bounds: CGRect {
    didSet {
      if oldValue.size != bounds.size {
        collectionViewLayout.invalidateLayout()
      }
    }
  }

  init(layout: UICollectionViewLayout) {
    super.init(frame: .zero, collectionViewLayout: layout)
    setup()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    setup()
  }

  private func setup() {
    dataSource = self
    [TransportTransitIntermediatePoint.self, TransportTransitPedestrian.self,
     TransportTransitTrain.self, TransportRuler.self].forEach {
      register(cellClass: $0)
    }
  }

  private func cellClass(step: MWMRouterTransitStepInfo) -> TransportTransitCell.Type {
    switch step.type {
    case .intermediatePoint: return TransportTransitIntermediatePoint.self
    case .pedestrian: return TransportTransitPedestrian.self
    case .train, .subway, .lightRail, .monorail, .tram, .bus,
         .ferry, .cableTram, .aerialLift, .funicular, .airService, .waterService:
      return TransportTransitTrain.self
    case .ruler: return TransportRuler.self
    }
  }

  private func updateDisplayedSteps() {
    displayedWidth = bounds.width
    var cursor = TransportTransitFlowLayout.Cursor(width: displayedWidth)
    displayedSteps = steps.enumerated().flatMap { index, step -> [DisplayedStep] in
      let hasFollowingStep = index + 1 < steps.count
      var numbers = [step.number ?? ""]
      if displayedWidth > 0, cellClass(step: step) == TransportTransitTrain.self {
        numbers = TransportTransitTrain.numberGroups(step.number, maximumWidth: displayedWidth,
                                                     firstRowWidth: cursor.remainingWidth,
                                                     hasFollowingStep: hasFollowingStep)
      }
      return numbers.enumerated().map { fragmentIndex, number in
        let fragment = DisplayedStep(info: step, number: number, isContinuation: fragmentIndex > 0)
        let size = estimatedCellSize(step: fragment)
        cursor.append(size: size, hasFollowingStep: hasFollowingStep && fragmentIndex + 1 == numbers.count,
                      isContinuation: fragment.isContinuation)
        return fragment
      }
    }
    stepFrames = cursor.frames
    reloadData()
  }

  private func estimatedCellSize(step: DisplayedStep) -> CGSize {
    if cellClass(step: step.info) == TransportTransitTrain.self {
      return TransportTransitTrain.estimatedCellSize(number: step.number, isContinuation: step.isContinuation)
    }
    return cellClass(step: step.info).estimatedCellSize(step: step.info)
  }
}

extension TransportTransitStepsCollectionView: UICollectionViewDataSource {
  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    displayedSteps.count
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let item = indexPath.item
    let displayedStep = displayedSteps[item]
    let step = displayedStep.info
    let cellClass = cellClass(step: step)
    let cell = collectionView.dequeueReusableCell(withCellClass: cellClass, indexPath: indexPath) as! TransportTransitCell
    if let trainCell = cell as? TransportTransitTrain {
      trainCell.config(step: step, number: displayedStep.number, isContinuation: displayedStep.isContinuation)
    } else {
      cell.config(step: step)
    }
    return cell
  }
}
