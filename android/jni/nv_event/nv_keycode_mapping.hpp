//----------------------------------------------------------------------------------
// File:            libs\jni\nv_event\nv_keycode_mapping.h
// Samples Version: NVIDIA Android Lifecycle samples 1_0beta 
// Email:           tegradev@nvidia.com
// Web:             http://developer.nvidia.com/category/zone/mobile-development
//
// Copyright 2009-2011 NVIDIA® Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//----------------------------------------------------------------------------------

class NVKeyCodeMapping
{
public:
  NVKeyCodeMapping()
  {
    memset(m_keyMapping, 0, sizeof(NVKeyCode) * NV_MAX_KEYCODE);
  }

  void Init(JNIEnv* env, jobject thiz);

  bool MapKey(int key, NVKeyCode& code);

protected:
  void AddKeyMapping(JNIEnv* env, jobject thiz, jclass KeyCode_class, const char* name, NVKeyCode value);
  NVKeyCode m_keyMapping[NV_MAX_KEYCODE];
};

/* Init the mapping array, set up the event queue */
void NVKeyCodeMapping::AddKeyMapping(JNIEnv* env, jobject thiz, jclass KeyCode_class, const char* name, NVKeyCode value)
{
  // Add a new mapping...
  jfieldID id = env->GetStaticFieldID(KeyCode_class, name, "I");
  int keyID = env->GetStaticIntField(KeyCode_class, id);

  if (keyID < NV_MAX_KEYCODE)
  {
    /* TODO TBD Should check for collision */
    m_keyMapping[keyID] = value;
  }
}

#define AddKeymappingMacro(name, value) \
	AddKeyMapping(env, thiz, KeyCode_class, name, value)

