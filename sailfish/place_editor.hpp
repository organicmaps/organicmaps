#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <memory>

namespace osm
{
class EditableMapObject;
}  // namespace osm

namespace sailfish
{
// Kept free of map headers, which Qt 5.6 moc can't parse.
class PlaceEditor : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool valid READ valid NOTIFY changed)
  Q_PROPERTY(QString category READ category NOTIFY changed)
  Q_PROPERTY(bool nameEditable READ nameEditable NOTIFY changed)
  // The default (local) name.
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
  // {code, language, value} rows.
  Q_PROPERTY(QVariantList localizedNames READ localizedNames NOTIFY changed)
  Q_PROPERTY(bool addressEditable READ addressEditable NOTIFY changed)
  Q_PROPERTY(QString street READ street WRITE setStreet NOTIFY changed)
  Q_PROPERTY(QStringList nearbyStreets READ nearbyStreets NOTIFY changed)
  Q_PROPERTY(QString houseNumber READ houseNumber WRITE setHouseNumber NOTIFY changed)
  // {id, kind, section, icon, label, value, inputHint} rows; the icon names a file in icons/editor.
  Q_PROPERTY(QVariantList fields READ fields NOTIFY changed)
  // A ResetAction.
  Q_PROPERTY(int resetAction READ resetAction NOTIFY changed)

public:
  enum Kind
  {
    Text,
    // "yes" or "".
    Wifi,
    SelfService,
    OpeningHours,
    // ';' separated cuisines() keys.
    Cuisine,
    // ';' separated phone numbers.
    Phone,
    // "yes", "no" or "" for unknown.
    YesNo
  };
  Q_ENUM(Kind)

  enum ResetAction
  {
    // For a new place.
    NoReset,
    ResetEdits,
    RemovePlace,
    PlaceDoesntExist
  };
  Q_ENUM(ResetAction)

  enum Section
  {
    Address,
    Details,
    SocialMedia,
    Building
  };
  Q_ENUM(Section)

  explicit PlaceEditor(QObject * parent = nullptr);
  ~PlaceEditor() override;

  bool valid() const { return m_valid; }
  QString category() const;
  bool nameEditable() const;
  QString name() const;
  void setName(QString const & name);
  QVariantList localizedNames() const;
  bool addressEditable() const;
  QString street() const;
  void setStreet(QString const & street);
  QStringList nearbyStreets() const;
  QString houseNumber() const;
  void setHouseNumber(QString const & houseNumber);
  QVariantList fields() const;
  int resetAction() const;

  Q_INVOKABLE void start();
  // A PlaceCategories type; returns false when no map is loaded there.
  Q_INVOKABLE bool create(QString const & type, double lat, double lon);
  Q_INVOKABLE void setField(int id, QString const & value);
  // Empty for a valid value.
  Q_INVOKABLE QString fieldError(int id, QString const & value) const;
  Q_INVOKABLE QString nameError(QString const & name) const;
  // An empty name removes it.
  Q_INVOKABLE void setLocalizedName(int code, QString const & name);
  // {code, language} rows, without the ones already named.
  Q_INVOKABLE QVariantList otherLanguages() const;
  Q_INVOKABLE QString houseNumberError(QString const & houseNumber) const;
  // {value, name} rows.
  Q_INVOKABLE QVariantList selfServiceValues() const;
  // {key, name} rows.
  Q_INVOKABLE QVariantList cuisines() const;
  Q_INVOKABLE QString cuisineNames(QString const & value) const;
  // See FormatOpeningHours().
  Q_INVOKABLE QString openingHoursText(QString const & value) const;
  // Saves locally, false on error.
  Q_INVOKABLE bool save();
  Q_INVOKABLE void createNote(QString const & note);
  // Discards the local edits, or removes a created place.
  Q_INVOKABLE void reset();
  Q_INVOKABLE void placeDoesntExist(QString const & comment);

signals:
  void changed();

private:
  std::unique_ptr<osm::EditableMapObject> m_object;
  bool m_valid = false;
  bool m_creating = false;
};

// The category picker of a new place, without a place to edit.
class PlaceCategories : public QObject
{
  Q_OBJECT

public:
  using QObject::QObject;

  // {type, name, recent} rows: recent ones first, then all by name. A query leaves out the recent ones.
  Q_INVOKABLE QVariantList categories(QString const & query) const;
  Q_INVOKABLE void createStandaloneNote(double lat, double lon, QString const & note);
};
}  // namespace sailfish
