import Chart
import CoreApi

protocol TrackActivePointPresenter: AnyObject {
  func updateActivePointDistance(_ distance: Double)
  func updateMyPositionDistance(_ distance: Double)
}

protocol ElevationProfilePresenterProtocol:
  UICollectionViewDataSource,
  UICollectionViewDelegateFlowLayout,
  TrackActivePointPresenter {
  func configure()
  func update(with state: ElevationProfileState)
  func onSelectedPointChanged(_ distance: Double)
}

protocol ElevationProfileViewControllerDelegate: AnyObject {
  func updateMapPoint(distance: Double)
}

enum ElevationProfileState {
  case track(ElevationProfileDisplayData)
  case trackRecording(ElevationProfileDisplayData)
  case routePreview(ElevationProfileDisplayData)
}

struct ElevationProfileDisplayData {
  let trackInfo: TrackInfo
  let chartData: ElevationProfileChartData
  let activePointDistance: Double
  let myPositionDistance: Double
}

private struct DescriptionsViewModel {
  let title: String
  let value: String
  let imageName: String
}

final class ElevationProfilePresenter: NSObject {
  private enum Constants {
    static let placeholderText = L("elevation_profile_placeholder")
  }

  private weak var view: ElevationProfileViewProtocol?
  private var state: ElevationProfileState
  private weak var delegate: ElevationProfileViewControllerDelegate?

  private let cellSpacing: CGFloat = 8
  private var descriptionModels: [DescriptionsViewModel]
  private let formatter: ElevationProfileFormatter

  init(view: ElevationProfileViewProtocol,
       state: ElevationProfileState,
       formatter: ElevationProfileFormatter = ElevationProfileFormatter(),
       delegate: ElevationProfileViewControllerDelegate?) {
    self.view = view
    self.delegate = delegate
    self.formatter = formatter
    self.state = state
    descriptionModels = []
    super.init()
    apply(state: state, configureView: false)
  }

  private static func descriptionModels(for trackInfo: TrackInfo) -> [DescriptionsViewModel] {
    [
      DescriptionsViewModel(title: L("elevation_profile_ascent"),
                            value: trackInfo.ascent,
                            imageName: "ic_em_ascent_24"),
      DescriptionsViewModel(title: L("elevation_profile_descent"),
                            value: trackInfo.descent,
                            imageName: "ic_em_descent_24"),
      DescriptionsViewModel(title: L("elevation_profile_max_elevation"),
                            value: trackInfo.maxElevation,
                            imageName: "ic_em_max_attitude_24"),
      DescriptionsViewModel(title: L("elevation_profile_min_elevation"),
                            value: trackInfo.minElevation,
                            imageName: "ic_em_min_attitude_24"),
    ]
  }
}

extension ElevationProfilePresenter: ElevationProfilePresenterProtocol {
  func update(with state: ElevationProfileState) {
    apply(state: state, configureView: true)
  }

  private func apply(state: ElevationProfileState, configureView: Bool) {
    self.state = state
    switch state {
    case .track(let data), .trackRecording(let data), .routePreview(let data):
      descriptionModels = Self.descriptionModels(for: data.trackInfo)
    }
    if configureView {
      configure()
    }
  }

  func updateActivePointDistance(_ distance: Double) {
    guard let view, view.canReceiveUpdates else { return }
    view.setActivePointDistance(distance)
  }

  func updateMyPositionDistance(_ distance: Double) {
    guard let view, view.canReceiveUpdates else { return }
    view.setMyPositionDistance(distance)
  }

  func configure() {
    view?.isChartViewHidden = false

    switch state {
    case .track(let data):
      configureTrack(data)
    case .trackRecording(let data):
      configureTrackRecording(data)
    case .routePreview(let data):
      configureRoutePreview(data)
    }
  }

  private func configureTrack(_ data: ElevationProfileDisplayData) {
    view?.setChartData(ChartPresentationData(data.chartData, formatter: formatter))
    view?.reloadDescription()
    view?.isXAxisViewHidden = false
    view?.placeholderText = nil
    view?.userInteractionEnabled = true
    view?.isChartViewInfoHidden = false
    view?.setActivePointDistance(data.activePointDistance)
    view?.setMyPositionDistance(data.myPositionDistance)
  }

  private func configureTrackRecording(_ data: ElevationProfileDisplayData) {
    view?.setChartData(ChartPresentationData(data.chartData, formatter: formatter))
    view?.reloadDescription()
    view?.isXAxisViewHidden = data.chartData.isPlaceholder
    view?.placeholderText = data.chartData.isPlaceholder ? Constants.placeholderText : nil
    view?.userInteractionEnabled = false
    view?.isChartViewInfoHidden = true
  }

