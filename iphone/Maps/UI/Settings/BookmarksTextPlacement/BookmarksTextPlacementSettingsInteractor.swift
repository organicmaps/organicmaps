final class BookmarksTextPlacementSettingsInteractor {
  var presenter: BookmarksTextPlacementSettingsPresenter?

  private let settings: Settings.Type
  private var state: BookmarksTextPlacementSettingsState?

  init(settings: Settings.Type = Settings.self) {
    self.settings = settings
  }

  func loadSettings() {
    let state = BookmarksTextPlacementSettingsState(placement: settings.bookmarksTextPlacement())
    self.state = state
    presenter?.present(state)
  }

  private func select(_ placement: Placement) {
    guard var state else { return }
    state.placement = placement
    self.state = state
    settings.setBookmarksTextPlacement(placement)
    presenter?.present(state)
  }
}

extension BookmarksTextPlacementSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = BookmarksTextPlacementSettingsSection
  typealias Item = Placement

  func handle(_ action: SettingsViewControllerAction<Placement>) {
    switch action {
    case .didLoad:
      loadSettings()
    case .didSelect(let item):
      select(item)
    default:
      break
    }
  }
}
