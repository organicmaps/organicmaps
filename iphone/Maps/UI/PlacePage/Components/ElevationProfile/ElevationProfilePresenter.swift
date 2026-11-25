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
  func onSelectionFinished(_ distance: Double)
  func onSelectionCancelled()
}

protocol ElevationProfileViewControllerDelegate: AnyObject {
  func updateMapPoint(distance: Double)
  func didFinishSelectingPoint(distance: Double)
  func didCancelSelectingPoint()
}

extension ElevationProfileViewControllerDelegate {
  func didFinishSelectingPoint(distance _: Double) {}
  func didCancelSelectingPoint() {}
}

enum ElevationProfileState {
  case track(ElevationProfileDisplayData)
  case trackRecording(ElevationProfileDisplayData)
  case routePreview(ElevationProfileDisplayData)
}

struct ElevationProfileDisplayData {
  let trackInfo: TrackInfo
  let chartData: ElevationProfileChartData
  var activePointDistance: Double
  let myPositionDistance: Double
}

private struct DescriptionsViewModel {
  let title: String
  let value: String
  let imageName: String
}

final class ElevationProfilePresenter: NSObject {
  private weak var view: ElevationProfileViewProtocol?
  private var state: ElevationProfileState?
  private weak var delegate: ElevationProfileViewControllerDelegate?

  private let cellSpacing: CGFloat = 8
  private var descriptionModels: [DescriptionsViewModel]
  private let formatter: ElevationProfileFormatter

  init(view: ElevationProfileViewProtocol,
       state: ElevationProfileState?,
       formatter: ElevationProfileFormatter = ElevationProfileFormatter(),
       delegate: ElevationProfileViewControllerDelegate?) {
    self.view = view
    self.delegate = delegate
    self.formatter = formatter
    self.state = state
    descriptionModels = []
    super.init()
    if let state {
      apply(state: state, configureView: false)
    }
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

  private func recordActivePointDistance(_ distance: Double) {
    if case .routePreview(var data)? = state {
      data.activePointDistance = distance
      state = .routePreview(data)
    }
  }

  func updateActivePointDistance(_ distance: Double) {
    recordActivePointDistance(distance)
    guard let view, view.canReceiveUpdates else { return }
    view.setActivePointDistance(distance)
  }

  func updateMyPositionDistance(_ distance: Double) {
    guard let view, view.canReceiveUpdates else { return }
    view.setMyPositionDistance(distance)
  }

  func configure() {
    guard let state else { return }
    view?.isChartViewHidden = false

    switch state {
    case .track(let data), .routePreview(let data):
      configureInteractiveProfile(data)
    case .trackRecording(let data):
      configureTrackRecording(data)
    }
  }

  private func configureInteractiveProfile(_ data: ElevationProfileDisplayData) {
    view?.setChartData(ChartPresentationData(data.chartData, formatter: formatter))
    view?.reloadDescription()
    view?.userInteractionEnabled = true
    view?.isChartViewInfoHidden = false
    view?.setActivePointDistance(data.activePointDistance)
    view?.setMyPositionDistance(data.myPositionDistance)
  }

  private func configureTrackRecording(_ data: ElevationProfileDisplayData) {
    view?.setChartData(ChartPresentationData(data.chartData, formatter: formatter))
    view?.reloadDescription()
    view?.userInteractionEnabled = false
    view?.isChartViewInfoHidden = true
  }

  func onSelectedPointChanged(_ distance: Double) {
    recordActivePointDistance(distance)
    delegate?.updateMapPoint(distance: distance)
  }

  func onSelectionFinished(_ distance: Double) {
    delegate?.didFinishSelectingPoint(distance: distance)
  }

  func onSelectionCancelled() {
    delegate?.didCancelSelectingPoint()
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

  var highlightedSection: (entry: Double, finish: Int)?
  let chartValues: [ChartValue]
  let chartLines: [Line]
  let distances: [Double]
  let segmentBoundaryDistances: [Double]
  let maxDistance: Double
  let pointsCount: Int

  init(_ elevationData: ElevationProfileData) {
    let points = elevationData.points
    let chartValues = points.map { ChartValue(xValues: $0.distance, y: $0.altitude) }
    self.chartValues = chartValues
    chartLines = [Line(values: chartValues, color: .chartLine, type: .line),
                  Line(values: chartValues, color: .chartShadow, type: .lineArea)]
    distances = points.map(\.distance)
    segmentBoundaryDistances = elevationData.segmentDistances.map(\.doubleValue)
    maxDistance = distances.last ?? 0
    pointsCount = chartValues.count
  }
}

extension ElevationProfileChartData: ChartData {
  var xAxisValues: [Double] { distances }
  var lines: [ChartLine] { chartLines }
  var segmentDistances: [Double] { segmentBoundaryDistances.filter { $0 > 0 && $0 < maxDistance } }
}