void NVKeyCodeMapping::Init(JNIEnv* env, jobject thiz)
{
  jclass KeyCode_class = env->FindClass("android/view/KeyEvent");

  AddKeymappingMacro("KEYCODE_BACK",NV_KEYCODE_BACK);
  AddKeymappingMacro("KEYCODE_TAB",NV_KEYCODE_TAB);
  AddKeymappingMacro("KEYCODE_ENTER",NV_KEYCODE_ENTER);

  AddKeymappingMacro("KEYCODE_SPACE",NV_KEYCODE_SPACE);
  AddKeymappingMacro("KEYCODE_ENDCALL",NV_KEYCODE_ENDCALL);
  AddKeymappingMacro("KEYCODE_HOME",NV_KEYCODE_HOME);

  AddKeymappingMacro("KEYCODE_DPAD_LEFT",NV_KEYCODE_DPAD_LEFT);
  AddKeymappingMacro("KEYCODE_DPAD_UP",NV_KEYCODE_DPAD_UP);
  AddKeymappingMacro("KEYCODE_DPAD_RIGHT",NV_KEYCODE_DPAD_RIGHT);
  AddKeymappingMacro("KEYCODE_DPAD_DOWN",NV_KEYCODE_DPAD_DOWN);

  AddKeymappingMacro("KEYCODE_DEL",NV_KEYCODE_DEL);

  AddKeymappingMacro("KEYCODE_0",NV_KEYCODE_0);
  AddKeymappingMacro("KEYCODE_1",NV_KEYCODE_1);
  AddKeymappingMacro("KEYCODE_2",NV_KEYCODE_2);
  AddKeymappingMacro("KEYCODE_3",NV_KEYCODE_3);
  AddKeymappingMacro("KEYCODE_4",NV_KEYCODE_4);
  AddKeymappingMacro("KEYCODE_5",NV_KEYCODE_5);
  AddKeymappingMacro("KEYCODE_6",NV_KEYCODE_6);
  AddKeymappingMacro("KEYCODE_7",NV_KEYCODE_7);
  AddKeymappingMacro("KEYCODE_8",NV_KEYCODE_8);
  AddKeymappingMacro("KEYCODE_9",NV_KEYCODE_9);

  AddKeymappingMacro("KEYCODE_A",NV_KEYCODE_A);
  AddKeymappingMacro("KEYCODE_B",NV_KEYCODE_B);
  AddKeymappingMacro("KEYCODE_C",NV_KEYCODE_C);
  AddKeymappingMacro("KEYCODE_D",NV_KEYCODE_D);
  AddKeymappingMacro("KEYCODE_E",NV_KEYCODE_E);
  AddKeymappingMacro("KEYCODE_F",NV_KEYCODE_F);
  AddKeymappingMacro("KEYCODE_G",NV_KEYCODE_G);
  AddKeymappingMacro("KEYCODE_H",NV_KEYCODE_H);
  AddKeymappingMacro("KEYCODE_I",NV_KEYCODE_I);
  AddKeymappingMacro("KEYCODE_J",NV_KEYCODE_J);
  AddKeymappingMacro("KEYCODE_K",NV_KEYCODE_K);
  AddKeymappingMacro("KEYCODE_L",NV_KEYCODE_L);
  AddKeymappingMacro("KEYCODE_M",NV_KEYCODE_M);
  AddKeymappingMacro("KEYCODE_N",NV_KEYCODE_N);
  AddKeymappingMacro("KEYCODE_O",NV_KEYCODE_O);
  AddKeymappingMacro("KEYCODE_P",NV_KEYCODE_P);
  AddKeymappingMacro("KEYCODE_Q",NV_KEYCODE_Q);
  AddKeymappingMacro("KEYCODE_R",NV_KEYCODE_R);
  AddKeymappingMacro("KEYCODE_S",NV_KEYCODE_S);
  AddKeymappingMacro("KEYCODE_T",NV_KEYCODE_T);
  AddKeymappingMacro("KEYCODE_U",NV_KEYCODE_U);
  AddKeymappingMacro("KEYCODE_V",NV_KEYCODE_V);
  AddKeymappingMacro("KEYCODE_W",NV_KEYCODE_W);
  AddKeymappingMacro("KEYCODE_X",NV_KEYCODE_X);
  AddKeymappingMacro("KEYCODE_Y",NV_KEYCODE_Y);
  AddKeymappingMacro("KEYCODE_Z",NV_KEYCODE_Z);

  AddKeymappingMacro("KEYCODE_STAR",NV_KEYCODE_STAR);
  AddKeymappingMacro("KEYCODE_PLUS",NV_KEYCODE_PLUS);
  AddKeymappingMacro("KEYCODE_MINUS",NV_KEYCODE_MINUS);

  AddKeymappingMacro("KEYCODE_NUM",NV_KEYCODE_NUM);

  AddKeymappingMacro("KEYCODE_ALT_LEFT",NV_KEYCODE_ALT_LEFT);
  AddKeymappingMacro("KEYCODE_ALT_RIGHT",NV_KEYCODE_ALT_RIGHT);

  AddKeymappingMacro("KEYCODE_SHIFT_LEFT",NV_KEYCODE_SHIFT_LEFT);
  AddKeymappingMacro("KEYCODE_SHIFT_RIGHT",NV_KEYCODE_SHIFT_RIGHT);

  AddKeymappingMacro("KEYCODE_APOSTROPHE",NV_KEYCODE_APOSTROPHE);
  AddKeymappingMacro("KEYCODE_SEMICOLON",NV_KEYCODE_SEMICOLON);
  AddKeymappingMacro("KEYCODE_EQUALS",NV_KEYCODE_EQUALS);
  AddKeymappingMacro("KEYCODE_COMMA",NV_KEYCODE_COMMA);
  AddKeymappingMacro("KEYCODE_PERIOD",NV_KEYCODE_PERIOD);
  AddKeymappingMacro("KEYCODE_SLASH",NV_KEYCODE_SLASH);
  AddKeymappingMacro("KEYCODE_GRAVE",NV_KEYCODE_GRAVE);
  AddKeymappingMacro("KEYCODE_LEFT_BRACKET",NV_KEYCODE_LEFT_BRACKET);
  AddKeymappingMacro("KEYCODE_BACKSLASH",NV_KEYCODE_BACKSLASH);
  AddKeymappingMacro("KEYCODE_RIGHT_BRACKET",NV_KEYCODE_RIGHT_BRACKET);
}

bool NVKeyCodeMapping::MapKey(int key, NVKeyCode& code)
{
  if (key < NV_MAX_KEYCODE)
  {
    code = m_keyMapping[key];
    return true;
  }
  else
    return false;
}
