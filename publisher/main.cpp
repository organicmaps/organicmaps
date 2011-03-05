#include "aard_dictionary.hpp"
#include "slof_indexer.hpp"
#include "../coding_sloynik/bzip2_compressor.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"
#include "../base/base.hpp"
#include "../base/assert.hpp"
#include "../base/logging.hpp"
#include "../std/bind.hpp"
#include "../3party/gflags/src/gflags/gflags.h"

DEFINE_int32(max_uncompressed_article_chunk_size, 899950,
             "Max size of chunk of articles, uncompressed.");
DEFINE_int32(compression_level, 9, "BZip2 compression level.");
DEFINE_string(input, "", "Input file.");
DEFINE_string(output, "", "Output dictionary file.");

int main(int argc, char ** argv)
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  CHECK(!FLAGS_input.empty(), ());
  CHECK(!FLAGS_output.empty(), ());
  FileReader inputReader(FLAGS_input.c_str());
  FileWriter outputWriter(FLAGS_output.c_str());
  {
    sl::AardDictionary inputDictionary(inputReader);
    sl::SlofIndexer indexer(outputWriter,
                            FLAGS_max_uncompressed_article_chunk_size,
                            bind(&CompressBZip2, FLAGS_compression_level, _1, _2, _3));
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
  LOG(LINFO, ("Indexing done."));
  LOG(LINFO, ("Input size:", inputReader.Size()));
  LOG(LINFO, ("Output size:", outputWriter.Pos()));
}
