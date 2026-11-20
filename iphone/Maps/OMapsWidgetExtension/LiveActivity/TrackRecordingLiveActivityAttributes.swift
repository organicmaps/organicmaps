import ActivityKit
import AppIntents

struct TrackRecordingLiveActivityAttributes: ActivityAttributes {
  struct ContentState: Codable, Hashable {
    struct StatisticsViewModel: Codable, Hashable {
      let key: String
      let value: String
    }

    struct ElevationStats {
      let ascent: StatisticsViewModel
      let descent: StatisticsViewModel
      let maxElevation: StatisticsViewModel
      let minElevation: StatisticsViewModel
    }

    let duration: StatisticsViewModel
    let distance: StatisticsViewModel

    // Keep the flat coding keys used by earlier Live Activity payloads.
    private let ascent: StatisticsViewModel?
    private let descent: StatisticsViewModel?
    private let maxElevation: StatisticsViewModel?
    private let minElevation: StatisticsViewModel?

    var elevation: ElevationStats? {
      guard let ascent, let descent, let maxElevation, let minElevation else { return nil }
      return ElevationStats(ascent: ascent, descent: descent, maxElevation: maxElevation, minElevation: minElevation)
    }

    init(duration: StatisticsViewModel, distance: StatisticsViewModel, elevation: ElevationStats?) {
      self.duration = duration
      self.distance = distance
      ascent = elevation?.ascent
      descent = elevation?.descent
      maxElevation = elevation?.maxElevation
      minElevation = elevation?.minElevation
    }
  }
}
