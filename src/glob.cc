#include "glob.hh"
#include <stdexcept>
#include "ivanp/string.hh"
#include "glob.h"

namespace ivanp {

[[ noreturn ]] int glob_err(const char *epath, int eerrno) {
  throw std::runtime_error(cat("glob error ",eerrno,": ",epath));
}

std::vector<std::string> glob(const std::string& pattern) {
  glob_t globbuf;
  glob(
    pattern.c_str(),
    GLOB_BRACE | GLOB_TILDE,
    glob_err,
    &globbuf);

  std::vector<std::string> list;
  list.reserve(globbuf.gl_pathc);
  for (decltype(globbuf.gl_pathc) i=0; i<globbuf.gl_pathc; ++i)
    list.emplace_back(globbuf.gl_pathv[i]);

  if (globbuf.gl_pathc > 0) globfree(&globbuf);
  return list;
}

}
