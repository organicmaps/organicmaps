import Chart

protocol ElevationProfilePresenterProtocol: UICollectionViewDataSource, UICollectionViewDelegateFlowLayout {
  func configure()
  func onAppear()
  func onDissapear()

  func onDifficultyButtonPressed()
  func onDragBegin()
  func onZoomBegin()
  func onNavigateBegin()
  func onSelectedPointChanged(_ point: CGFloat)
}

protocol ElevationProfileViewControllerDelegate: AnyObject {
  func openDifficultyPopup()
  func updateMapPoint(_ distance: Double)
}

fileprivate struct DescriptionsViewModel {
  let title: String
  let value: UInt
  let imageName: String
}

class ElevationProfilePresenter: NSObject {
  private weak var view: ElevationProfileViewProtocol?
  private let data: ElevationProfileData
  private let delegate: ElevationProfileViewControllerDelegate?

  private let cellSpacing: CGFloat = 8
  private let descriptionModels: [DescriptionsViewModel]
  private let chartData: ElevationProfileChartData
  private let formatter: ChartFormatter

  init(view: ElevationProfileViewProtocol,
       data: ElevationProfileData,
       imperialUnits: Bool,
       delegate: ElevationProfileViewControllerDelegate?) {
    self.view = view
    self.data = data
    self.delegate = delegate
    chartData = ElevationProfileChartData(data)
    formatter = ChartFormatter(imperial: imperialUnits)

    descriptionModels = [
      DescriptionsViewModel(title: L("elevation_profile_ascent"), value: data.ascent, imageName: "ic_em_ascent_24"),
      DescriptionsViewModel(title: L("elevation_profile_descent"), value: data.descent, imageName: "ic_em_descent_24"),
      DescriptionsViewModel(title: L("elevation_profile_maxaltitude"), value: data.maxAttitude, imageName: "ic_em_max_attitude_24"),
      DescriptionsViewModel(title: L("elevation_profile_minaltitude"), value: data.minAttitude, imageName: "ic_em_min_attitude_24")
    ]
  }

  deinit {
    MWMBookmarksManager.shared().resetElevationActivePointChanged()
    MWMBookmarksManager.shared().resetElevationMyPositionChanged()
  }
}

extension ElevationProfilePresenter: ElevationProfilePresenterProtocol {
  func configure() {
    view?.setDifficulty(data.difficulty)
    view?.setTrackTime("\(data.trackTime)")
    let presentationData = ChartPresentationData(chartData,
                                                 formatter: formatter,
                                                 useFilter: true)
    view?.setChartData(presentationData)
    view?.setActivePoint(data.activePoint)
    view?.setMyPosition(data.myPosition)
//    if let extendedDifficultyGrade = data.extendedDifficultyGrade {
//      view?.isExtendedDifficultyLabelHidden = false
//      view?.setExtendedDifficultyGrade(extendedDifficultyGrade)
//    } else {
      view?.isExtendedDifficultyLabelHidden = true
//    }

    MWMBookmarksManager.shared().setElevationActivePointChanged(data.trackId) { [weak self] distance in
      self?.view?.setActivePoint(distance)
    }
    MWMBookmarksManager.shared().setElevationMyPositionChanged(data.trackId) { [weak self] distance in
      self?.view?.setMyPosition(distance)
    }
  }

  func onAppear() {
    Statistics.logEvent(kStatElevationProfilePageOpen,
                        withParameters: [/*kStatServerId: data.serverId,*/ //TODO: clarify
                                         kStatMethod: "info|track",
                                         kStatState: "preview"])
  }

  func onDissapear() {
    Statistics.logEvent(kStatElevationProfilePageClose,
                        withParameters: [/*kStatServerId: data.serverId,*/ //TODO: clarify
                                         kStatMethod: "swipe"])
  }

  func onDifficultyButtonPressed() {
    delegate?.openDifficultyPopup()
  }

  func onDragBegin() {
    Statistics.logEvent(kStatElevationProfilePageDrag,
                        withParameters: [/*kStatServerId: data.serverId,*/ //TODO: clarify
                                         kStatAction: "zoom_in|zoom_out|drag",
                                         kStatSide: "left|right|all"])
  }


  func onZoomBegin() {
    Statistics.logEvent(kStatElevationProfilePageZoom,
                        withParameters: [/*kStatServerId: data.serverId,*/ //TODO: clarify
                                         kStatIsZoomIn: true])
  }

  func onNavigateBegin() {
    Statistics.logEvent(kStatElevationProfilePageNavigationAction,
                        withParameters: [/*kStatServerId: data.serverId,*/ //TODO: clarify
                          :])
  }

