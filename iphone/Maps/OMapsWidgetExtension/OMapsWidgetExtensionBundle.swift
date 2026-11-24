import Foundation
import SwiftUI
import WidgetKit

@main
struct OMapsWidgetExtensionBundle: WidgetBundle {
  var body: some Widget {
    // WidgetBundleBuilder only accepts `if` with an `#available` clause.
    // Skip on Mac: ActivityKit is weak-linked because macOS 13-15 lack it.
    if #available(iOS 16.2, *), !ProcessInfo.processInfo.isiOSAppOnMac {
      TrackRecordingLiveActivityConfiguration()
    }
  }
}
