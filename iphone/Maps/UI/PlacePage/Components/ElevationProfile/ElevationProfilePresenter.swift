protocol ElevationProfilePresenterProtocol: UICollectionViewDataSource, UICollectionViewDelegateFlowLayout {
  func configure()
  func onAppear()
  func onDissapear()

  func onDifficultyButtonPressed()
  func onDragBegin()
  func onZoomBegin()
  func onNavigateBegin()
}

protocol ElevationProfileViewControllerDelegate: AnyObject {
  func openDifficultyPopup()
}

fileprivate struct DescriptionsViewModel {
  let title: String
  let value: String
  let imageName: String
}

class ElevationProfilePresenter: NSObject {
  private weak var view: ElevationProfileViewProtocol?
  private let data: ElevationProfileData
  private let delegate: ElevationProfileViewControllerDelegate?


  private let cellSpacing: CGFloat = 8
  private let descriptionModels: [DescriptionsViewModel]

  init(view: ElevationProfileViewProtocol,
       data: ElevationProfileData,
       delegate: ElevationProfileViewControllerDelegate?) {
    self.view = view
    self.data = data
    self.delegate = delegate

    descriptionModels = [
      DescriptionsViewModel(title: L("elevation_profile_ascent"), value: data.ascent, imageName: "ic_em_ascent_24"),
      DescriptionsViewModel(title: L("elevation_profile_descent"), value: data.descent, imageName: "ic_em_descent_24"),
      DescriptionsViewModel(title: L("elevation_profile_maxaltitude"), value: data.maxAttitude, imageName: "ic_em_max_attitude_24"),
      DescriptionsViewModel(title: L("elevation_profile_minaltitude"), value: data.minAttitude, imageName: "ic_em_min_attitude_24")
    ]
  }
}

extension ElevationProfilePresenter: ElevationProfilePresenterProtocol {
  func configure() {
    view?.setDifficulty(data.difficulty)
    view?.setTrackTime(data.trackTime)
    if let extendedDifficultyGrade = data.extendedDifficultyGrade {
      view?.isExtendedDifficultyLabelHidden = false
      view?.setExtendedDifficultyGrade(extendedDifficultyGrade)
    } else {
      view?.isExtendedDifficultyLabelHidden = true
    }
  }

  func onAppear() {
    Statistics.logEvent(kStatElevationProfilePageOpen,
                        withParameters: [kStatServerId: data.serverId,
                                         kStatMethod: "info|track",
                                         kStatState: "preview"])
  }

  func onDissapear() {
    Statistics.logEvent(kStatElevationProfilePageClose,
                        withParameters: [kStatServerId: data.serverId,
                                         kStatMethod: "swipe"])
  }

  func onDifficultyButtonPressed() {
    delegate?.openDifficultyPopup()
  }

  func onDragBegin() {
    Statistics.logEvent(kStatElevationProfilePageDrag,
                        withParameters: [kStatServerId: data.serverId,
                                         kStatAction: "zoom_in|zoom_out|drag",
                                         kStatSide: "left|right|all"])
  }


  func onZoomBegin() {
    Statistics.logEvent(kStatElevationProfilePageZoom,
                        withParameters: [kStatServerId: data.serverId,
                                         kStatIsZoomIn: true])
  }

  func onNavigateBegin() {
    Statistics.logEvent(kStatElevationProfilePageNavigationAction,
                        withParameters: [kStatServerId: data.serverId])
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
    cell.configure(title: model.title, value: model.value, imageName: model.imageName)
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
