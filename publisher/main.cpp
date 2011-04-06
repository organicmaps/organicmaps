#include "aard_dictionary.hpp"
#include "slof_indexer.hpp"
#include "../coding/bzip2_compressor.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"
#include "../base/base.hpp"
#include "../base/assert.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/fstream.hpp"
#include "../std/iterator.hpp"
#include "../std/string.hpp"
#include "../3party/gflags/src/gflags/gflags.h"
#include "../3party/jansson/myjansson.hpp"

DEFINE_int32(max_uncompressed_article_chunk_size, 899950,
             "Max size of chunk of articles, uncompressed.");
DEFINE_int32(compression_level, 9, "BZip2 compression level.");
DEFINE_string(input, "", "Input file.");
DEFINE_string(output, "", "Output dictionary file.");
DEFINE_string(article_file_suffix, "", "Suffix of the article files.");
DEFINE_string(redirects, "", "JSON file with redirects.");

void IndexAard(sl::SlofIndexer & indexer)
{
  FileReader inputReader(FLAGS_input.c_str());
  sl::AardDictionary inputDictionary(inputReader);

  LOG(LINFO, ("Starting indexing, keys:", inputDictionary.KeyCount()));
  for (uint32_t id = 0; id < inputDictionary.KeyCount(); ++id)
  {
    if ((id % 5000) == 0)
      LOG(LINFO, (id, "done."));
    // TODO: Handle redirects.
    // TODO: Handle several keys for article?
    string key, article;
    inputDictionary.KeyById(id, key);
    inputDictionary.ArticleById(id, article);
    if (article.empty())
    {
      LOG(LWARNING, ("Skipping empty article for:", key));
    }
    else
    {
      uint64_t const articleId = indexer.AddArticle(article);
      indexer.AddKey(key, articleId);
    }
  }
  LOG(LINFO, ("Logging stats."));
  indexer.LogStats();
  LOG(LINFO, ("Finishing indexing."));
}

void IndexJson(sl::SlofIndexer & indexer)
{
  vector<vector<string> > articles;
  LOG(LINFO, ("Reading list of articles."));
  {
    ifstream fin(FLAGS_input.c_str());
    string line;
    for (int i = 0; getline(fin, line); ++i)
    {
      if (line.empty())
        continue;

      my::Json root(line.c_str());
      CHECK_EQUAL(json_typeof(root), JSON_ARRAY, (i, line));
      CHECK_EQUAL(3, json_array_size(root), (i, line));
      articles.push_back(vector<string>());
      for (int j = 0; j < 3; ++j)
      {
        json_t * pJsonElement = json_array_get(root, j);
        CHECK(pJsonElement, (i, line));
        CHECK_EQUAL(json_typeof(pJsonElement), JSON_STRING, (i, j, line));
        articles.back().push_back(json_string_value(pJsonElement));
      }
    }
  }

  LOG(LINFO, ("Sorting list of articles."));
  sort(articles.begin(), articles.end());

  LOG(LINFO, ("Adding articles."));
  map<string, uint64_t> keysToArticles;
  for (size_t i = 0; i < articles.size(); ++i)
  {
    string const url = articles[i][0];
    string const title = articles[i][1];
    string const fileName = articles[i][2] + FLAGS_article_file_suffix;
    articles[i].clear();

    FileReader articleReader(fileName);
    string article(static_cast<size_t>(articleReader.Size()), 0);
    articleReader.Read(0, &article[0], article.size());

    uint64_t const articleId = indexer.AddArticle(article);
    indexer.AddKey(title, articleId);
    CHECK(keysToArticles.insert(make_pair(title, articleId)).second, (i));

    if ((i & 127) == 0)
      LOG(LINFO, ("Done:", i));
  }
  articles.clear();

  LOG(LINFO, ("Adding redirects."));
  map<string, string> redirects;
  {
    ifstream fin(FLAGS_redirects.c_str());
    string line;
    for (int i = 0; getline(fin, line); ++i)
    {
      if (line.empty())
        continue;

      my::Json root(line.c_str());
      CHECK_EQUAL(json_typeof(root), JSON_ARRAY, (i, line));
      CHECK_EQUAL(2, json_array_size(root), (i, line));
      string s[2];
      for (int j = 0; j < 2; ++j)
      {
        json_t * pJsonElement = json_array_get(root, j);
        CHECK(pJsonElement, (i, j, line));
        CHECK_EQUAL(json_typeof(pJsonElement), JSON_STRING, (i, line));
        s[j] = json_string_value(pJsonElement);
      }
      CHECK(redirects.insert(make_pair(s[0], s[1])).second, (s[0], s[1]));
    }
  }
  for (map<string, string>::const_iterator it = redirects.begin(); it != redirects.end(); ++it)
  {
    string const & src = it->first;
    string dst = it->second;

    if (keysToArticles.count(src))
    {
      LOG(LWARNING, ("Conflicting redirect", src, dst));
      continue;
    }

    uint64_t articleId = -1;
    for (size_t depth = 0; articleId == -1 && !dst.empty() && depth < 5; ++depth)
    {
      articleId = ValueForKey(keysToArticles, dst, uint64_t(-1));
      dst = ValueForKey(redirects, dst, string());
    }

    if (articleId == -1)
      LOG(LWARNING, ("Redirect not found", it->first, it->second));
    else
      indexer.AddKey(src, articleId);
  }
}

int main(int argc, char ** argv)
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  CHECK(!FLAGS_input.empty(), ());
  CHECK(!FLAGS_output.empty(), ());

  FileWriter outputWriter(FLAGS_output.c_str());
  {
    sl::SlofIndexer indexer(outputWriter,
                            FLAGS_max_uncompressed_article_chunk_size,
                            bind(&CompressBZip2, FLAGS_compression_level, _1, _2, _3));

    size_t const & inputSize = FLAGS_input.size();
    if (inputSize > 5 && FLAGS_input.substr(inputSize - 5) == ".aard")
      IndexAard(indexer);
    else if (inputSize > 5 && FLAGS_input.substr(inputSize - 5) == ".json")
      IndexJson(indexer);
    else
      CHECK(false, (FLAGS_input));

    LOG(LINFO, ("Finishing indexing."));
  }
  LOG(LINFO, ("Output size:", outputWriter.Pos()));
}
