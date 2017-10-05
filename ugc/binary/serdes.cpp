#include "ugc/binary/serdes.hpp"

#include <set>

using namespace std;

namespace ugc
{
namespace binary
{
namespace
{
class BaseCollector
{
public:
  virtual ~BaseCollector() = default;

  void VisitVarUint(uint32_t, char const * /* name */ = nullptr) {}
  void VisitVarUint(uint64_t, char const * /* name */ = nullptr) {}
  virtual void VisitRating(float const f, char const * /* name */ = nullptr) {}
  virtual void operator()(string const & /* s */, char const * /* name */ = nullptr) {}
  virtual void operator()(Sentiment const /* sentiment */, char const * /* name */ = nullptr) {}
  virtual void operator()(Time const /* time */, char const * /* name */ = nullptr) {}
  virtual void operator()(TranslationKey const & tk, char const * /* name */ = nullptr) {}
  virtual void operator()(Text const & text, char const * /* name */ = nullptr) {}

  template <typename T>
  void operator()(vector<T> const & vs, char const * /* name */ = nullptr)
  {
    for (auto const & v : vs)
      (*this)(v);
  }

  template <typename R>
  typename enable_if<is_fundamental<R>::value>::type operator()(R const & r,
                                                                char const * /* name */ = nullptr)
  {
  }

  template <typename R>
  typename enable_if<!is_fundamental<R>::value>::type operator()(R const & r,
                                                                 char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }
};

// Collects all translation keys from UGC.
class TranslationKeyCollector : public BaseCollector
{
public:
  using BaseCollector::operator();

  explicit TranslationKeyCollector(set<TranslationKey> & keys) : m_keys(keys) {}

  void operator()(TranslationKey const & tk, char const * /* name */ = nullptr) override
  {
    m_keys.insert(tk.m_key);
  }

private:
  set<TranslationKey> & m_keys;
};

// Collects all texts from UGC.
class TextCollector : public BaseCollector
{
public:
  using BaseCollector::operator();

  TextCollector(vector<Text> & texts) : m_texts(texts) {}

  void operator()(Text const & text, char const * /* name */ = nullptr) override
  {
    m_texts.push_back(text);
  }

private:
  vector<Text> & m_texts;
};
}  // namespace

void UGCSeriaizer::CollectTranslationKeys()
{
  ASSERT(m_keys.empty(), ());

  set<TranslationKey> keys;
  TranslationKeyCollector collector(keys);
  for (auto const & p : m_ugcs)
    collector(p.m_ugc);
  m_keys.assign(keys.begin(), keys.end());
}

void UGCSeriaizer::CollectTexts()
{
  ASSERT(m_texts.empty(), ());
  for (auto const & p : m_ugcs)
  {
    m_texts.emplace_back();
    TextCollector collector(m_texts.back());
    collector(p.m_ugc);
  }
}
}  // namespace binary
}  // namespace ugc
