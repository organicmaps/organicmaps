#pragma once

// Use 'extern' to eliminate duplicate data allocation.
extern const wchar_t * SCENE_MAP;
extern const wchar_t * SCENE_SETTINGS;
extern const wchar_t * SCENE_DOWNLOAD_GROUP;
extern const wchar_t * SCENE_DOWNLOAD_COUNTRY;
extern const wchar_t * SCENE_DOWNLOAD_REGION;
extern const wchar_t * SCENE_ABOUT;
extern const wchar_t * SCENE_SEARCH;
extern const wchar_t * SCENE_BMCATEGORIES;
extern const wchar_t * SCENE_SELECT_BM_CATEGORY;
extern const wchar_t * SCENE_SELECT_COLOR;
extern const wchar_t * SCENE_CATEGORY;
extern const wchar_t * SCENE_SHARE_POSITION;
extern const wchar_t * SCENE_LICENSE;

class SceneRegister
{
public:
  static void RegisterAllScenes(void);

private:
  SceneRegister(void);
  ~SceneRegister(void);
};
