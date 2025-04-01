extension RoutePreview {
  struct ViewModel: Equatable {
    let transportOptions: [MWMRouterType] = MWMRouterType.allCases
    var routePoints: RoutePoints = .empty
    var routerType: MWMRouterType
    var entity: MWMNavigationDashboardEntity
    var estimates: NSAttributedString = NSAttributedString()
    var state: MWMNavigationDashboardState
    var presentationStep: ModalPresentationStep
    var shouldClose: Bool = false
    var progress: CGFloat = 0
  }
}

extension RoutePreview.ViewModel {
  static let initial = RoutePreview.ViewModel(
    routerType: .vehicle,
    entity: MWMNavigationDashboardEntity(),
    state: .prepare,
    presentationStep: .hidden
  )

  var startButtonIsEnabled: Bool {
    routePoints.start != nil && routePoints.finish != nil && !showActivityIndicator
  }
  var startButtonIsHidden: Bool {
    routerType == .ruler || presentationStep == .hidden
  }
  var showActivityIndicator: Bool { progress < 1 }
}
