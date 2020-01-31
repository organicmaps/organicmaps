protocol PlacePagePresenterProtocol: class {
  var maxOffset: CGFloat { get }

  func configure()
  func setAdState(_ state: AdBannerState)
  func updatePreviewOffset()
  func layoutIfNeeded()
  func findNextStop(_ offset: CGFloat, velocity: CGFloat) -> PlacePageState
}

class PlacePagePresenter: NSObject {
  private weak var view: PlacePageViewProtocol!
  private let interactor: PlacePageInteractorProtocol
  private let isPreviewPlus: Bool
  private let layout: IPlacePageLayout
  private var scrollSteps:[PlacePageState] = []


  init(view: PlacePageViewProtocol,
       interactor: PlacePageInteractorProtocol,
       layout: IPlacePageLayout,
       isPreviewPlus: Bool) {
    self.view = view
    self.interactor = interactor
    self.layout = layout
    self.isPreviewPlus = isPreviewPlus
  }
}

// MARK: - PlacePagePresenterProtocol

extension PlacePagePresenter: PlacePagePresenterProtocol {
  var maxOffset: CGFloat {
    get {
      return scrollSteps.last?.offset ?? 0
    }
  }

  func configure() {
    for viewController in layout.viewControllers {
      view.addToStack(viewController)
    }
    
    if let actionBar = layout.actionBar {
      view.hideActionBar(false)
      view.addActionBar(actionBar)
    } else {
      view.hideActionBar(true)
    }
  }

  func setAdState(_ state: AdBannerState) {
    layout.adState = state
  }

  func updatePreviewOffset() {
    layoutIfNeeded()
    scrollSteps = layout.calculateSteps(inScrollView: view.scrollView)
    let state = isPreviewPlus ? scrollSteps[2] : scrollSteps[1]
    view.scrollTo(CGPoint(x: 0, y: state.offset))
  }

  func layoutIfNeeded() {
    view.layoutIfNeeded()
  }

  private func findNearestStop(_ offset: CGFloat) -> PlacePageState{
    var result = scrollSteps[0]
    scrollSteps.suffix(from: 1).forEach { ppState in
      if abs(result.offset - offset) > abs(ppState.offset - offset) {
        result = ppState
      }
    }
    return result
  }

  func findNextStop(_ offset: CGFloat, velocity: CGFloat) -> PlacePageState {
    if velocity == 0 {
      return findNearestStop(offset)
    }

    var result: PlacePageState
    if velocity < 0 {
      guard let first = scrollSteps.first else { return .closed(-view.scrollView.height) }
      result = first
      scrollSteps.suffix(from: 1).forEach {
        if offset > $0.offset {
          result = $0
        }
      }
    } else {
      guard let last = scrollSteps.last else { return .closed(-view.scrollView.height) }
      result = last
      scrollSteps.reversed().suffix(from: 1).forEach {
        if offset < $0.offset {
          result = $0
        }
      }
    }

    return result
  }
}
