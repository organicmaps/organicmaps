struct TrackStatisticsViewModel {
  private var statistics: TrackStatistics

  enum Section: Int, CaseIterable {
    case statistics
  }

  struct SectionModel {
    var cells: [CellModel]
  }

  struct CellModel {
    var text: String
    var detailText: String
  }

  private static let distanceFormatter: MKDistanceFormatter = {
    let formatter = MKDistanceFormatter()
    formatter.units = Settings.measurementUnits() == .imperial ? .imperial : .metric
    formatter.unitStyle = .abbreviated
    return formatter
  }()

  private(set) var data: [SectionModel] = []

  init(statistics: TrackStatistics) {
    self.statistics = statistics
    self.data = Self.buildData(from: statistics)
  }

  private static func buildData(from statistics: TrackStatistics) -> [SectionModel] {
    let length = distanceFormatter.string(fromDistance: statistics.length)
    let duration = DateComponentsFormatter.etaString(from: statistics.duration)
    var rows: [CellModel] = []
    rows.append(CellModel(text: L("length"), detailText: length))
    if let duration = duration {
      // TODO: Localize string
      rows.append(CellModel(text: L("duration"), detailText: duration))
    }
    return [SectionModel(cells: rows)]
  }
}
