import SwiftUI

struct StatisticDetailView: View {
  let viewModel: TrackRecordingLiveActivityAttributes.ContentState.DetailViewModel

  var body: some View {
    VStack(alignment: .leading) {
      HStack {
        Image(uiImage: UIImage(named: viewModel.imageName)!)
          .resizable()
          .aspectRatio(contentMode: .fit)
          .frame(width: 16, height: 16)
          .foregroundStyle(.white)
        Text(viewModel.value)
          .contentTransition(.numericText())
          .font(.system(size: 14, weight: .bold).monospacedDigit())
          .minimumScaleFactor(0.5)
          .lineLimit(1)
          .foregroundStyle(.white)
      }
      Text(viewModel.key)
        .font(.system(size: 10))
        .minimumScaleFactor(0.5)
        .lineLimit(2)
        .multilineTextAlignment(.center)
        .foregroundStyle(.white)
    }
  }
}
