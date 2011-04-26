#include "../words/slof_dictionary.hpp"
#include "../coding/file_reader.hpp"
#include "../base/logging.hpp"
#include "../std/iostream.hpp"
#include "../std/scoped_ptr.hpp"
#include "../3party/gflags/src/gflags/gflags.h"

DEFINE_string(dictionary, "", "Path to the dictionary.");
DEFINE_string(lookup, "", "Word to lookup.");

int main(int argc, char * argv [])
{
  google::ParseCommandLineFlags(&argc, &argv, false);

  scoped_ptr<sl::SlofDictionary> m_pDic;
  if (!FLAGS_dictionary.empty())
  {
    m_pDic.reset(new sl::SlofDictionary(new FileReader(FLAGS_dictionary)));
    cout << "Keys in the dictionary: " << m_pDic->KeyCount() << endl;
  }

  if (!FLAGS_lookup.empty())
  {
    CHECK(m_pDic.get(), ("Dictionary should not be empty."));
    for (sl::Dictionary::Id i = 0; i < m_pDic->KeyCount(); ++i)
    {
      string key;
      m_pDic->KeyById(i, key);
      if (key == FLAGS_lookup)
      {
        cout << "===== Found id " << i << " =====" << endl;
        string article;
        m_pDic->ArticleById(i, article);
        cout << article << endl;
        cout << "================================" << endl;
      }
    }
  }
}
