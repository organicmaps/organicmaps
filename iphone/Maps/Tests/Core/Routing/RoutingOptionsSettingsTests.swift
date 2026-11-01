@testable import Organic_Maps__Debug_
import XCTest

final class RoutingOptionsSettingsTests: XCTestCase {
  override func setUpWithError() throws {
    try super.setUpWithError()
    let controller = try XCTUnwrap(MapViewController.shared())
    controller.loadViewIfNeeded()
  }

  func testTrackOptimizationHintIsScopedToTrackPlanning() {
    let presenter = RoutingOptionsSettingsPresenter(viewController: RoutingOptionsSettingsViewController())
    let ordinary = RoutingOptionsSettingsState(options: RoutingOptions(routerType: .bicycle), canChangeOptimization: true)
    var track = ordinary
    track.isTrackNavigation = true
    XCTAssertEqual(presenter.sections(from: ordinary).last?.footer, L("route_optimization_description"))
    XCTAssertEqual(presenter.sections(from: track).last?.footer, L("navigate_track_optimization_description"))
  }

  func testTrackOptionsOnlyRebuildExplicitlyForOptimizationChanges() {
    let options = RoutingOptions(routerType: .bicycle)
    let previous = options.routeOptimizationEnabled
    let saved = RoutingOptions(routerType: .bicycle)
    defer {
      saved.save()
      options.routeOptimizationEnabled = previous
      MockTrackRouter.rebuildCount = 0
    }
    MockTrackRouter.rebuildCount = 0
    let interactor = RoutingOptionsSettingsInteractor(routerType: .bicycle, router: MockTrackRouter.self)
    interactor.handle(.didLoad)
    interactor.handle(.didChangeSwitch(.ferryCrossings, isOn: !options.avoidFerry))
    interactor.handle(.didDisappear)
    XCTAssertEqual(MockTrackRouter.rebuildCount, 0, "updateRoute handles avoidance changes")
    interactor.handle(.didChangeSwitch(.routeOptimization, isOn: !previous))
    interactor.handle(.didDisappear)
    XCTAssertEqual(MockTrackRouter.rebuildCount, 1)
    interactor.handle(.didDisappear)
    XCTAssertEqual(MockTrackRouter.rebuildCount, 1)
    interactor.handle(.didChangeSwitch(.routeOptimization, isOn: previous))
    interactor.handle(.didDisappear)
    XCTAssertEqual(MockTrackRouter.rebuildCount, 2, "Turning optimization off also restores the endpoint")
  }

  func testTrackPlannerHidesActionsAndRestoresOrdinaryLayoutAcrossSizesAndThemes() throws {
    let controller = NavigationDashboardViewController()
    controller.loadViewIfNeeded()
    var model = NavigationDashboard.ViewModel.initial
    model.dashboardState = .ready
    model.progress = 1
    for size in [CGSize(width: 375, height: 812), CGSize(width: 1024, height: 1366)] {
      for appearance in [UIUserInterfaceStyle.light, .dark] {
        controller.overrideUserInterfaceStyle = appearance
        controller.view.frame = CGRect(origin: .zero, size: size)
        for mode in [NavigationDashboard.PlanningMode.navigateTrack, .default] {
          model.routePoints.planningMode = mode
          controller.render(model)
          controller.view.layoutIfNeeded()
          let start = try XCTUnwrap(Mirror(reflecting: controller).children.first { $0.label == "startRouteButton" }?.value as? StartRouteButton)
          XCTAssertEqual(start.bounds.height, 44, accuracy: 0.5)
          for name in ["searchButton", "bookmarksButton", "saveRouteAsTrackButton"] {
            let button = try XCTUnwrap(Mirror(reflecting: controller).children.first { $0.label == name }?.value as? UIButton)
            XCTAssertEqual(button.isHidden, mode == .navigateTrack)
          }
        }
      }
    }
  }

  func test_RoutingOptionsPersistIndependentlyAndFilterUnsupportedControls() {
    let saved = [MWMRouterType.vehicle, .bicycle, .pedestrian].map { RoutingOptions(routerType: $0) }
    defer { saved.forEach { $0.save() } }
    let car = RoutingOptions(routerType: .vehicle)
    let bike = RoutingOptions(routerType: .bicycle)
    let walk = RoutingOptions(routerType: .pedestrian)
    car.avoidFerry = false
    car.avoidToll = true
    car.save()
    bike.avoidFerry = true
    bike.avoidDirty = true
    bike.save()
    walk.avoidFerry = false
    walk.save()

    XCTAssertTrue(RoutingOptions(routerType: .vehicle).avoidToll)
    XCTAssertFalse(RoutingOptions(routerType: .vehicle).avoidFerry)
    XCTAssertTrue(RoutingOptions(routerType: .bicycle).avoidFerry)
    XCTAssertFalse(RoutingOptions(routerType: .bicycle).avoidDirty)
    XCTAssertFalse(RoutingOptions(routerType: .pedestrian).avoidFerry)
    XCTAssertEqual(RoutingOption.avoidanceOptions.filter { $0.isSupported(in: bike) }, [.ferryCrossings])
    XCTAssertEqual(RoutingOption.avoidanceOptions.filter { $0.isSupported(in: walk) }, [.ferryCrossings])
    XCTAssertTrue(RoutingOptions(routerType: .publicTransport).supportedOptions.isEmpty)
    XCTAssertTrue(RoutingOptions(routerType: .ruler).supportedOptions.isEmpty)
  }

