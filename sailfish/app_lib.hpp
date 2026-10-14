#pragma once

// The Aurora OS and Sailfish OS application libraries share the same simple
// interface: application(), createView(), pathTo() and pathToMainQml().
// Aurora OS deprecates libsailfishapp.so.1 in favour of libauroraapp.so.2.
#if defined(OMIM_AURORA)
#include <auroraapp.h>
namespace AppLib = Aurora::Application;
#else
#include <sailfishapp.h>
namespace AppLib = SailfishApp;
#endif
