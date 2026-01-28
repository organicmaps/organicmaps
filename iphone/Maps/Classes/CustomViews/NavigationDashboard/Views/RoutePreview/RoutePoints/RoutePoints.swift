extension NavigationDashboard {
  struct RoutePoints: Equatable {
    let start: MWMRoutePoint?
    let finish: MWMRoutePoint?
    let intermediate: [MWMRoutePoint]
    let points: [MWMRoutePoint]

    init(points: [MWMRoutePoint]) {
      self.start = points.first { $0.type == .start }
      self.finish = points.first { $0.type == .finish }
      self.intermediate = points.filter { $0.type == .intermediate }
      self.points = points
    }
  }
}

extension NavigationDashboard.RoutePoints {
  static let empty = NavigationDashboard.RoutePoints(points: [])

  var count: Int { 2 + intermediate.count }
  var hasStartAndFinish: Bool { start != nil && finish != nil }

  subscript(index: Int) -> MWMRoutePoint? {
    switch index {
    case 0: return start
    case count - 1: return finish
    default: return intermediate[index - 1]
    }
  }

  func index(of point: MWMRoutePoint) -> Int? {
    switch point {
    case start: return 0
    case finish: return count - 1
    default: return intermediate.firstIndex(of: point)
    }
  }

  func title(for index: Int) -> String {
    switch index {
    case 0:
      return start?.title ?? L("p2p_from_here")
    case count - 1:
      return finish?.title ?? L("p2p_to_here")
    default:
      return intermediate[index - 1].title
    }
  }

  func subtitle(for index: Int) -> String? {
    self[index]?.subtitle
  }

  func image(for index: Int) -> UIImage {
    switch index {
    case 0:
      return UIImage(resource: .icRouteManagerStart)
    case count - 1:
      return UIImage(resource: .icRouteManagerFinish)
    default:
      let imageName = "route-point-\(index)"
      return UIImage(named: imageName) ?? UIImage(resource: .routePoint20)
    }
  }

  mutating func movePoint(from sourceIndex: Int, to destinationIndex: Int) {
    guard sourceIndex != destinationIndex else { return }
    var points = self.points
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
