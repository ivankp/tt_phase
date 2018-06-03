#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

#include <TFitResult.h>
#include <TMinuit.h>

#include "ivanp/string.hh"
#include "ivanp/math/math.hh"
#include "ivanp/program_options.hh"
#include "ivanp/binner.hh"

#include "Legendre.hh"
#include "iftty.hh"
#include "event.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using namespace ivanp::math;

#define NPAR 4
const char* pars_names[NPAR] = {"c2","c4","c6","phi2"};

std::vector<decltype(event)> events;
double total_weight = 0;

#define FWRAP(f) void f##_wrap( \
    Int_t& npar, Double_t *grad, Double_t &fval, Double_t *par, Int_t flag \
  ) { fval = f(par); }

double LogL(const double* c) {
  long double logl = 0.;
  const unsigned n = events.size();
  #pragma omp parallel for reduction(+:logl)
  for (unsigned i=0; i<n; ++i)
    logl += events[i].weight*log(Legendre(events[i].cos_theta,c));
  return -2.*logl;
};
FWRAP(LogL)

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  const char* ofname;
  int print_level = 0;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input dat files",req(),pos())
      (ofname,'o',"output file",req())
      (print_level,"--print-level",
       "-1 - quiet (also suppress all warnings)\n"
       " 0 - normal (default)\n"
       " 1 - verbose")
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    std::cerr << iftty("\033[31m",2) << e.what() << iftty("\033[0m",2) << endl;
    return 1;
  }
  // ================================================================
  double pars[NPAR], errs[NPAR];

  using clock = std::chrono::system_clock;
  using time  = std::chrono::time_point<clock>;
  time start = clock::now();

  for (auto& ifname : ifnames) {
    cout << iftty("\033[34m") << "Input file" << iftty("\033[0m")
         << ": " << ifname << endl;
    for (std::ifstream f(ifname); f; ) {
      f.read(reinterpret_cast<char*>(&event),sizeof(event));
      events.emplace_back(event);
      total_weight += event.weight;
    }
  }

  cout << iftty("\033[36m") << "Read time" << iftty("\033[0m")
       << ": " << std::chrono::duration<double>(clock::now()-start).count()
       << "s" << endl;

  start = clock::now();

  { TMinuit m(NPAR); // LogL fit ====================================
    m.SetFCN(LogL_wrap);
    m.SetPrintLevel(print_level);

    for (unsigned i=0; i<NPAR; ++i)
      m.DefineParameter(
        i,             // parameter number
        pars_names[i], // parameter name
        0,  // start value
        0.01,          // step size
        -0.5,  // mininum
        1.5   // maximum
      );

    m.Migrad();
    for (unsigned i=0; i<NPAR; ++i)
      m.GetParameter(i,pars[i],errs[i]);
  }

  cout << iftty("\033[36m") << "Fit time" << iftty("\033[0m")
       << ": " << std::chrono::duration<double>(clock::now()-start).count()
       << "s" << endl;
}
