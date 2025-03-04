import SwiftUI
import WidgetKit

#if canImport(ActivityKit)

struct TrackRecordingLiveActivityView: View {
  let state: TrackRecordingLiveActivityAttributes.ContentState

  var body: some View {
    VStack(alignment: .leading, spacing: 12) {
      HStack(alignment: .top, spacing: 24) {
        StatisticValueView(viewModel: state.duration)
        StatisticValueView(viewModel: state.distance)
        Spacer()
        AppLogo()
          .frame(width: 20, height: 20)
      }
      .padding([.top, .leading, .trailing], 16)
      HStack(alignment: .top, spacing: 20) {
        StatisticDetailView(viewModel: state.ascent)
        StatisticDetailView(viewModel: state.descent)
        StatisticDetailView(viewModel: state.maxElevation)
        StatisticDetailView(viewModel: state.minElevation)
      }
      .padding([.leading, .trailing, .bottom], 16)
    }
    .activityBackgroundTint(.black.opacity(0.2))
    // Uncomment the line below to simulate the background color in Preview because the `activityBackgroundTint` can only displayed on the device or simulator.
    //.background(.black.opacity(0.85))
  }
}

#if DEBUG
struct TrackRecordingLiveActivityWidget_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      let activityAttributes = TrackRecordingLiveActivityAttributes()
      let activityState = TrackRecordingLiveActivityAttributes.ContentState(
        duration: .init(
          value: "10d 10h 12min"
        ),
        distance: .init(
          value: "1245 km"
        ),
        ascent: .init(
          key: "Ascent",
          value: "100 m",
          imageName: "ic_em_ascent_24"
        ),
        descent: .init(
          key: "Descent",
          value: "100 m",
          imageName: "ic_em_descent_24"
        ),
        maxElevation: .init(
          key: "Max Elevation",
          value: "9999 m",
          imageName: "ic_em_max_attitude_24"
        ),
        minElevation: .init(
          key: "Min Elevation",
          value: "1000 m",
          imageName: "ic_em_min_attitude_24"
        )
      )

      activityAttributes
        .previewContext(activityState, viewKind: .content)
        .previewDisplayName("Notification")
    }
  }
}
#endif

#endif
