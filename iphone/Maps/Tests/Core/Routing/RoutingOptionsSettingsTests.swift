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
    XCTAssertEqual(RoutingOption.avoidanceOptions.filter { $0.isSupported(in: bike) }, [.ferryCrossings, .steps])
    XCTAssertEqual(RoutingOption.avoidanceOptions.filter { $0.isSupported(in: walk) }, [.ferryCrossings, .steps, .difficultTrails])
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

  func test_CyclingSurfaceSelectionComesFromState() {
    let bike = RoutingOptions(routerType: .bicycle)
    let presenter = RoutingOptionsSettingsPresenter(viewController: RoutingOptionsSettingsViewController())
    let preferences: [BicycleSurfacePreference] = [.any, .preferPaved, .preferUnpaved, .avoidUnpaved]
    for preference in preferences {
      bike.avoidDirty = preference == .avoidUnpaved
      let state = RoutingOptionsSettingsState(options: bike,
                                              canChangeOptimization: true,
                                              bicycleSurface: preference,
                                              transitModes: .all,
                                              transitPreference: .recommended)
      let sections = presenter.sections(from: state)
      XCTAssertFalse(sections.flatMap(\.items).contains { $0.item == .unpavedRoads })
      let selected = sections.first { $0.section == .bicycleSurface }?.items
        .filter { $0.kind == .selectable(isSelected: true) }
      XCTAssertEqual(selected?.map(\.item), [.bicycleSurface(preference)])
    }
  }

  func test_CyclingSurfaceSelectionClearsHardAvoid() {
    let savedBike = RoutingOptions(routerType: .bicycle)
    let savedSurface = RoutingOptions.bicycleSurfacePreference
    defer {
      RoutingOptions.bicycleSurfacePreference = savedSurface
      savedBike.save()
    }

    let bike = RoutingOptions(routerType: .bicycle)
    bike.avoidDirty = true
    bike.save()

    let interactor = RoutingOptionsSettingsInteractor(routerType: .bicycle)
    interactor.handle(.didLoad)
    interactor.handle(.didSelect(.bicycleSurface(.preferUnpaved)))
    XCTAssertEqual(RoutingOptions.bicycleSurfacePreference, .preferUnpaved)
    XCTAssertFalse(RoutingOptions(routerType: .bicycle).avoidDirty)
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

  func test_TransitKeepsOneModeAndGlobalOptimization() {
    let saved = RoutingOptions.transitModes
    defer { RoutingOptions.transitModes = saved }
    RoutingOptions.transitModes = .all
    XCTAssertTrue(MWMRouterType.publicTransport.hasRoutingSettings)
    XCTAssertFalse(MWMRouterType.ruler.hasRoutingSettings)
    let presenter = RoutingOptionsSettingsPresenter(viewController: RoutingOptionsSettingsViewController())
    let state = RoutingOptionsSettingsState(options: RoutingOptions(routerType: .publicTransport),
                                            canChangeOptimization: true,
                                            bicycleSurface: .any,
                                            transitModes: .subway,
                                            transitPreference: .recommended)
    let sections = presenter.sections(from: state)
    XCTAssertEqual(sections.map(\.section), [.transitPreference, .transitModes, .optimization])
    XCTAssertEqual(sections.first { $0.section == .transitModes }?.items.first { $0.item == .transitMode(.subway) }?.kind,
                   .switcher(isOn: true, isEnabled: false))
    XCTAssertEqual(sections.last?.header, L("routing_options_all_modes"))
    XCTAssertEqual(sections.last?.items.map(\.item), [.routeOptimization])

    RoutingOptions.transitModes = .subway
    let interactor = RoutingOptionsSettingsInteractor(routerType: .publicTransport)
    interactor.handle(.didLoad)
    interactor.handle(.didChangeSwitch(.transitMode(.bus), isOn: true))
    interactor.handle(.didChangeSwitch(.transitMode(.subway), isOn: false))
    XCTAssertEqual(RoutingOptions.transitModes, .bus)
  }

  func test_TransitPreferenceSelectionComesFromState() {
    let options = RoutingOptions(routerType: .publicTransport)
    let presenter = RoutingOptionsSettingsPresenter(viewController: RoutingOptionsSettingsViewController())
    for preference in [TransitPreference.recommended, .lessWalking, .fewerTransfers] {
      let state = RoutingOptionsSettingsState(options: options,
                                              canChangeOptimization: true,
                                              bicycleSurface: .any,
                                              transitModes: .all,
                                              transitPreference: preference)
      let selected = presenter.sections(from: state).first { $0.section == .transitPreference }?.items
        .filter { $0.kind == .selectable(isSelected: true) }
      XCTAssertEqual(selected?.map(\.item), [.transitPreference(preference)])
    }
  }

  func test_TransitPreferenceSelectionPersists() {
    let saved = RoutingOptions.transitPreference
    defer { RoutingOptions.transitPreference = saved }
    let interactor = RoutingOptionsSettingsInteractor(routerType: .publicTransport)
    interactor.handle(.didLoad)
    interactor.handle(.didSelect(.transitPreference(.lessWalking)))
    XCTAssertEqual(RoutingOptions.transitPreference, .lessWalking)
    interactor.handle(.didSelect(.transitPreference(.fewerTransfers)))
    XCTAssertEqual(RoutingOptions.transitPreference, .fewerTransfers)
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
      let state = RoutingOptionsSettingsState(options: option,
                                              canChangeOptimization: true,
                                              bicycleSurface: .any,
                                              transitModes: .all,
                                              transitPreference: .recommended)
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
