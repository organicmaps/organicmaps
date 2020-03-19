protocol ElevationDetailsPresenterProtocol: class {
  func configure()
  func onOkButtonPressed()
}

class ElevationDetailsPresenter {
  private weak var view: ElevationDetailsViewProtocol?
  private let router: ElevationDetailsRouterProtocol
  private let data: ElevationProfileData

  init(view: ElevationDetailsViewProtocol,
       router: ElevationDetailsRouterProtocol,
       data: ElevationProfileData) {
    self.view = view
    self.router = router
    self.data = data
  }
}

extension ElevationDetailsPresenter: ElevationDetailsPresenterProtocol {
  func configure() {
    view?.setDifficulty(data.difficulty)
//    view?.setExtendedDifficultyGrade(data.extendedDifficultyGrade ?? "")
//    view?.setDifficultyDescription(data.extendedDifficultyDescription ?? "")

    Statistics.logEvent(kStatElevationProfilePageDetailsOpen, withParameters: [kStatType: data.difficulty.rawValue]);
  }

  func onOkButtonPressed() {
    router.close()
  }
}
