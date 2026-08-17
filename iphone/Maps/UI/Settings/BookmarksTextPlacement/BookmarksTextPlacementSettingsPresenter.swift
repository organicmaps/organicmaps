final class BookmarksTextPlacementSettingsPresenter {
  private weak var viewController: BookmarksTextPlacementSettingsViewController?

  init(viewController: BookmarksTextPlacementSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: BookmarksTextPlacementSettingsState) {
    let sections = sections(from: state)
    viewController?.display(SettingsViewModel(title: RootSettings.bookmarksTextPlacement.title,
                                              sections: sections,
                                              animatingDifferences: false))
  }

  private func sections(from state: BookmarksTextPlacementSettingsState)
    -> [BookmarksTextPlacementSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .options,
                               header: nil,
                               footer: BookmarksTextPlacementSettingsSection.options.footer,
                               items: Placement.allCases.map { item($0, state: state) }),
    ]
  }

  private func item(_ placement: Placement,
                    state: BookmarksTextPlacementSettingsState) -> BookmarksTextPlacementSettingsItemViewModel {
    SettingsItemViewModel(item: placement,
                          title: placement.title,
                          kind: .selectable(isSelected: placement == state.placement))
  }
}
