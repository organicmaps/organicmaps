import SwiftUI
import WidgetKit

#if canImport(ActivityKit)

struct TrackRecordingLiveActivityView: View {
  let state: TrackRecordingActivityAttributes.ContentState
  let stopRecordingDeepLink: URL

  var body: some View {
    HStack(alignment: .center, spacing: 16) {
      Image(.logo)
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(width: 28, height: 28)
        .padding([.leading], 6)
      Text("Track Recording")
        .font(.title3.bold())
      Spacer()
    }
    .padding([.top, .leading, .trailing], 16)
    HStack(alignment: .center, spacing: 20) {
      VStack(alignment: .leading) {
        Text(state.recordingTime)
          .font(.title3.bold().monospacedDigit())
        Text("Time")
          .font(.footnote)
      }
      VStack(alignment: .leading) {
        Text(state.trackLength)
          .font(.title3.bold().monospacedDigit())
        Text("Distance")
          .font(.footnote)
      }
      .font(.title2)
      Spacer()
      HStack(alignment: .center, spacing: 20) {
//        Button(action: {
//          // Logic to pause the activity will go here
//        }) {
//          Image(systemName: "pause.fill")
//            .resizable()
//            .frame(width: 20, height: 20)
//            .foregroundColor(.white)
//        }
//        .frame(width: 44, height: 44)
//        .background(.blue)
//        .clipShape(.circle)
        Link(destination: stopRecordingDeepLink) {
          Image(systemName: "stop.fill")
            .resizable()
            .frame(width: 20, height: 20)
            .foregroundColor(.white)
        }
        .frame(width: 44, height: 44)
        .background(.red)
        .clipShape(.circle)
      }
    }
    .padding([.leading, .trailing, .bottom], 16)
  }
}

#endif

struct TrackRecordingLiveActivityWidget_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      let activityAttributes = TrackRecordingActivityAttributes(stopRecordingDeepLink: URL(string: "om://track-recording?action=stop")!)
      let activityState = TrackRecordingActivityAttributes.ContentState(recordingTime: "10:25", trackLength: "25.5 m")

      activityAttributes
        .previewContext(activityState, viewKind: .content)
        .previewDisplayName("Notification")
    }
  }
}
