@testable import Organic_Maps__Debug_
import XCTest

final class RoutingOptionsSettingsTests: XCTestCase {
  override func setUpWithError() throws {
    try super.setUpWithError()
    let controller = try XCTUnwrap(MapViewController.shared())
    controller.loadViewIfNeeded()
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
    XCTAssertTrue(RoutingOptions(routerType: .water).supportedOptions.isEmpty)
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
    XCTAssertFalse(MWMRouterType.water.hasRoutingSettings)
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
    for type in [MWMRouterType.vehicle, .pedestrian, .bicycle, .publicTransport, .ruler, .water, .publicTransport] {
      model.routerType = type
      model.routingOptions = RoutingOptions(routerType: type)
      controller.render(model)
      XCTAssertFalse(settingsButton.isHidden, "\(type)")
      XCTAssertEqual(settingsButton.isEnabled, type != .ruler && type != .water, "\(type)")
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

  func test_WaterSelectionRoundTripsThroughCoreAndNeverStartsNavigation() {
    let previousType = MWMRouter.type()
    defer { MWMRouter.setType(previousType) }
    XCTAssertTrue(MWMRouterType.allCases.contains(.water))
    for selected in [false, true] {
      XCTAssertEqual(MWMRouterType.water.image(for: selected).accessibilityLabel, L("water_routing"))
    }
    MWMRouter.setType(.water)
    XCTAssertEqual(MWMRouter.type(), .water)
    XCTAssertEqual(MWMRouter.string(from: .water), "water")
    XCTAssertFalse(MWMRouter.hasRouteAltitude())
    MWMRouter.startRouting()
    XCTAssertFalse(MWMRouter.isOnRoute())
  }

  func test_WaterSummaryHasSeparateDistancesAndConditionalQualification() throws {
    for confirmed in [false, true] {
      let entity = try XCTUnwrap(MWMNavigationDashboardEntity(waterDistance: 94000, landDistance: 24000,
                                                              shortestConfirmed: confirmed))
      let summary = try XCTUnwrap(entity.estimate()).string
      let lines = summary.components(separatedBy: "\n")
      XCTAssertEqual(lines[0], String(format: L("water_distance"), DistanceFormatter.distanceString(fromMeters: 94000)))
      XCTAssertEqual(lines[1], String(format: L("water_land_gaps"), DistanceFormatter.distanceString(fromMeters: 24000)))
      XCTAssertEqual(lines[2], String(format: L("water_total_distance"), DistanceFormatter.distanceString(fromMeters: 118_000)))
      XCTAssertEqual(lines.count, confirmed ? 3 : 4)
      XCTAssertEqual(summary.contains(L("water_shortest_unconfirmed")), !confirmed)
      XCTAssertEqual(entity.timeToTarget, 0)
      XCTAssertTrue(entity.transitSteps.isEmpty)
    }
  }

  func test_WaterProgressAndStartVisibilityResetAcrossStatesAndModes() throws {
    let controller = NavigationDashboardViewController()
    controller.loadViewIfNeeded()
    let properties = Mirror(reflecting: controller).children
    let start = try XCTUnwrap(properties.first { $0.label == "startRouteButton" }?.value as? StartRouteButton)
    let progress = try XCTUnwrap(properties.first { $0.label == "waterProgressView" }?.value as? UIProgressView)
    let spinner = try XCTUnwrap(properties.first { $0.label == "waterActivityIndicator" }?.value as? UIActivityIndicatorView)
    var model = NavigationDashboard.ViewModel.initial
    model.routerType = .water
    model.dashboardState = .planning
    controller.render(model)
    XCTAssertTrue(start.isHidden)
    XCTAssertEqual(model.startButtonState, .disabled)
    XCTAssertTrue(spinner.isAnimating)
    XCTAssertTrue(progress.isHidden)
    model.progress = 0.4
    controller.render(model)
    XCTAssertFalse(spinner.isAnimating)
    XCTAssertFalse(progress.isHidden)
    XCTAssertEqual(progress.progress, 0.4, accuracy: 0.001)
    for state in [MWMNavigationDashboardState.ready, .prepare, .error] {
      model.dashboardState = state
      controller.render(model)
      XCTAssertTrue(start.isHidden)
      XCTAssertTrue(progress.isHidden)
      XCTAssertFalse(spinner.isAnimating)
    }
    model.routerType = .vehicle
    model.dashboardState = .prepare
    controller.render(model)
    XCTAssertFalse(start.isHidden)
    XCTAssertTrue(progress.isHidden)
  }
}
