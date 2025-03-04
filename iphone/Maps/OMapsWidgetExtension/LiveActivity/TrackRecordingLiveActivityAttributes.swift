import ActivityKit
import AppIntents

struct TrackRecordingLiveActivityAttributes: ActivityAttributes {
  struct ContentState: Codable, Hashable {
    struct ValueViewModel: Codable, Hashable {
      let value: String
    }

    struct DetailViewModel: Codable, Hashable {
      let key: String
      let value: String
      let imageName: String
    }

    let duration: ValueViewModel
    let distance: ValueViewModel
    let ascent: DetailViewModel
    let descent: DetailViewModel
    let maxElevation: DetailViewModel
    let minElevation: DetailViewModel
  }
}
