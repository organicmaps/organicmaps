enum SettingsItemKind: Equatable {
  case link
  case selectable(isSelected: Bool)
  case switcher(isOn: Bool, isEnabled: Bool)
  case iCloudSwitcher(isOn: Bool, isAvailable: Bool, errorDescription: String?)
  case textField(text: String, placeholder: String?, isEnabled: Bool, isValid: Bool)
  case message(text: String)
  case slider(value: Float, minimumValue: Float, maximumValue: Float, valueTitle: String, isEnabled: Bool)
}

struct SettingsItemViewModel<Item: Hashable>: Equatable {
  let item: Item
  let title: String?
  let detail: String?
  let kind: SettingsItemKind

  init(item: Item, title: String? = nil, detail: String? = nil, kind: SettingsItemKind) {
    self.item = item
    self.title = title
    self.detail = detail
    self.kind = kind
  }
}

struct SettingsSectionViewModel<Section: Hashable, Item: Hashable>: Equatable {
  let section: Section
  let header: String?
  let footer: String?
  var items: [SettingsItemViewModel<Item>]

  init(section: Section, header: String? = nil, footer: String? = nil, items: [SettingsItemViewModel<Item>]) {
    self.section = section
    self.header = header
    self.footer = footer
    self.items = items
  }
}

struct SettingsViewModel<Section: Hashable, Item: Hashable>: Equatable {
  let title: String
  let sections: [SettingsSectionViewModel<Section, Item>]
  let reconfiguredItems: [Item]
  let animatingDifferences: Bool

  init(title: String,
       sections: [SettingsSectionViewModel<Section, Item>],
       reconfiguredItems: [Item] = [],
       animatingDifferences: Bool = true) {
    self.title = title
    self.sections = sections
    self.reconfiguredItems = reconfiguredItems
    self.animatingDifferences = animatingDifferences
  }
}
