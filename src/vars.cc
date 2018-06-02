#include <iostream>

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

#include "ivanp/math/math.hh"
#include "ivanp/program_options.hh"
#include "ivanp/timed_counter.hh"
#include "ivanp/string.hh"

#include "float_or_double_reader.hh"
#include "iftty.hh"
#include "vec4.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using std::get;
using ivanp::cat;
using namespace ivanp::math;

class ofile {
  std::ofstream f;
public:
  ofile(const char* name): f(name) { }
  friend inline ofile& operator<<(ofile& f, double x) {
    f.f.write(reinterpret_cast<const char*>(&x),sizeof(x));
    return f;
  }
};

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  const char* ofname;
  const char* tree_name = "t3";

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input ROOT BH ntuples",req(),pos())
      (ofname,'o',"output file name",req())
      (tree_name,{"-t","--tree"},cat("input TTree name [",tree_name,']'))
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    std::cerr << iftty("\033[31m",2) << e.what() << iftty("\033[0m",2) << endl;
    return 1;
  }
  // ================================================================

  // Open input ntuples root file ===================================
  TChain chain(tree_name);
  cout << iftty("\033[33m") << "Input ntuples" << iftty("\033[0m") << endl;
  for (const char* name : ifnames) {
    if (!chain.Add(name,0)) return 1;
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

  ofile f(ofname); // Output file

  vec4 Higgs, jets;

  // LOOP ===========================================================
  using cnt = ivanp::timed_counter<Long64_t>;
  for (cnt ent(reader.GetEntries(true)); reader.Next(); ++ent) {
    // Read particles -----------------------------------------------
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
    const double Q2 = Q*Q;
    const auto hj_mass = std::sqrt(Q2);

    const vec4 Z(0,0,Q[3],Q[2]);
    const auto ell = ((Q*jets)/Q2)*Higgs - ((Q*Higgs)/Q2)*jets;

    const auto cos_theta = (ell*Z) / std::sqrt(sq(ell)*sq(Z));
    // --------------------------------------------------------------

    f << (*_weight) << hj_mass << cos_theta;
  }
}

