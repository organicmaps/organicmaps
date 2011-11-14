#include "classificator_loader.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"
#include "drules_struct.pb.h"

#include "../../platform/platform.hpp"

#include "../coding/file_reader_stream.hpp"
#include "../coding/file_reader.hpp"

#include "../base/logging.hpp"


namespace classificator
{
  void ReadCommon(ReaderType const & classificator,
                  ReaderType const & visibility,
                  ReaderType const & types)
  {
    string buffer;

    Classificator & c = classif();
    c.Clear();

    //LOG(LINFO, ("Reading classificator"));
    classificator.ReadAsString(buffer);
    c.ReadClassificator(buffer);

    //LOG(LINFO, ("Reading visibility"));
    visibility.ReadAsString(buffer);
    c.ReadVisibility(buffer);

    //LOG(LINFO, ("Reading types mapping"));
    types.ReadAsString(buffer);
    c.ReadTypesMapping(buffer);
  }

  void ReadVisibility(string const & fPath)
  {
    string buffer;
    ReaderType(new FileReader(fPath)).ReadAsString(buffer);
    classif().ReadVisibility(buffer);
  }

  void Load()
  {
    LOG(LINFO, ("Reading of classificator started"));

    Platform & p = GetPlatform();

    ReadCommon(p.GetReader("classificator.txt"),
               p.GetReader("visibility.txt"),
               p.GetReader("types.txt"));

    //LOG(LINFO, ("Reading of drawing rules"));
#ifdef USE_PROTO_STYLES
    // Load from protobuffer text file.
    string buffer;
    ReaderType(p.GetReader("drules_proto.txt")).ReadAsString(buffer);
    drule::rules().LoadFromProto(buffer);
#else
    ReaderPtrStream rulesS(p.GetReader("drawing_rules.bin"));
    drule::ReadRules(rulesS);
#endif

    LOG(LINFO, ("Reading of classificator done"));
  }
}
