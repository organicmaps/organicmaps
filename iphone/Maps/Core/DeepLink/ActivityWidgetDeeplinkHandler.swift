enum ActivityWidgetDeeplinkSource: String, CaseIterable {
  case liveActivityWidget = "live-activity-widget"
}

struct ActivityWidgetDeeplinkParser {

  private enum Feature {
    case trackRecording(TrackRecordingAction)
  }

  static func parse(_ url: URL) -> Bool {
    LOG(.debug, "Parsing activity widged deeplink")
    guard let feature = parseFeature(from: url) else {
      return false
    }
    LOG(.debug, "Handling feature: \(feature)")
    handle(feature)
    return true
  }

  private static func parseFeature(from url: URL) -> Feature? {
    guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false),
          let host = components.host,
          ActivityWidgetDeeplinkSource(rawValue: host) != nil else {
      return nil
    }
    if let trackRecordingAction = TrackRecordingAction.parseFromDeeplink(url.absoluteString) {
      return .trackRecording(trackRecordingAction)
    }
    return nil
  }

  private static func handle(_ feature: Feature) {
    switch feature {
    case .trackRecording(let action):
      TrackRecordingManager.shared.processAction(action)
    }
  }
}

extension TrackRecordingAction {

  static private let kTrackRecordingDeeplinkPath = "track-recording"

  func buildDeeplink(for source: ActivityWidgetDeeplinkSource) -> URL {
    return URL(string: "om://" + source.rawValue + "/" + Self.kTrackRecordingDeeplinkPath + "/" + self.rawValue)!
  }

  static func parseFromDeeplink(_ deeplink: String) -> TrackRecordingAction? {
    let components = deeplink.components(separatedBy: "/")
    if components.contains(kTrackRecordingDeeplinkPath) {
      return TrackRecordingAction(rawValue: components.last ?? "")
    }
    return nil
  }
}
