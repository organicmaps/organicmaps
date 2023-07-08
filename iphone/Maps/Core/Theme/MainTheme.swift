class MainTheme: Theme {
  override func register() {
    self.registerStyleSheet(GlobalStyleSheet.self)
    self.registerStyleSheet(FontStyleSheet.self)
    self.registerStyleSheet(SearchStyleSheet.self)
    self.registerStyleSheet(BookmarksStyleSheet.self)
    self.registerStyleSheet(MapStyleSheet.self)
    self.registerStyleSheet(AuthStyleSheet.self)
    self.registerStyleSheet(PlacePageStyleSheet.self)
    self.registerStyleSheet(PartnersStyleSheet.self)
  }
}

