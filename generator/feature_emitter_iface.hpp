#pragma once

class FeatureBuilder1;

class FeatureEmitterIFace
{
public:
  virtual ~FeatureEmitterIFace() {}
  virtual void operator() (FeatureBuilder1 const &) = 0;
};
