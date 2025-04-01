enum RoutePreview {
  struct ViewModel: Equatable {
    let transportOptions: [MWMRouterType] = MWMRouterType.allCases
    var points: [MWMRoutePoint] = []
    var routerType: MWMRouterType
    var entity: MWMNavigationDashboardEntity
    var state: MWMNavigationDashboardState
    var presentationStep: ModalPresentationStep
    var shouldClose: Bool = false
    var isStartRoutingAllowed: Bool = true
  }

  enum Request {
    case startRouteBuilding
    case selectRouterType(MWMRouterType)
    case addRoutePoint
    case deleteRoutePoint(MWMRoutePoint)
    case moveRoutePoint(from: Int, to: Int)
    case startNavigation
    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
    case updateDrivingOptionState(MWMDrivingOptionsState)
    case setHidden(Bool)
    case close
  }

  enum Response: Equatable {
    case none
    case show(points: [MWMRoutePoint], routerType: MWMRouterType)
    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
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