  func test_RoutingOptionsEqualityIncludesProfile() {
    let bike = RoutingOptions(routerType: .bicycle)
    let walk = RoutingOptions(routerType: .pedestrian)
    bike.avoidFerry = false
    walk.avoidFerry = false
    XCTAssertNotEqual(bike, walk)
    let sameBike = RoutingOptions(routerType: .bicycle)
    sameBike.avoidFerry = false
    XCTAssertEqual(bike, sameBike)
    XCTAssertEqual(bike.hash, sameBike.hash)
  }

  func test_TransitKeepsOnlyTheGlobalOptimizationSection() {
    XCTAssertTrue(MWMRouterType.publicTransport.hasRoutingSettings)
    XCTAssertFalse(MWMRouterType.ruler.hasRoutingSettings)
    let presenter = RoutingOptionsSettingsPresenter(viewController: RoutingOptionsSettingsViewController())
    let state = RoutingOptionsSettingsState(options: RoutingOptions(routerType: .publicTransport),
                                            canChangeOptimization: true)
    let sections = presenter.sections(from: state)
    XCTAssertEqual(sections.map(\.section), [.optimization])
    XCTAssertEqual(sections.first?.header, L("routing_options_all_modes"))
    XCTAssertEqual(sections.first?.items.map(\.item), [.routeOptimization])
  }

  func test_DashboardKeepsSettingsVisibleAndDisablesRulerSettings() throws {
    let controller = NavigationDashboardViewController()
    controller.loadViewIfNeeded()
    controller.view.frame = CGRect(x: 0, y: 0, width: 390, height: 844)
    let settingsButton = try XCTUnwrap(Mirror(reflecting: controller).children
      .first { $0.label == "settingsButton" }?.value as? UIButton)
    var model = NavigationDashboard.ViewModel.initial
    model.dashboardState = .prepare
    for type in [MWMRouterType.vehicle, .pedestrian, .bicycle, .publicTransport, .ruler, .publicTransport] {
      model.routerType = type
      model.routingOptions = RoutingOptions(routerType: type)
      controller.render(model)
      XCTAssertFalse(settingsButton.isHidden, "\(type)")
      XCTAssertEqual(settingsButton.isEnabled, type != .ruler, "\(type)")
    }
  }

  func test_OptimizationIsSharedByTheProfilesAndMarkedAsGlobal() {
    let profiles: [MWMRouterType] = [.vehicle, .bicycle, .pedestrian, .publicTransport]
    let options = profiles.map { RoutingOptions(routerType: $0) }
    let previous = options[0].routeOptimizationEnabled
    defer { options[0].routeOptimizationEnabled = previous }
    options[1].routeOptimizationEnabled = !previous
    for option in options {
      XCTAssertEqual(option.routeOptimizationEnabled, !previous)
    }
    let presenter = RoutingOptionsSettingsPresenter(viewController: RoutingOptionsSettingsViewController())
    for option in options {
      let state = RoutingOptionsSettingsState(options: option, canChangeOptimization: true)
      let sections = presenter.sections(from: state)
      XCTAssertEqual(sections.last?.section, .optimization)
      XCTAssertEqual(sections.last?.header, L("routing_options_all_modes"))
    }
  }

  func test_SettingsScreenKeepsItsProfileWhenTheRouteModeChanges() {
    let previousType = MWMRouter.type()
    let savedBike = RoutingOptions(routerType: .bicycle)
    let savedWalk = RoutingOptions(routerType: .pedestrian)
    defer {
      savedBike.save()
      savedWalk.save()
      MWMRouter.setType(previousType)
    }
    let walk = RoutingOptions(routerType: .pedestrian)
    walk.avoidFerry = false
    walk.save()
    MWMRouter.setType(.vehicle)
    let interactor = RoutingOptionsSettingsInteractor(routerType: .bicycle)
    interactor.handle(.didLoad)
    MWMRouter.setType(.pedestrian)
    interactor.handle(.didChangeSwitch(.ferryCrossings, isOn: true))
    XCTAssertTrue(RoutingOptions(routerType: .bicycle).avoidFerry)
    XCTAssertFalse(RoutingOptions(routerType: .pedestrian).avoidFerry)
  }

  private final class MockTrackRouter: MWMRouter {
    static var rebuildCount = 0
    override class func isTrackRoute() -> Bool { true }
    override class func isOnRoute() -> Bool { false }
    override class func isRoutingActive() -> Bool { true }
    override class func type() -> MWMRouterType { .bicycle }
    override class func rebuild(withBestRouter _: Bool) { rebuildCount += 1 }
  }
}
