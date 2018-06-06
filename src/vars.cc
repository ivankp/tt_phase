#include <iostream>
#include <fstream>

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

#include "json.hpp"

#include "ivanp/string.hh"
#include "ivanp/math/math.hh"
#include "ivanp/program_options.hh"
#include "ivanp/timed_counter.hh"
#include "ivanp/binner.hh"

#include "float_or_double_reader.hh"
#include "iftty.hh"
#include "vec4.hh"
#include "event.hh"
#include "glob.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using std::get;
using ivanp::cat;
using namespace ivanp::math;

class mass_bin {
  std::ofstream f;
public:
  std::ofstream& open(const std::string& name) { f.open(name); return f; }
  inline void operator()() {
    f.write(reinterpret_cast<const char*>(&event),sizeof(event));
  }
};

int main(int argc, char* argv[]) {
  const char *ifname, *ofname;
  const char *tree_name = "t3";
  std::vector<double> mass_edges;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifname,'i',"input JSON config file",req(),pos())
      (ofname,'o',"output file name prefix",req())
      (mass_edges,'b',"mass binning",req())
      (tree_name,{"-t","--tree"},cat("input TTree name [",tree_name,']'))
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    cerr << iftty("\033[31m",2) << e.what() << iftty("\033[0m",2) << endl;
    return 1;
  }
  // ================================================================

  nlohmann::json info;
  std::ifstream(ifname) >> info;

  std::vector<std::string> ifnames;
  for (const std::string& f : info["files"]) {
    auto fs = ivanp::glob(f);
    if (fs.empty())
      throw std::runtime_error(cat("glob \"",f,"\" matched no files"));
    ifnames.insert(
      ifnames.end(),
      std::make_move_iterator(fs.begin()),
      std::make_move_iterator(fs.end()));
  }
  info["files"] = ifnames;

  // Open input ntuples root file ===================================
  TChain chain(tree_name);
  cout << iftty("\033[34m") << "Input ntuples" << iftty("\033[0m") << endl;
  for (const std::string& name : ifnames) {
    if (!chain.Add(name.c_str(),0)) return 1;
    cout << "  " << name << endl;
  }
  cout << endl;

  // Set up branches for reading
  TTreeReader reader(&chain);

  TTreeReaderValue<Int_t> _nparticle(reader,"nparticle");
  TTreeReaderArray<Int_t> _kf(reader,"kf");

  float_or_double_array_reader _px(reader,"px");
  float_or_double_array_reader _py(reader,"py");
  float_or_double_array_reader _pz(reader,"pz");
  float_or_double_array_reader _E (reader,"E" );
  float_or_double_value_reader _weight(reader,"weight2");

  ivanp::binner<mass_bin, std::tuple<
    ivanp::axis_spec<ivanp::container_axis<decltype(mass_edges)&>,0,0> >
  > files(mass_edges);

  for (unsigned i=0, n=files.nbins(); i<n; ++i) {
    auto& f = files.bins()[i].open(
      cat(ofname,'_',mass_edges[i],'-',mass_edges[i+1],".dat"));

    info["M"] = { mass_edges[i], mass_edges[i+1] };
    f << info << endl;
  }

  // LOOP ===========================================================
  using cnt = ivanp::timed_counter<Long64_t>;
  for (cnt ent(reader.GetEntries(true)); reader.Next(); ++ent) {
    // Read particles -----------------------------------------------
    vec4 Higgs, jets;
    const unsigned np = *_nparticle;
    for (unsigned i=0; i<np; ++i) {
      if (_kf[i]==25) {
        Higgs = {_px[i],_py[i],_pz[i],_E[i]};
      } else {
        jets += {_px[i],_py[i],_pz[i],_E[i]};
      }
    }
    // --------------------------------------------------------------

    const auto Q = Higgs + jets;
    const double Q2 = Q*Q; // Mass^2

    const vec4 Z(0,0,Q[3],Q[2]);
    const auto ell = ((Q*jets)/Q2)*Higgs - ((Q*Higgs)/Q2)*jets;

    event.cos_theta = (ell*Z) / std::sqrt(sq(ell)*sq(Z));
    // --------------------------------------------------------------

    event.weight = (*_weight);

    files(std::sqrt(Q2));
  }
}

