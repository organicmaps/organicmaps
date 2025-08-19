import Chart
import CoreApi

protocol TrackActivePointPresenter: AnyObject {
  func updateActivePointDistance(_ distance: Double)
  func updateMyPositionDistance(_ distance: Double)
}

protocol ElevationProfilePresenterProtocol: UICollectionViewDataSource, UICollectionViewDelegateFlowLayout, TrackActivePointPresenter {
  func configure()
  func update(with trackData: PlacePageTrackData)
  func onDifficultyButtonPressed()
  func onSelectedPointChanged(_ point: CGFloat)
}

protocol ElevationProfileViewControllerDelegate: AnyObject {
  func openDifficultyPopup()
  func updateMapPoint(_ point: CLLocationCoordinate2D, distance: Double)
}

fileprivate struct DescriptionsViewModel {
  let title: String
  let value: String
  let imageName: String
}

final class ElevationProfilePresenter: NSObject {
  private weak var view: ElevationProfileViewProtocol?
  private weak var trackData: PlacePageTrackData?
  private weak var delegate: ElevationProfileViewControllerDelegate?
  private let bookmarkManager: BookmarksManager = .shared()

  private let cellSpacing: CGFloat = 8
  private var descriptionModels: [DescriptionsViewModel]
  private var chartData: ElevationProfileChartData?
  private let formatter: ElevationProfileFormatter

  init(view: ElevationProfileViewProtocol,
       trackData: PlacePageTrackData,
       formatter: ElevationProfileFormatter = ElevationProfileFormatter(),
       delegate: ElevationProfileViewControllerDelegate?) {
    self.view = view
    self.delegate = delegate
    self.formatter = formatter
    self.trackData = trackData
    if let profileData = trackData.elevationProfileData {
      self.chartData = ElevationProfileChartData(profileData)
    }
    self.descriptionModels = Self.descriptionModels(for: trackData.trackInfo)
  }

  private static func descriptionModels(for trackInfo: TrackInfo) -> [DescriptionsViewModel] {
    [
      DescriptionsViewModel(title: L("elevation_profile_ascent"), value: trackInfo.ascent, imageName: "ic_em_ascent_24"),
      DescriptionsViewModel(title: L("elevation_profile_descent"), value: trackInfo.descent, imageName: "ic_em_descent_24"),
      DescriptionsViewModel(title: L("elevation_profile_max_elevation"), value: trackInfo.maxElevation, imageName: "ic_em_max_attitude_24"),
      DescriptionsViewModel(title: L("elevation_profile_min_elevation"), value: trackInfo.minElevation, imageName: "ic_em_min_attitude_24")
    ]
  }
}

extension ElevationProfilePresenter: ElevationProfilePresenterProtocol {
  func update(with trackData: PlacePageTrackData) {
    self.trackData = trackData
    if let profileData = trackData.elevationProfileData {
      self.chartData = ElevationProfileChartData(profileData)
    } else {
      self.chartData = nil
    }
    descriptionModels = Self.descriptionModels(for: trackData.trackInfo)
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
    guard let trackData = trackData,
          let profileData = trackData.elevationProfileData,
          let chartData,
          chartData.points.count >= kMinPointsToDraw else {
      view?.userInteractionEnabled = false
      return
    }

    view?.setChartData(ChartPresentationData(chartData, formatter: formatter))
    view?.reloadDescription()

    guard !profileData.isTrackRecording else {
      view?.isChartViewInfoHidden = true
      return
    }

    view?.userInteractionEnabled = true
    view?.setActivePointDistance(trackData.activePointDistance)
    view?.setMyPositionDistance(trackData.myPositionDistance)
  }

  func onDifficultyButtonPressed() {
    delegate?.openDifficultyPopup()
  }

  func onSelectedPointChanged(_ point: CGFloat) {
    guard let chartData else { return }
    let distance: Double = floor(point) / CGFloat(chartData.points.count) * chartData.maxDistance
    let point = chartData.points.first { $0.distance >= distance } ?? chartData.points[0]
    delegate?.updateMapPoint(point.coordinates, distance: point.distance)
  }
}

// MARK: - UICollectionDataSource

extension ElevationProfilePresenter {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return descriptionModels.count
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
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    let width = collectionView.width
    let cellHeight = collectionView.height
    let modelsCount = CGFloat(descriptionModels.count)
    let cellWidth = (width - cellSpacing * (modelsCount - 1) - collectionView.contentInset.right - collectionView.contentInset.left) / modelsCount
    return CGSize(width: cellWidth, height: cellHeight)
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
    return cellSpacing
  }
}

fileprivate struct ElevationProfileChartData {

  struct Line: ChartLine {
    var values: [ChartValue]
    var color: UIColor
    var type: ChartLineType
  }

  fileprivate let chartValues: [ChartValue]
  fileprivate let chartLines: [Line]
  fileprivate let distances: [Double]
  fileprivate let maxDistance: Double
  fileprivate let points: [ElevationHeightPoint]

  init(_ elevationData: ElevationProfileData) {
    self.points = elevationData.points
    self.chartValues = points.map { ChartValue(xValues: $0.distance, y: $0.altitude) }
    self.distances = points.map { $0.distance }
    self.maxDistance = distances.last ?? 0
    let lineColor = StyleManager.shared.theme?.colors.chartLine ?? .blue
    let lineShadowColor = StyleManager.shared.theme?.colors.chartShadow ?? .lightGray
    let l1 = Line(values: chartValues, color: lineColor, type: .line)
    let l2 = Line(values: chartValues, color: lineShadowColor, type: .lineArea)
    chartLines = [l1, l2]
  }

  private static func altBetweenPoints(_ p1: ElevationHeightPoint,
                                       _ p2: ElevationHeightPoint,
                                       at distance: Double) -> Double {
    assert(distance > p1.distance && distance < p2.distance, "distance must be between points")
    let d = (distance - p1.distance) / (p2.distance - p1.distance)
    return p1.altitude + round(Double(p2.altitude - p1.altitude) * d)
  }
}

extension ElevationProfileChartData: ChartData {
  public var xAxisValues: [Double] { distances }
  public var lines: [ChartLine] { chartLines }
  public var type: ChartType { .regular }
}
