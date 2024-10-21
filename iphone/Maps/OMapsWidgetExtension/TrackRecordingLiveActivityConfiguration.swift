import WidgetKit
import SwiftUI

struct TrackRecordingLiveActivityConfiguration: Widget {
  var body: some WidgetConfiguration {
    ActivityConfiguration(for: TrackRecordingActivityAttributes.self) { context in
      TrackRecordingLiveActivityView(state: context.state, stopRecordingDeepLink: context.attributes.stopRecordingDeepLink)
    } dynamicIsland: { context in
      DynamicIsland {
        DynamicIslandExpandedRegion(.center) {

        }
      } compactLeading: {

      } compactTrailing: {

      } minimal: {

      }
    }
  }
}