  func onSelectedPointChanged(_ point: CGFloat) {
    let x1 = Int(floor(point))
    let x2 = Int(ceil(point))
    let d1: Double = chartData.points[x1].distance
    let d2: Double = chartData.points[x2].distance
    let dx = Double(point.truncatingRemainder(dividingBy: 1))
    let distance = d1 + (d2 - d1) * dx
    delegate?.updateMapPoint(distance)
  }
}

// MARK: - UICollectionDataSource

extension ElevationProfilePresenter {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return descriptionModels.count
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "ElevationProfileDescriptionCell", for: indexPath) as! ElevationProfileDescriptionCell
    let model = descriptionModels[indexPath.row]
    cell.configure(title: model.title, value: formatter.altitudeString(from: Double(model.value)), imageName: model.imageName)
    return cell
  }
}

// MARK: - UICollectionViewDelegateFlowLayout

extension ElevationProfilePresenter {
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    let width = collectionView.width
    let cellHeight = collectionView.height
    let modelsCount = CGFloat(descriptionModels.count)
    let cellWidth = (width - cellSpacing * (modelsCount - 1)) / modelsCount
    return CGSize(width: cellWidth, height: cellHeight)
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
    return cellSpacing
  }
}

fileprivate struct ElevationProfileChartData {
  struct Line: IChartLine {
    var values: [Int]
    var name: String
    var color: UIColor
    var type: ChartLineType
  }

  fileprivate let chartLines: [Line]
  fileprivate let distances: [Double]
  fileprivate let points: [ElevationHeightPoint]

  init(_ elevationData: ElevationProfileData) {
    points = ElevationProfileChartData.rearrangePoints(elevationData.points)
    let values = points.map { Int($0.altitude) }
//    let formatter = MKDistanceFormatter()
//    formatter.unitStyle = .abbreviated
//    formatter.units = .metric
//    labels = points.map { formatter.string(fromDistance: $0.distance )}
    distances = points.map { $0.distance }
    let color = UIColor(red: 0.12, green: 0.59, blue: 0.94, alpha: 1)
    let l1 = Line(values: values, name: "Altitude", color: color, type: .line)
    let l2 = Line(values: values, name: "Altitude", color: color.withAlphaComponent(0.12), type: .lineArea)
    chartLines = [l1, l2]
  }

  private static func rearrangePoints(_ points: [ElevationHeightPoint]) -> [ElevationHeightPoint] {
    if points.isEmpty {
      return []
    }

    var result: [ElevationHeightPoint] = []

    let distance = points.last?.distance ?? 0
    let step = floor(distance / Double(points.count))
    result.append(points[0])
    var currentDistance = step
    var i = 1
    while i < points.count {
      let prevPoint = points[i - 1]
      let nextPoint = points[i]
      if currentDistance > nextPoint.distance {
        i += 1
        continue
      }
      result.append(ElevationHeightPoint(distance: currentDistance,
                                         andAltitude: altBetweenPoints(prevPoint, nextPoint, at: currentDistance)))
      currentDistance += step
      if currentDistance > nextPoint.distance {
        i += 1
      }
    }

    return result
  }

  private static func altBetweenPoints(_ p1: ElevationHeightPoint,
                                       _ p2: ElevationHeightPoint,
                                       at distance: Double) -> Double {
    assert(distance > p1.distance && distance < p2.distance, "distance must be between points")

    let d = (distance - p1.distance) / (p2.distance - p1.distance)
    return p1.altitude + round(Double(p2.altitude - p1.altitude) * d)
  }

}

extension ElevationProfileChartData: IChartData {
  public var xAxisValues: [Double] {
    distances
  }

  public var lines: [IChartLine] {
    chartLines
  }

  public var type: ChartType {
    .regular
  }
}

fileprivate struct ChartFormatter: IFormatter {
  private let distanceFormatter: MKDistanceFormatter
  private let altFormatter: MeasurementFormatter
  private let imperial: Bool

  init(imperial: Bool) {
    self.imperial = imperial

    distanceFormatter = MKDistanceFormatter()
    distanceFormatter.units = imperial ? .imperial : .metric
    distanceFormatter.unitStyle = .abbreviated

    altFormatter = MeasurementFormatter()
    altFormatter.unitOptions = [.providedUnit]
  }

  func distanceString(from value: Double) -> String {
    distanceFormatter.string(fromDistance: value)
  }

  func altitudeString(from value: Double) -> String {
    let alt = imperial ? value / 0.3048 : value
    let measurement = Measurement(value: alt.rounded(), unit: imperial ? UnitLength.feet : UnitLength.meters)
    return altFormatter.string(from: measurement)
  }
}
