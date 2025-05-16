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
      // TODO: localize
      return start?.title ?? L("from")
    case count - 1:
      return finish?.title ?? L("to")
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
      return UIImage(resource: .icRouteManagerMyPosition)
    case count - 1:
      return UIImage(resource: .icRouteManagerFinish)
    default:
      let imageName = "route-point-\(index)"
      return UIImage(named: imageName) ?? UIImage(resource: .routePoint20)
    }
  }

  func imageStyle(for index: Int) -> GlobalStyleSheet {
    switch index {
    case 0: return .blue
    default: return .black
    }
  }
}
