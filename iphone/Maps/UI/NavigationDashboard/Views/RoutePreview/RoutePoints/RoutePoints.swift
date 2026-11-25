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
    struct TrackPreview: Equatable {
      let name: String
      let destination: String
      let canReverse: Bool
    }

    private(set) var track: TrackPreview?
    var planningMode: PlanningMode { track == nil ? .default : .navigateTrack }

    init(points: [MWMRoutePoint]) {
      start = points.first { $0.type == .start }
      finish = points.first { $0.type == .finish }
      intermediate = points.filter { $0.type == .intermediate }
      self.points = points
    }

    init(track info: MWMFollowedTrackInfo) {
      self.init(points: [])
      let destination = if info.endsAtGap {
        info.isReversed ? L("navigate_track_section_start") : L("navigate_track_section_finish")
      } else {
        info.isReversed ? L("navigate_track_start") : L("navigate_track_finish")
      }
      track = TrackPreview(name: info.name.isEmpty ? L("navigate_track_entry") : info.name,
                           destination: destination,
                           canReverse: info.canReverse)
    }
  }
}

extension NavigationDashboard.RoutePoints {
  static let empty = NavigationDashboard.RoutePoints(points: [])

  var count: Int { track != nil ? 3 : 2 + intermediate.count }
  var hasStartAndFinish: Bool { track != nil || (start != nil && finish != nil) }

  subscript(index: Int) -> MWMRoutePoint? {
    guard track == nil else { return nil }
    switch index {
    case 0: return start
    case count - 1: return finish
    default: return intermediate[index - 1]
    }
  }

  func title(for index: Int) -> String {
    if let track {
      switch index {
      case 0: return L("p2p_your_location")
      case 1: return track.name
      default: return track.destination
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
    planningMode == .default && !points.isEmpty && (0 ..< count).contains(index)
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
