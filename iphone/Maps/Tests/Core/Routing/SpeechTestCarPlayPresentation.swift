import CarPlay
@testable import Organic_Maps__Debug_

@objc(MWMSpeechTestCarPlayPresentation)
final class SpeechTestCarPlayPresentation: NSObject, CarPlayRouterListener {
  private let router = CarPlayRouter()
  @objc private(set) var showsNextTurn = false
  @objc private(set) var finished = false

  @objc init(active: Bool, hasTrip: Bool) {
    super.init()
    if hasTrip {
      let item = MKMapItem.forCurrentLocation()
      let choice = CPRouteChoice(summaryVariants: ["Route"], additionalInformationVariants: [], selectionSummaryVariants: [])
      router.previewTrip = CPTrip(origin: item, destination: item, routeChoices: [choice])
    }
    router.setRoutingPresentationActive(active)
    router.addListener(self)
    router.subscribeToEvents()
  }

  @objc static func whileCarOwnsRouting(_ update: () -> Void) {
    let getter = class_getInstanceMethod(CarPlayService.self, #selector(getter: CarPlayService.isCarPlayRoutingActive))!
    let carOwnsRouting: @convention(block) (AnyObject) -> Bool = { _ in true }
    let original = method_setImplementation(getter, imp_implementationWithBlock(carOwnsRouting))
    defer { imp_removeBlock(method_setImplementation(getter, original)) }
    update()
  }

  deinit {
    router.unsubscribeFromEvents()
  }

  func didCreateRoute(routeInfo _: RouteInfo, trip _: CPTrip) {}
  func didFailureBuildRoute(forTrip _: CPTrip, code _: RouterResultCode, countries _: [String]) {}

  func didUpdateRouteInfo(_ routeInfo: RouteInfo, forTrip _: CPTrip) {
    showsNextTurn = routeInfo.nextTurnDirection == .left
  }

  func routeDidFinish(_: CPTrip) {
    finished = true
    router.finishTrip()
  }
}