  private func configureRoutePreview(_ data: ElevationProfileDisplayData) {
    view?.setChartData(ChartPresentationData(data.chartData, formatter: formatter))
    view?.reloadDescription()
    view?.isXAxisViewHidden = false
    view?.placeholderText = nil
    view?.userInteractionEnabled = false
    view?.isChartViewInfoHidden = true
    view?.setActivePointDistance(data.activePointDistance)
    view?.setMyPositionDistance(data.myPositionDistance)
  }

  func onSelectedPointChanged(_ distance: Double) {
    delegate?.updateMapPoint(distance: distance)
  }
}

// MARK: - UICollectionDataSource

extension ElevationProfilePresenter {
  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    descriptionModels.count
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(cell: ElevationProfileDescriptionCell.self, indexPath: indexPath)
    let model = descriptionModels[indexPath.row]
    cell.configure(subtitle: model.title, value: model.value, imageName: model.imageName)
    return cell
  }
}

// MARK: - UICollectionViewDelegateFlowLayout

extension ElevationProfilePresenter {
  func collectionView(_ collectionView: UICollectionView,
                      layout _: UICollectionViewLayout,
                      sizeForItemAt _: IndexPath) -> CGSize {
    let width = collectionView.width
    let cellHeight = collectionView.height
    let modelsCount = CGFloat(descriptionModels.count)
    let horizontalSpacing = cellSpacing * (modelsCount - 1)
    let horizontalInsets = collectionView.contentInset.right + collectionView.contentInset.left
    let cellWidth = (width - horizontalSpacing - horizontalInsets) / modelsCount
    return CGSize(width: cellWidth, height: cellHeight)
  }

  func collectionView(_: UICollectionView,
                      layout _: UICollectionViewLayout,
                      minimumInteritemSpacingForSectionAt _: Int) -> CGFloat {
    cellSpacing
  }
}

struct ElevationProfileChartData {
  struct Line: ChartLine {
    var values: [ChartValue]
    var color: UIColor
    var type: ChartLineType
  }

  let chartValues: [ChartValue]
  let chartLines: [Line]
  let distances: [Double]
  let segmentBoundaryDistances: [Double]
  let maxDistance: Double
  let pointsCount: Int
  let isPlaceholder: Bool

  static func placeholder(altitude: Double) -> ElevationProfileChartData {
    // Keep the placeholder invisible while giving the chart a non-degenerate Y range.
    let values = [
      ChartValue(xValues: 0, y: CGFloat(altitude)),
      ChartValue(xValues: 1, y: CGFloat(altitude + 1)),
    ]
    return ElevationProfileChartData(chartValues: values,
                                     chartLines: [Line(values: values, color: .clear, type: .line)],
                                     distances: [0, 1],
                                     segmentBoundaryDistances: [],
                                     isPlaceholder: true)
  }

  init(_ elevationData: ElevationProfileData) {
    let points = elevationData.points
    let chartValues = points.map { ChartValue(xValues: $0.distance, y: $0.altitude) }
    let lineColor = UIColor.chartLine
    let lineShadowColor = UIColor.chartShadow
    let l1 = Line(values: chartValues, color: lineColor, type: .line)
    let l2 = Line(values: chartValues, color: lineShadowColor, type: .lineArea)
    self.init(chartValues: chartValues,
              chartLines: [l1, l2],
              distances: points.map(\.distance),
              segmentBoundaryDistances: elevationData.segmentDistances.map(\.doubleValue),
              isPlaceholder: false)
  }

  private init(chartValues: [ChartValue],
               chartLines: [Line],
               distances: [Double],
               segmentBoundaryDistances: [Double],
               isPlaceholder: Bool) {
    self.chartValues = chartValues
    self.chartLines = chartLines
    self.distances = distances
    self.segmentBoundaryDistances = segmentBoundaryDistances
    self.isPlaceholder = isPlaceholder
    maxDistance = distances.last ?? 0
    pointsCount = chartValues.count
  }
}

extension ElevationProfileChartData: ChartData {
  var xAxisValues: [Double] { distances }
  var lines: [ChartLine] { chartLines }
  var segmentDistances: [Double] { segmentBoundaryDistances.filter { $0 > 0 && $0 < maxDistance } }
  var type: ChartType { .regular }
}
