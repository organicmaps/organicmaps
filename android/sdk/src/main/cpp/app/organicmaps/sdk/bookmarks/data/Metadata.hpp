#pragma once

#include <jni.h>

#include "indexer/map_object.hpp"

// Fills mapobject's metadata.
void InjectMetadata(JNIEnv * env, jclass clazz, jobject const mapObject, osm::MapObject const & src);
