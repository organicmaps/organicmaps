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
  func update(with trackData: PlacePageTrackData)
  func update(with previewData: RouteElevationPreviewData)
  func onSelectedPointChanged(_ distance: Double)
}

protocol ElevationProfileViewControllerDelegate: AnyObject {
  func updateMapPoint(distance: Double)
}

private struct DescriptionsViewModel {
  let title: String
  let value: String
  let imageName: String
}

final class ElevationProfilePresenter: NSObject {
  private weak var view: ElevationProfileViewProtocol?
  private var trackInfo: TrackInfo
  private var elevationProfileData: ElevationProfileData?
  private var activePointDistance: Double
  private var myPositionDistance: Double
  private weak var delegate: ElevationProfileViewControllerDelegate?

  private let cellSpacing: CGFloat = 8
  private var descriptionModels: [DescriptionsViewModel]
  private var chartData: ElevationProfileChartData?
  private let formatter: ElevationProfileFormatter

  init(view: ElevationProfileViewProtocol,
       trackInfo: TrackInfo,
       elevationProfileData: ElevationProfileData?,
       activePointDistance: Double = 0,
       myPositionDistance: Double = 0,
       formatter: ElevationProfileFormatter = ElevationProfileFormatter(),
       delegate: ElevationProfileViewControllerDelegate?) {
    self.view = view
    self.delegate = delegate
    self.formatter = formatter
    self.trackInfo = trackInfo
    self.elevationProfileData = elevationProfileData
    self.activePointDistance = activePointDistance
    self.myPositionDistance = myPositionDistance
    if let elevationProfileData {
      chartData = ElevationProfileChartData(elevationProfileData)
    }
    descriptionModels = Self.descriptionModels(for: trackInfo)
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
  func update(with trackData: PlacePageTrackData) {
    applyTrackData(trackInfo: trackData.trackInfo,
                   elevationProfileData: trackData.elevationProfileData,
                   activePointDistance: trackData.activePointDistance,
                   myPositionDistance: trackData.myPositionDistance)
  }

  func update(with previewData: RouteElevationPreviewData) {
    applyTrackData(trackInfo: previewData.trackInfo,
                   elevationProfileData: previewData.elevationProfileData,
                   activePointDistance: 0,
                   myPositionDistance: 0)
  }

  private func applyTrackData(trackInfo: TrackInfo,
                              elevationProfileData: ElevationProfileData?,
                              activePointDistance: Double,
                              myPositionDistance: Double) {
    self.trackInfo = trackInfo
    self.elevationProfileData = elevationProfileData
    self.activePointDistance = activePointDistance
    self.myPositionDistance = myPositionDistance
    if let elevationProfileData {
      chartData = ElevationProfileChartData(elevationProfileData)
    } else {
      chartData = nil
    }
    descriptionModels = Self.descriptionModels(for: trackInfo)
    configure()
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

    let kMinPointsToDraw = 2
    guard let elevationProfileData,
          let chartData,
          chartData.pointsCount >= kMinPointsToDraw
    else {
      view?.userInteractionEnabled = false
      return
    }

    view?.setChartData(ChartPresentationData(chartData, formatter: formatter))
    view?.reloadDescription()

    guard !elevationProfileData.isTrackRecording else {
      view?.userInteractionEnabled = false
      view?.isChartViewInfoHidden = true
      return
    }

    view?.userInteractionEnabled = true
    view?.setActivePointDistance(activePointDistance)
    view?.setMyPositionDistance(myPositionDistance)
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

private struct ElevationProfileChartData {
  struct Line: ChartLine {
    var values: [ChartValue]
    var color: UIColor
    var type: ChartLineType
  }

  fileprivate let chartValues: [ChartValue]
  fileprivate let chartLines: [Line]
  fileprivate let distances: [Double]
  fileprivate let segmentBoundaryDistances: [Double]
  fileprivate let maxDistance: Double
  fileprivate let pointsCount: Int

  init(_ elevationData: ElevationProfileData) {
    let points = elevationData.points
    pointsCount = points.count
    chartValues = points.map { ChartValue(xValues: $0.distance, y: $0.altitude) }
    distances = points.map(\.distance)
    segmentBoundaryDistances = elevationData.segmentDistances.map(\.doubleValue)
    maxDistance = distances.last ?? 0
    let lineColor = UIColor.chartLine
    let lineShadowColor = UIColor.chartShadow
    let l1 = Line(values: chartValues, color: lineColor, type: .line)
    let l2 = Line(values: chartValues, color: lineShadowColor, type: .lineArea)
    chartLines = [l1, l2]
  }
}

extension ElevationProfileChartData: ChartData {
  public var xAxisValues: [Double] { distances }
  public var lines: [ChartLine] { chartLines }
  public var segmentDistances: [Double] { segmentBoundaryDistances.filter { $0 > 0 && $0 < maxDistance } }
  public var type: ChartType { .regular }
}
