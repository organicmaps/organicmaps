project(agg)

set(SRC
  agg_curves.cpp
  agg_vcgen_stroke.cpp
)

add_library(${PROJECT_NAME} ${SRC})
