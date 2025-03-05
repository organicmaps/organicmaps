import SwiftUI

struct StatisticValueView: View {
  let viewModel: TrackRecordingLiveActivityAttributes.ContentState.ValueViewModel

  var body: some View {
    Text(viewModel.value)
      .contentTransition(.numericText())
      .minimumScaleFactor(0.1)
      .font(.title3.bold().monospacedDigit())
      .foregroundStyle(.white)
  }
}
