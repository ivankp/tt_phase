#include <iostream>
#include <fstream>
#include <memory>

#include <boost/optional.hpp>

#include <TH1.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLatex.h>
#include <TLine.h>

#include "json.hpp"

#include "ivanp/string.hh"
#include "ivanp/math/math.hh"
#include "ivanp/program_options.hh"
#include "ivanp/expand.hh"

#include "Legendre.hh"
#include "iftty.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::endl;
using std::get;
using ivanp::cat;
using ivanp::math::sq;

#define NPAR 4
const char* par_name[NPAR] = {"c2","c4","c6","phi2"};

const int colors[] = { 418, 2 };

struct _tex: TLatex {
  _tex() { SetTextSize(0.025); }
  template <typename... F>
  TLatex* operator()(int col, int row, const std::string& str, F... f) {
    TLatex* p = DrawLatexNDC(0.15+0.2*col,0.85-0.04*row,str.c_str());
    EXPAND(f(p));
    return p;
  }
} tex;

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  std::string ofname;
  bool logy = false, more_logy = false;
  boost::optional<std::array<double,2>> y_range;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input json files",req(),pos())
      (ofname,'o',"output pdf file",req())
      (y_range,'y',"y-axis range")
      (more_logy,"--more-logy","more y-axis log labels")
      (logy,"--logy")
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    std::cerr << iftty("\033[31m",2) << e.what() << iftty("\033[0m",2) << endl;
    return 1;
  }
  // ================================================================

  TCanvas canv("","",700,600);
  gStyle->SetOptStat(0);

  TPad pad1("","",0,0.25,1,1);
  pad1.SetMargin(0.05,0.05,0,0.1);
  if (logy) pad1.SetLogy();
  TPad pad2("","",0,0,1,0.25);
  pad2.SetMargin(0.05,0.05,0.25,0);

  const unsigned npages = ifnames.size();
  unsigned page_cnt = npages;
  if (npages>1) ofname += '(';

  for (const auto& ifname : ifnames) {
    cout << ifname << endl;
    nlohmann::json json;
    std::ifstream(ifname) >> json;

    const double cos_range = json["cos_range"];
    const auto& jhist = json["hist"];
    const unsigned nbins = jhist.size();

    const std::array<double,2> M = json["info"]["M"];
    TH1D h("",cat("M #in [",M[0],',',M[1],") GeV").c_str(),nbins,-1,1);
    h.Sumw2();
    { unsigned i = 1, n = 0;
      auto& w2 = *h.GetSumw2();
      double sumw = 0;
      for (auto it=jhist.begin(); it!=jhist.end(); ++it, ++i) {
        sumw += h[i] = it->at(0);
        w2[i] = sq(it->at(1).get<double>());
        n += it->at(2).get<decltype(n)>();
      }
      h.SetEntries(n);
      h.Scale(1./(h.GetBinWidth(1)*sumw));
    }
    h.SetLineWidth(2);
    h.SetLineColor(602);

    std::vector<TF1> fits;
    const auto& jfits = json["fits"];
    fits.reserve(jfits.size());
    for (auto it=jfits.begin(); it!=jfits.end(); ++it) {
      cout << it.key() << endl;
      fits.emplace_back(it.key().c_str(),
        [](double* x, double* c){ return Legendre(*x,c); },
        -1,1,NPAR);
      auto& f = fits.back();
      const auto& fit = it.value();
      for (unsigned i=0; i<NPAR; ++i) {
        const char* name = par_name[i];
        f.SetParName(i,name);
        const std::array<double,2> p = fit.at(name);
        f.SetParameter(i,p[0]);
        f.SetParError(i,p[1]);
        f.SetLineColor(colors[fits.size()-1]);
      }
    }

    pad1.cd(); // ===================================================
    TAxis* ya = h.GetYaxis();
    if (y_range) ya->SetRangeUser((*y_range)[0],(*y_range)[1]);
    if (more_logy) ya->SetMoreLogLabels();
    h.Draw();
    for (unsigned fi=0; fi<fits.size(); ++fi) {
      auto& f = fits[fi];
      f.Draw("SAME");
      tex(fi,0,cat(f.GetName(), " fit"))->SetTextColor(colors[fi]);
      for (unsigned pi=0; pi<NPAR; ++pi) {
        tex(fi,pi+1,cat(
          ivanp::starts_with(f.GetParName(pi),"phi") ? "#" : "",
          f.GetParName(pi)," = ",f.GetParameter(pi),
          (f.GetParError(pi)==0) ? " FIXED" : ""));
      }
      tex(fi,NPAR+1,cat("#chi^{2}/ndf = ",
        jfits[f.GetName()]["chi2"].get<double>() / (nbins-NPAR) ));
      tex(fi,NPAR+2,cat("-2logL = ", jfits[f.GetName()]["logl"]));
    }
    tex(3,0,cat("Events: ",h.GetEntries()));

    pad2.cd(); // ===================================================
    auto h_rat = h;
    h_rat.SetTitle(cat(";cos #theta / ",cos_range).c_str());
    h_rat.Divide(&fits[1]);
    h_rat.Draw();
    std::vector<TF1> frats;
    frats.reserve(fits.size());
    frats.emplace_back("frat", ROOT::Math::ParamFunctor(
      [&](double* x,double*){ return fits[0](x)/fits[1](x); }),-1,1);
    frats.emplace_back("one",[](double*,double*){ return 1.; },-1,1);
    for (unsigned i=0; i<frats.size(); ++i) {
      frats[i].SetLineColor(fits[i].GetLineColor());
      frats[i].Draw("SAME");
    }

    TAxis *ax = h_rat.GetXaxis();
    TAxis *ay = h_rat.GetYaxis();
    ax->SetLabelSize(0.1);
    ay->SetLabelSize(0.1);
    ax->SetTitleSize(0.1);
    ax->SetTickLength(0.09);
    ay->SetTickLength(0.02);

    canv.cd();
    pad1.Draw();
    pad2.Draw();

    if (!--page_cnt && npages>1) ofname += ')';
    canv.Print(ofname.c_str(),cat("Title:",M[0],'-',M[1]).c_str());
    if (page_cnt==npages-1 && npages>1) ofname.pop_back();
  }
}
