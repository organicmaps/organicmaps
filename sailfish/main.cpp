// Everything lives in liborganicmaps.so. On aarch64 libhybris GL drivers keep their current
// context in bionic TLS slots right after the thread pointer, where the static linker would place
// an executable's own thread_local data; shared library TLS is laid out by the patched loader.
int OrganicMapsMain(int argc, char * argv[]);

// Exported, as Q_DECL_EXPORT in sailfishapp.h/auroraapp.h: the booster dlopens the executable and calls main().
__attribute__((visibility("default"))) int main(int argc, char * argv[])
{
  return OrganicMapsMain(argc, argv);
}
