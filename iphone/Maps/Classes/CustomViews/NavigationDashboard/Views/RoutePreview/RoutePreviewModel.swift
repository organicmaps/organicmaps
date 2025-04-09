enum RoutePreview {
  struct ViewModel: Equatable {
    let transportOptions: [MWMRouterType] = MWMRouterType.allCases
    var points: RoutePoints = .empty
    var routerType: MWMRouterType
    var entity: MWMNavigationDashboardEntity
    var estimates: NSAttributedString = NSAttributedString()
    var state: MWMNavigationDashboardState
    var presentationStep: ModalPresentationStep
    var shouldClose: Bool = false
    var progress: CGFloat = 0
  }

  struct RoutePoints: Equatable {
    let start: MWMRoutePoint?
    let finish: MWMRoutePoint?
    let intermediate: [MWMRoutePoint]
  }

  enum Request {
    case prepareRoute
    case startRoutePlanning
    case routeIsReady
    case selectRouterType(MWMRouterType)
    case selectRoutePoint(MWMRoutePoint?, at: Int)
    case addRoutePoint
    case deleteRoutePoint(MWMRoutePoint)
    case moveRoutePoint(from: Int, to: Int)
    case startNavigation
    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
    case updateDrivingOptionState(MWMDrivingOptionsState)
    case updateNavigationInfo(MWMNavigationDashboardEntity)
    case updatePresentationFrame(CGRect)
//    case updatePresentationStep(ModalPresentationStep)
    case setHidden(Bool)
    case close
  }

  enum Response: Equatable {
    case none
    case show(points: [MWMRoutePoint], routerType: MWMRouterType)
    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
    case updateNavigationInfo(MWMNavigationDashboardEntity)
    case updatePresentationStep(ModalPresentationStep)
    case showNavigationDashboard
    case setHidden(Bool)
    case close
  }
}

extension RoutePreview.ViewModel {
  static let initial = RoutePreview.ViewModel(
    routerType: .vehicle,
    entity: MWMNavigationDashboardEntity(),
    state: .prepare,
    presentationStep: .hidden
  )

  var startButtonIsEnabled: Bool { points.start != nil && points.finish != nil && !showActivityIndicator }
  var startButtonIsHidden: Bool { routerType == .ruler }
  var showActivityIndicator: Bool { progress < 1 }
}

extension RoutePreview.RoutePoints {
  static let empty = RoutePreview.RoutePoints(start: nil, finish: nil, intermediate: [])

  var count: Int { 2 + intermediate.count }

  init(points: [MWMRoutePoint]) {
    self.start = points.first { $0.type == .start }
    self.finish = points.first { $0.type == .finish }
    self.intermediate = points.filter { $0.type == .intermediate }
  }

  subscript(index: Int) -> (point: MWMRoutePoint?, type: MWMRoutePointType) {
    switch index {
    case 0:
      return (start, .start)
    case count - 1:
      return (finish, .finish)
    default:
      return (intermediate[index - 1], .intermediate)
    }
  }

  func title(for index: Int) -> String {
    switch index {
    case 0:
      return start?.title ?? L("from")
    case count - 1:
      return finish?.title ?? L("to")
    default:
      return intermediate[index - 1].title
    }
  }
}

extension MWMRouterType: CaseIterable {
  public static var allCases: [MWMRouterType] = [
    .vehicle,
    .pedestrian,
    .publicTransport,
    .bicycle,
    .ruler
  ]

  func image(for isSelected: Bool) -> UIImage {
    switch self {
    case .vehicle:
      UIImage(resource: isSelected ? .icCarSelected : .icCar)
    case .pedestrian:
      UIImage(resource: isSelected ? .icPedestrianSelected : .icPedestrian)
    case .publicTransport:
      UIImage(resource: isSelected ? .icTrainSelected : .icTrain)
    case .bicycle:
      UIImage(resource: isSelected ? .icBikeSelected : .icBike)
    case .ruler:
      UIImage(resource: isSelected ? .icRulerRouteSelected : .icRulerRoute)
    @unknown default:
      fatalError("Unknown router type")
    }
  }
}
