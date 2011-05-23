#include "search_processor.hpp"

#include "../indexer/feature.hpp"
#include "../indexer/classificator.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

namespace search
{
  int Score(string const & key, string const & word)
  {
    size_t const offset = word.find(key);

    if (offset == 0) // best match - from the beginning
    {
      if (word.size() == key.size())
        return 1000; // full match
      else
        return 100;  // partial match
    }
    else if (offset == string::npos) // no match
    {
      return -1;
    }
    else    // match in the middle of the string
      return key.size() * 2; // how many symbols matched
  }

  Query::Query(string const & line)
  {
    //utf8_string::Split(line, m_tokens);
  }

  bool Query::operator()(char lang, string const & utf8s)
  {
    vector<string> words;
    //utf8_string::Split(utf8s, words);
    int score = -1;
    for (size_t i = 0; i < m_tokens.size(); ++i)
    {
      for (size_t j = 0; j < words.size(); ++j)
      {
        score += Score(m_tokens[i], words[j]);
      }
    }
    if (score > 0)
    {
      m_queue.push(make_pair(score, Result(utf8s, m_currFeature->GetLimitRect(-1), 0)));
      return false;
    }
    return true;
  }

  void Query::Match(FeatureType const & f)
  {
    m_currFeature = &f;
    f.ForEachNameRef(*this);
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////

  Processor::Processor(Query & query)
  : m_query(query)
  {
  }

  bool Processor::operator() (FeatureType const & f) const
  {
    // filter out features without any name
    if (f.HasName())
      m_query.Match(f);
    return true;
  }

}
