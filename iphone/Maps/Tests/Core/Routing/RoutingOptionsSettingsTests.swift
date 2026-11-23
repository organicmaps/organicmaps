@testable import Organic_Maps__Debug_
import CarPlay
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
    car.avoidDirty = false
    car.save()
    bike.avoidFerry = true
    bike.avoidDirty = true
    bike.save()
    walk.avoidFerry = false
    walk.save()

    XCTAssertTrue(RoutingOptions(routerType: .vehicle).avoidToll)
    XCTAssertFalse(RoutingOptions(routerType: .vehicle).avoidFerry)
    XCTAssertTrue(RoutingOptions(routerType: .bicycle).avoidFerry)
    XCTAssertTrue(RoutingOptions(routerType: .bicycle).avoidDirty)
    XCTAssertFalse(RoutingOptions(routerType: .vehicle).avoidDirty)
    XCTAssertFalse(RoutingOptions(routerType: .pedestrian).avoidDirty)
    XCTAssertFalse(RoutingOptions(routerType: .pedestrian).avoidFerry)
    XCTAssertEqual(RoutingOption.avoidanceOptions.filter { $0.isSupported(in: bike) }, [.unpavedRoads, .ferryCrossings, .steps])
    XCTAssertEqual(RoutingOption.avoidanceOptions.filter { $0.isSupported(in: walk) }, [.ferryCrossings, .steps, .difficultTrails])
    XCTAssertTrue(RoutingOptions(routerType: .publicTransport).supportedOptions.isEmpty)
    XCTAssertTrue(RoutingOptions(routerType: .ruler).supportedOptions.isEmpty)
  }

  func test_RoutingOptionsMaxSpeed() {
    let car = RoutingOptions(routerType: .vehicle)
    let previousSpeed = car.maxSpeedKMpH
    defer { car.maxSpeedKMpH = previousSpeed }

    car.maxSpeedKMpH = 50.0
    XCTAssertEqual(car.maxSpeedKMpH, 50.0)
    XCTAssertTrue(car.hasOptions)

    car.maxSpeedKMpH = 0.0
    XCTAssertEqual(car.maxSpeedKMpH, 0.0)
  }

  func test_CarPlaySpeedLimitTemplateCustomOptionDisabledWhileDriving() {
    let car = RoutingOptions(routerType: .vehicle)
    let previousSpeed = car.maxSpeedKMpH
    defer { car.maxSpeedKMpH = previousSpeed }

    // When driving, the custom speed limit item must be disabled.
    let drivingTemplate = SettingsTemplateBuilder.buildSpeedLimitTemplate(isDriving: true)
    let drivingSection = drivingTemplate.sections.first
    XCTAssertNotNil(drivingSection)
    let drivingItems = drivingSection?.items as? [CPListItem] ?? []
    let drivingCustomItem = drivingItems.last
    XCTAssertNotNil(drivingCustomItem)
    XCTAssertFalse(drivingCustomItem?.isEnabled ?? true)
    XCTAssertEqual(drivingCustomItem?.detailText, L("keyboard_availability_alert"))

    // When stopped / not driving, the custom speed limit item must be enabled.
    let stoppedTemplate = SettingsTemplateBuilder.buildSpeedLimitTemplate(isDriving: false)
    let stoppedSection = stoppedTemplate.sections.first
    let stoppedItems = stoppedSection?.items as? [CPListItem] ?? []
    let stoppedCustomItem = stoppedItems.last
    XCTAssertNotNil(stoppedCustomItem)
    XCTAssertTrue(stoppedCustomItem?.isEnabled ?? false)
    XCTAssertNil(stoppedCustomItem?.detailText)

    // Verify custom sub-template contains selectable speed limit values.
    let customTemplate = SettingsTemplateBuilder.buildCustomSpeedLimitTemplate()
    let customSection = customTemplate.sections.first
    let customItems = customSection?.items as? [CPListItem] ?? []
    XCTAssertFalse(customItems.isEmpty)
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

  func test_StepsAvoidancePersistsOnlyForWalkingAndCycling() {
    let profiles: [MWMRouterType] = [.vehicle, .bicycle, .pedestrian, .publicTransport, .ruler]
    let saved = profiles.map { RoutingOptions(routerType: $0) }
    defer { saved.forEach { $0.save() } }
    for profile in [MWMRouterType.bicycle, .pedestrian] {
      let options = RoutingOptions(routerType: profile)
      options.avoidFerry = false
      options.avoidDirty = false
      options.avoidDifficultTrails = false
      options.avoidSteps = true
      options.save()
      let reloaded = RoutingOptions(routerType: profile)
      XCTAssertTrue(reloaded.avoidSteps)
      XCTAssertTrue(reloaded.hasOptions)
      XCTAssertEqual(reloaded.enabledOptionsCount, 1)
    }
    for profile in [MWMRouterType.vehicle, .publicTransport, .ruler] {
      let options = RoutingOptions(routerType: profile)
      options.avoidSteps = true
      options.save()
      XCTAssertFalse(RoutingOptions(routerType: profile).avoidSteps)
    }
  }

  func test_DifficultTrailAvoidancePersistsForWalkingOnly() {
    let profiles: [MWMRouterType] = [.vehicle, .bicycle, .pedestrian, .publicTransport, .ruler]
    let saved = profiles.map { RoutingOptions(routerType: $0) }
    defer { saved.forEach { $0.save() } }
    for profile in profiles {
      let options = RoutingOptions(routerType: profile)
      options.avoidDifficultTrails = true
      options.save()
      XCTAssertEqual(RoutingOptions(routerType: profile).avoidDifficultTrails, profile == .pedestrian)
    }
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
}
