#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <chrono>

#include <boost/optional.hpp>

#include "json.hpp"

#include "ivanp/string.hh"
#include "ivanp/math/math.hh"
#include "ivanp/program_options.hh"
#include "ivanp/binner.hh"
#include "ivanp/root/minuit.hh"

#include "Legendre.hh"
#include "iftty.hh"
#include "event.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using ivanp::cat;
using namespace ivanp::math;

class {
  using clock = std::chrono::system_clock;
  using time  = std::chrono::time_point<clock>;
  time t0;
public:
  void start() { t0 = clock::now(); }
  void print(const char* msg) {
    std::ios_base::fmtflags flags(cout.flags());
    const auto prec = cout.precision();
    cout << std::fixed << std::setprecision(2);
    cout << iftty("\033[36m") << msg << iftty("\033[0m")
         << ": " << std::chrono::duration<double>(clock::now()-t0).count()
         << " s" << endl;
    cout.flags(flags);
    cout.precision(prec);
  }
} timer;

struct bin {
  double w = 0, w2 = 0;
  unsigned n = 0;
  void operator()() noexcept {
    w  += event.weight;
    w2 += sq(event.weight);
    ++n;
  }
};

#define NPAR 4
const char* par_name[NPAR] = {"c2","c4","c6","phi2"};

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  const char* ofname;
  int print_level = 0;
  unsigned nbins = 100;
  double cos_range = 1;
  boost::optional<double> fix_phi;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input dat files",req(),pos())
      (ofname,'o',"output file",req())
      (nbins,'n',cat("number of cosθ bins [",nbins,']'))
      (cos_range,'r',cat("cosθ range [",cos_range,']'))
      (fix_phi,"--phi","fix phase value")
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

  std::vector<decltype(event)> events;
  double total_weight = 0;

  ivanp::binner<bin, std::tuple<
    ivanp::axis_spec<ivanp::uniform_axis<double>,0,0> >
  > hist({nbins,-1,1});

  nlohmann::json info;

  timer.start();

  for (auto& ifname : ifnames) {
    cout << iftty("\033[34m") << "Input file" << iftty("\033[0m")
         << ": " << ifname << endl;
    std::ifstream f(ifname);
    std::string line;
    std::getline(f,line);
    info.merge_patch(nlohmann::json::parse(line));
    while (f) {
      f.read(reinterpret_cast<char*>(&event),sizeof(event));
      if (std::abs(event.cos_theta) > cos_range) continue;
      event.cos_theta /= cos_range;
      events.emplace_back(event);
      hist(event.cos_theta);
    }
  }

  for (auto& b : hist.bins()) total_weight += b.w;

  timer.print("Read time");
  timer.start();

  // Chi2 fit =====================================================
  std::vector<std::array<double,3>> chi2_data;
  chi2_data.reserve(nbins);
  const double bin_width = (hist.axis().max() - hist.axis().min())/nbins;
  for (unsigned i=0; i<nbins; ++i) {
    const auto& b = hist.bins()[i];
    chi2_data.push_back({
      b.w/(total_weight*bin_width),
      b.w2/sq(total_weight*bin_width),
      hist.axis().min() + (i+0.5)*bin_width
    });
  }

  auto fChi2 = [=,&b=chi2_data](const double* c) -> double {
    double chi2 = 0.;
    for (unsigned i=0; i<nbins; ++i)
      chi2 += sq(b[i][0] - Legendre(b[i][2],c))/b[i][1];
    return chi2;
  };

  std::array<double,NPAR>
    chi2_pars { 0, 0, 0, fix_phi ? *fix_phi : 0 },
    chi2_errs;

  auto fit_Chi2 = [&]{
    auto m = make_minuit(NPAR,fChi2);
    m.SetPrintLevel(print_level);

    for (unsigned i=0; i<NPAR; ++i)
      m.DefineParameter(
        i,           // parameter number
        par_name[i], // parameter name
        chi2_pars[i],// start value
        0.01,        // step size
        -0.5,        // mininum
        1.5          // maximum
      );

    if (fix_phi) m.FixParameter(3);

    m.Migrad();
    for (unsigned i=0; i<NPAR; ++i)
      m.GetParameter(i,chi2_pars[i],chi2_errs[i]);
  };
  fit_Chi2();

  timer.print("Chi2 fit time");
  timer.start();

  // LogL fit =====================================================
  auto fLogL = [&](const double* c) -> double {
    long double logl = 0.;
    const unsigned n = events.size();
    #pragma omp parallel for reduction(+:logl)
    for (unsigned i=0; i<n; ++i)
      logl += events[i].weight*std::log(Legendre(events[i].cos_theta,c));
    return -2.*logl;
  };

  std::array<double,NPAR> logl_pars(chi2_pars), logl_errs;

  auto fit_LogL = [&]{
    auto m = make_minuit(NPAR,fLogL);
    m.SetPrintLevel(print_level);

    for (unsigned i=0; i<NPAR; ++i)
      m.DefineParameter(
        i,           // parameter number
        par_name[i], // parameter name
        logl_pars[i],// start value
        0.01,        // step size
        -0.5,        // mininum
        1.5          // maximum
      );

    if (fix_phi) m.FixParameter(3);

    m.Migrad();
    for (unsigned i=0; i<NPAR; ++i)
      m.GetParameter(i,logl_pars[i],logl_errs[i]);
  };
  fit_LogL();

  if (fChi2(chi2_pars.data())/fChi2(logl_pars.data()) > 2.) {
    cout << "Redoing Chi2 fit" << endl;
    chi2_pars = logl_pars;
    fit_Chi2();
  }

  timer.print("LogL fit time");

  // Write output ===================================================
  std::ofstream out(ofname);
  out << "{\"info\":" << info.dump(2);
  out << std::setprecision(8);
  out << ",\n \"cos_range\":" << cos_range;
  out << std::scientific;
  out << ",\n \"fits\":{\n  \"chi2\":{";
  for (unsigned i=0; i<NPAR; ++i) {
    if (i) out << ',';
    if (!(i%2)) out << "\n    ";
    out << "\"" << par_name[i] << "\":["
        << chi2_pars[i] <<','<< chi2_errs[i] << "]";
  }
  out << ",\n    \"chi2\":" << fChi2(chi2_pars.data())
      << ",\"logl\":" << fLogL(chi2_pars.data())
      << "},\n  \"logl\":{";
  for (unsigned i=0; i<NPAR; ++i) {
    if (i) out << ',';
    if (!(i%2)) out << "\n    ";
    out << "\"" << par_name[i] << "\":["
        << logl_pars[i] <<','<< logl_errs[i] << "]";
  }
  out << ",\n    \"chi2\":" << fChi2(logl_pars.data())
      << ",\"logl\":" << fLogL(logl_pars.data())
      << "}},\n \"hist\":[";
  bool first = true;
  for (auto& b : hist.bins()) {
    if (!first) out << ',';
    else first = false;
    out << "\n  [" << b.w << ',' << std::sqrt(b.w2) << ',' << b.n << ']';
  }
  out << "\n]}";
  out.close();

  cout << iftty("\033[36m") << "Wrote " << iftty("\033[0m")
       << ofname << endl;
}
