final class TrackStatisticsBuilder {
  static func build(statistics: TrackStatistics) -> TrackStatisticsViewController {
    let viewModel = TrackStatisticsViewModel(statistics: statistics)
    let viewController = TrackStatisticsViewController(viewModel: viewModel)
    return viewController
  }
}
