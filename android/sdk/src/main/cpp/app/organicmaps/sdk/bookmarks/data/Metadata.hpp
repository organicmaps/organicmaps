#pragma once

#include <jni.h>

#include "indexer/map_object.hpp"

jobject CreateMetadata(JNIEnv * env, osm::MapObject const & mapObject);
