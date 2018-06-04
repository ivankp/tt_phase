#include <iostream>
#include <fstream>

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
  if (logy) canv.SetLogy();
  gStyle->SetOptStat(0);

  TPad pad1("","",0,0.25,1,1);
  pad1.SetMargin(0.05,0.05,0,0.1);
  TPad pad2("","",0,0,1,0.25);
  pad2.SetMargin(0.05,0.05,0.25,0);

  TLatex latex;
  latex.SetTextSize(0.025);

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

    TH1D* h = new TH1D("","",nbins,-1,1);
    h->SetXTitle(cat("cos #theta / ",cos_range).c_str());
    h->Sumw2();
    { unsigned i = 1, n = 0;
      auto& w2 = *h->GetSumw2();
      double sumw = 0;
      for (auto it=jhist.begin(); it!=jhist.end(); ++it, ++i) {
        sumw += (*h)[i] = it->at(0);
        w2[i] = sq(it->at(1).get<double>());
        n += it->at(2).get<decltype(n)>();
      }
      h->SetEntries(n);
      h->Scale(1./(h->GetBinWidth(1)*sumw));
    }
    h->SetLineWidth(2);
    h->SetLineColor(602);

    std::vector<TF1*> fits;
    const auto& jfits = json["fits"];
    fits.reserve(jfits.size());
    for (auto it=jfits.begin(); it!=jfits.end(); ++it) {
      cout << it.key() << endl;
      TF1* f = new TF1(it.key().c_str(),
        [](double* x, double* c){ return Legendre(*x,c); },
        -1,1,NPAR);
      const auto& fit = it.value();
      for (unsigned i=0; i<NPAR; ++i) {
        const char* name = par_name[i];
        f->SetParName(i,name);
        const std::array<double,2> p = fit.at(name);
        f->SetParameter(i,p[0]);
        f->SetParError(i,p[1]);
        f->SetLineColor(colors[fits.size()]);
      }
      fits.push_back(f);
    }

    pad1.cd();
    TAxis* ya = h->GetYaxis();
    if (y_range) ya->SetRangeUser((*y_range)[0],(*y_range)[1]);
    if (more_logy) ya->SetMoreLogLabels();
    h->Draw();
    const double line0 = 0.85;
    for (unsigned fi=0; fi<fits.size(); ++fi) {
      auto& f = *fits[fi];
      f.Draw("SAME");
      latex.DrawLatexNDC(0.15+0.2*fi,line0,cat(
          f.GetName(), " fit").c_str())->SetTextColor(colors[fi]);
      for (unsigned pi=0; pi<NPAR; ++pi) {
        latex.DrawLatexNDC(0.15+0.2*fi,line0-0.04*(pi+1),cat(
            f.GetParName(pi)," = ",f.GetParameter(pi),
            (f.GetParError(pi)==0) ? " FIXED" : ""
          ).c_str());
      }
      latex.DrawLatexNDC(0.15+0.2*fi,line0-0.04*(NPAR+1),cat(
          "#chi^{2}/ndf = ",
          jfits[f.GetName()]["chi2"].get<double>() / (nbins-NPAR)
        ).c_str());
      latex.DrawLatexNDC(0.15+0.2*fi,line0-0.04*(NPAR+2),cat(
          "-2logL = ", jfits[f.GetName()]["logl"]
        ).c_str());
    }
    latex.DrawLatexNDC(0.70,line0,cat("Events: ",h->GetEntries()).c_str());

    canv.cd();
    pad1.Draw();
    pad2.Draw();

    if (!--page_cnt && npages>1) ofname += ')';
    canv.Print(ofname.c_str());
    if (page_cnt==npages-1 && npages>1) ofname.pop_back();
  }
}
