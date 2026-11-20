extension NavigationDashboard {
  enum PlanningMode {
    case `default`, navigateTrack

    var transportOptions: [MWMRouterType] {
      switch self {
      case .default: return MWMRouterType.allCases
      case .navigateTrack: return [.vehicle, .pedestrian, .bicycle]
      }
    }
  }

  struct RoutePoints: Equatable {
    let start: MWMRoutePoint?
    let finish: MWMRoutePoint?
    let intermediate: [MWMRoutePoint]
    let points: [MWMRoutePoint]
    var planningMode: PlanningMode { trackName == nil ? .default : .navigateTrack }
    private var trackName: String?

    init(points: [MWMRoutePoint]) {
      start = points.first { $0.type == .start }
      finish = points.first { $0.type == .finish }
      intermediate = points.filter { $0.type == .intermediate }
      self.points = points
    }

    init(trackName: String) {
      self.init(points: [])
      self.trackName = trackName
    }
  }
}

extension NavigationDashboard.RoutePoints {
  static let empty = NavigationDashboard.RoutePoints(points: [])

  var count: Int { trackName != nil ? 3 : 2 + intermediate.count }
  var hasStartAndFinish: Bool { trackName != nil || (start != nil && finish != nil) }

  subscript(index: Int) -> MWMRoutePoint? {
    guard trackName == nil else { return nil }
    switch index {
    case 0: return start
    case count - 1: return finish
    default: return intermediate[index - 1]
    }
  }

  func title(for index: Int) -> String {
    if let trackName {
      switch index {
      case 0: return L("p2p_your_location")
      case 1: return trackName.isEmpty ? L("navigate_track_start") : trackName
      default: return L("navigate_track_finish")
      }
    }
    switch index {
    case 0:
      return start?.title ?? L("p2p_from_here")
    case count - 1:
      return finish?.title ?? L("p2p_to_here")
    default:
      return intermediate[index - 1].title
    }
  }

  func defaultImage(for index: Int) -> UIImage {
    switch index {
    case 0:
      return UIImage(resource: .icRouteManagerStart)
    case count - 1:
      return UIImage.routePointFinish
    default:
      let imageName = "route-point-\(index)"
      return UIImage(named: imageName) ?? UIImage(resource: .routePoint20)
    }
  }

  func type(for index: Int) -> MWMRoutePointType {
    switch index {
    case 0: return .start
    case count - 1: return .finish
    default: return .intermediate
    }
  }

  func canMovePoint(at index: Int) -> Bool {
    planningMode == .default && (0 ..< count).contains(index)
  }

  func canMovePoint(from sourceIndex: Int, to destinationIndex: Int) -> Bool {
    sourceIndex != destinationIndex && canMovePoint(at: sourceIndex) && canMovePoint(at: destinationIndex)
  }

  mutating func movePoint(from sourceIndex: Int, to destinationIndex: Int) {
    guard canMovePoint(from: sourceIndex, to: destinationIndex) else { return }
    var points = points
    if points.count == 1, let point = points.first {
      point.type = point.type == .start ? .finish : .start
    } else {
      let pointToMove = points.remove(at: sourceIndex)
      points.insert(pointToMove, at: destinationIndex)
      for (index, point) in points.enumerated() {
        switch index {
        case 0:
          point.type = .start
        case points.count - 1:
          point.type = .finish
        default:
          point.type = .intermediate
        }
      }
    }
    self = NavigationDashboard.RoutePoints(points: points)
  }
}
