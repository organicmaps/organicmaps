#pragma once
#include "data_header.hpp"

#include "../coding/reader.hpp"


class FilesContainerR;
class IntervalIndexIFace;

class IndexFactory
{
  feature::DataHeader m_header;

public:
  void Load(FilesContainerR const & cont);

  inline feature::DataHeader const & GetHeader() const { return m_header; }

  IntervalIndexIFace * CreateIndex(ModelReaderPtr reader);
};

void LoadMapHeader(ModelReader * pReader, feature::DataHeader & header);
