#pragma once

#include "skin.hpp"

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "drape/pointers.hpp"

#include "std/function.hpp"
#include "std/unique_ptr.hpp"

class ScreenBase;

namespace gui
{

class RulerHelper;
class CountryStatusHelper;

class StorageAccessor
{
public:
  using TSlotFn = function<void ()>;

  virtual ~StorageAccessor() {}
  virtual string GetCurrentCountryName() const = 0;
  virtual size_t GetMapSize() const = 0;
  virtual size_t GetRoutingSize() const = 0;
  virtual size_t GetDownloadProgress() const  = 0;

  virtual void SetCountryIndex(storage::TIndex const & index) = 0;
  virtual storage::TIndex GetCountryIndex() const = 0;
  virtual storage::TStatus GetCountryStatus() const = 0;

  void SetStatusChangedCallback(TSlotFn const & fn);

protected:
  TSlotFn m_statusChanged;
};

class DrapeGui
{
public:
  using TRecacheSlot = function<void (Skin::ElementName)>;
  using TScaleFactorFn = function<double ()>;
  using TGeneralizationLevelFn = function<int (ScreenBase const &)>;
  using TLocalizeStringFn = function<string (string const &)>;

  static DrapeGui & Instance();
  static RulerHelper & GetRulerHelper();
  static CountryStatusHelper & GetCountryStatusHelper();

  static dp::FontDecl const & GetGuiTextFont();

  void Init(TScaleFactorFn const & scaleFn, TGeneralizationLevelFn const & gnLvlFn);
  void SetLocalizator(TLocalizeStringFn const & fn);
  void SetStorageAccessor(ref_ptr<gui::StorageAccessor> accessor);
  void Destroy();

  void SetCountryIndex(storage::TIndex const & index);

  double GetScaleFactor();
  int GetGeneralization(ScreenBase const & screen);
  string GetLocalizedString(string const & stringID) const;

  void SetRecacheSlot(TRecacheSlot const & fn);
  void EmitRecacheSignal(Skin::ElementName elements);
  void ClearRecacheSlot();

  bool IsCopyrightActive() const { return m_isCopyrightActive; }
  void DeactivateCopyright() { m_isCopyrightActive = false; }

private:
  RulerHelper & GetRulerHelperImpl();
  CountryStatusHelper & GetCountryStatusHelperImpl();

private:
  struct Impl;
  unique_ptr<Impl> m_impl;
  bool m_isCopyrightActive = true;
};

}
