// test_6.cpp
// Streaming AR(p) / ARMA(p,q) with EW ridge LS over multiple half-lives.
// Uses ONLY: ts (INT64), bid_px (INT64), ask_px (INT64).
// Price = (bid_px + ask_px)/2.0
// Segment reset if Δts ≤ 0 or Δts > 2×STEP.

#include <parquet/api/reader.h>
#include <parquet/schema.h>
#include <parquet/types.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

// ---------- utils ----------
static inline string lower(string s){ for(char& c:s) c=tolower((unsigned char)c); return s; }

static bool parse_step_to_ns(const string& in, int64_t& out_ns, string& label){
  string s=in;
  if(!s.empty() && all_of(s.begin(),s.end(),::isdigit)) s+="s";
  static const regex r1(R"(^([0-9]+(?:\.[0-9]+)?)s$)",regex::icase);
  static const regex r2(R"(^([0-9]+)(ns|us|µs|ms|s|m|h)$)",regex::icase);
  smatch m; long double ns=0;
  if(regex_match(s,m,r1)){
    ns = stold(m[1].str())*1'000'000'000.0L;
  }else if(regex_match(s,m,r2)){
    long double v=stold(m[1].str());
    string u=lower(m[2].str());
    if(u=="ns") ns=v;
    else if(u=="us"||u=="µs") ns=v*1'000.0L;
    else if(u=="ms") ns=v*1'000'000.0L;
    else if(u=="s")  ns=v*1'000'000'000.0L;
    else if(u=="m")  ns=v*60.0L*1'000'000'000.0L;
    else if(u=="h")  ns=v*3600.0L*1'000'000'000.0L;
    else return false;
  }else return false;
  out_ns=(int64_t)llround(ns);
  if(out_ns%1'000'000'000LL==0) label=to_string(out_ns/1'000'000'000LL)+"s";
  else if(out_ns%1'000'000LL==0) label=to_string(out_ns/1'000'000LL)+"ms";
  else if(out_ns%1'000LL==0)     label=to_string(out_ns/1'000LL)+"us";
  else                           label=to_string(out_ns)+"ns";
  return out_ns>0;
}

// *_SYMB_YYYY_M_STEP.parquet
static vector<pair<tuple<int,int>,string>>
find_parquet_files(const string& root, const string& symb, const string& step_label){
  vector<pair<tuple<int,int>,string>> out;
  regex pat("^.*_"+symb+R"(_([0-9]{4})_([0-9]{1,2})_)"+step_label+R"(\.parquet$)");
  if(!fs::exists(root)) return out;
  for(auto it=fs::recursive_directory_iterator(root); it!=fs::recursive_directory_iterator(); ++it){
    if(!it->is_regular_file()) continue;
    if(it->path().extension()!=".parquet") continue;
    string fn=it->path().filename().string();
    smatch m;
    if(regex_match(fn,m,pat)){
      int y=stoi(m[1].str()), mm=stoi(m[2].str());
      out.emplace_back(make_tuple(y,mm), it->path().string());
    }
  }
  sort(out.begin(), out.end(), [](auto&a,auto&b){return a.first<b.first;});
  return out;
}

// exact name or last token (handles dotted paths)
static int col_index_exact(const parquet::SchemaDescriptor* schema, const string& expect){
  auto last_token = [](const string& s)->string{
    auto pos = s.rfind('.');
    return (pos==string::npos) ? s : s.substr(pos+1);
  };
  const string e = lower(expect);
  for(int i=0;i<schema->num_columns();++i){
    string full = lower(schema->Column(i)->path()->ToDotString());
    string base = lower(last_token(full));
    if(full==e || base==e) return i;
  }
  return -1;
}

// INT64 -> vector<int64_t>
static void read_int64_col(parquet::RowGroupReader& rg, int col, vector<int64_t>& dst){
  auto col_reader = rg.Column(col);
  auto phys = col_reader->descr()->physical_type();
  int64_t n = rg.metadata()->num_rows();
  dst.assign(n, 0);
  int64_t off=0;
  if(phys==parquet::Type::INT64){
    auto* r = static_cast<parquet::Int64Reader*>(col_reader.get());
    while(off<n){
      int64_t rn=0; r->ReadBatch(n-off,nullptr,nullptr,dst.data()+off,&rn);
      if(rn<=0) break; off+=rn;
    }
  }else{
    throw runtime_error("Expected INT64 column.");
  }
  if(off!=n) throw runtime_error("Short read on INT64 column.");
}

// INT64 (optionally DECIMAL-annotated) -> vector<double> with 10^{-scale}
static void read_int64_as_double(parquet::RowGroupReader& rg, int col, vector<double>& dst){
  auto col_reader = rg.Column(col);
  auto* descr = col_reader->descr();
  int64_t n = rg.metadata()->num_rows();
  dst.assign(n, 0.0);

  // decimal scale if annotated
  double scale_mul = 1.0;
  if (auto lt = descr->logical_type(); lt && lt->is_decimal()) {
    if (auto* dlt = dynamic_cast<const parquet::DecimalLogicalType*>(lt.get())) {
      scale_mul = pow(10.0, -dlt->scale());
    }
  }

  auto phys = descr->physical_type();
  if(phys!=parquet::Type::INT64) throw runtime_error("Expected INT64 for price columns.");
  auto* r = static_cast<parquet::Int64Reader*>(col_reader.get());

  vector<int64_t> buf(n);
  int64_t off=0;
  while(off<n){
    int64_t rn=0; r->ReadBatch(n-off,nullptr,nullptr,buf.data()+off,&rn);
    if(rn<=0) break; off+=rn;
  }
  if(off!=n) throw runtime_error("Short read on INT64 column.");
  for(int64_t i=0;i<n;++i) dst[i] = (double)buf[i] * scale_mul;
  for(int64_t i=0;i<n;++i) dst[i] = (double)buf[i] * 1e-8;
}

// ---------- EW-RLS worker ----------
struct HLWorker{
  int p, q;
  double lambda, ridge;
  int d;
  vector<double> Sxx, Sxy;
  uint64_t used=0;

  HLWorker(int p_, int q_, double lambda_, double ridge_)
  : p(p_), q(q_), lambda(lambda_), ridge(ridge_), d(p_+q_+1),
    Sxx(d*d,0.0), Sxy(d,0.0) {}

  inline void update(const deque<double>& y_lags, const deque<double>& e_lags, double y){
    vector<double> x(d,1.0);
    for(int i=0;i<p;++i) x[i] = y_lags[i];
    for(int j=0;j<q;++j) x[p+j] = (j<(int)e_lags.size()? e_lags[j] : 0.0);
    for(double& v: Sxy) v *= lambda;
    for(double& v: Sxx) v *= lambda;
    for(int i=0;i<d;++i){
      Sxy[i] += x[i]*y;
      double* row=&Sxx[i*d];
      for(int j=0;j<d;++j) row[j] += x[i]*x[j];
    }
    ++used;
  }

  bool solve(vector<double>& theta) const {
    const int d=this->d;
    theta.assign(d,0.0);
    vector<double> A=Sxx;
    for(int i=0;i<d;++i) A[i*d+i]+=ridge;

    vector<double> L(d*d,0.0);
    for(int i=0;i<d;++i){
      for(int j=0;j<=i;++j){
        double sum=A[i*d+j];
        for(int k=0;k<j;++k) sum -= L[i*d+k]*L[j*d+k];
        if(i==j){
          if(sum<=1e-30) return false;
          L[i*d+j]=sqrt(sum);
        }else{
          L[i*d+j]=sum/L[j*d+j];
        }
      }
    }
    vector<double> z(d,0.0);
    for(int i=0;i<d;++i){
      double sum=Sxy[i];
      for(int k=0;k<i;++k) sum -= L[i*d+k]*z[k];
      z[i]=sum/L[i*d+i];
    }
    for(int i=d-1;i>=0;--i){
      double sum=z[i];
      for(int k=i+1;k<d;++k) sum -= L[k*d+i]*theta[k];
      theta[i]=sum/L[i*d+i];
    }
    return true;
  }

  static double max_abs_root(const vector<double>& a){
    int m=(int)a.size(); if(m<=0) return 0.0;
    vector<double> v(m,0.0), w(m,0.0);
    for(int i=0;i<m;++i) v[i]=1.0/(i+1.0);
    double rho=0.0;
    for(int it=0; it<64; ++it){
      long double s=0; for(int j=0;j<m;++j) s += (long double)a[j]*(long double)v[j];
      w[0]=(double)s; for(int i=1;i<m;++i) w[i]=v[i-1];
      long double n=0; for(int i=0;i<m;++i) n += (long double)w[i]*w[i];
      double nn=(double)sqrt((double)n);
      if(!isfinite(nn) || nn==0.0){ rho=numeric_limits<double>::infinity(); break; }
      for(int i=0;i<m;++i) v[i]=w[i]/nn; rho=nn;
    }
    vector<double> u(m,0.0);
    long double s=0; for(int j=0;j<m;++j) s += (long double)a[j]*(long double)v[j];
    u[0]=(double)s; for(int i=1;i<m;++i) u[i]=v[i-1];
    long double num=0, den=0;
    for(int i=0;i<m;++i){ num+= (long double)u[i]*u[i]; den+= (long double)v[i]*v[i]; }
    if(den>0) rho=(double)sqrt((double)(num/den));
    return fabs(rho);
  }

  double n_eff_est() const { return (lambda>=1.0)?numeric_limits<double>::infinity():1.0/(1.0-lambda); }
};

// ---------- CLI options ----------
struct Opts{
  string in_root, symb, step_in, step_label;
  int64_t step_ns=0;
  string model="ar";  // "ar" or "arma"
  int p=5;
  int q=0;
  string transform="logret";
  vector<double> half_lives;
  double ridge=1e-8;
  uint64_t print_every=0;
  string out_csv="ar_hl_{HL}.csv";
  double oos_frac=0.2;
  int lb_lags=20;
};

static void usage(const char* a0){
  cerr <<
"Usage:\n"
"  " << a0 << " <in_root> <SYMBOL> <STEP> [flags]\n\n"
"Flags:\n"
"  --model=ar|arma                 (default ar)\n"
"  --p=N                           (default 5)\n"
"  --q=N                           (MA order; used when --model=arma)\n"
"  --transform=logret|ret          (default logret)\n"
"  --ew-half-life=1,2,4,...        (bars)\n"
"  --ridge=1e-8                    (Ridge)\n"
"  --print-every=K                 (snapshot every K obs)\n"
"  --out-csv=ar_hl_{HL}.csv        (pattern)\n"
"  --oos-frac=0.2                  (test fraction)\n"
"  --lb-lags=20                    (lags for Ljung–Box & ACF)\n";
}

static bool parse_hl_list(const string& s, vector<double>& v){
  v.clear(); string t;
  for(size_t i=0;i<=s.size();++i){
    if(i==s.size() || s[i]==','){
      if(!t.empty()){ v.push_back(stod(t)); t.clear(); }
    }else t+=s[i];
  }
  return !v.empty();
}

static bool parse_opts(int argc, char** argv, Opts& o){
  if(argc<4){ usage(argv[0]); return false; }
  o.in_root=argv[1]; o.symb=argv[2]; o.step_in=argv[3];
  if(!parse_step_to_ns(o.step_in,o.step_ns,o.step_label)){ cerr<<"Bad STEP\n"; return false; }

  for(int i=4;i<argc;++i){
    string a=argv[i];
    if(a.rfind("--model=",0)==0){ o.model=lower(a.substr(8)); }
    else if(a.rfind("--p=",0)==0) o.p=stoi(a.substr(4));
    else if(a.rfind("--q=",0)==0) o.q=stoi(a.substr(4));
    else if(a.rfind("--transform=",0)==0) o.transform=a.substr(12);
    else if(a.rfind("--ew-half-life=",0)==0){ if(!parse_hl_list(a.substr(15),o.half_lives)) return false; }
    else if(a.rfind("--ridge=",0)==0) o.ridge=stod(a.substr(8));
    else if(a.rfind("--print-every=",0)==0) o.print_every=stoull(a.substr(14));
    else if(a.rfind("--out-csv=",0)==0) o.out_csv=a.substr(10);
    else if(a.rfind("--oos-frac=",0)==0) o.oos_frac=stod(a.substr(11));
    else if(a.rfind("--lb-lags=",0)==0) o.lb_lags=stoi(a.substr(10));
    else { cerr<<"Unknown flag: "<<a<<"\n"; return false; }
  }
  if(o.half_lives.empty()){ cerr<<"Need --ew-half-life\n"; return false; }
  if(!(o.oos_frac>0.0 && o.oos_frac<0.9)){ cerr<<"--oos-frac must be in (0,0.9)\n"; return false; }
  if(o.lb_lags<=0 || o.lb_lags>200){ cerr<<"--lb-lags should be in [1..200]\n"; return false; }
  if(o.model!="ar" && o.model!="arma"){ cerr<<"--model must be ar or arma\n"; return false; }
  if(o.model=="arma" && o.q<=0){ cerr<<"For --model=arma please set --q (e.g. --q=5..10)\n"; return false; }
  return true;
}

// ---------- data ----------
struct Series{
  vector<double> y;
  vector<char> seg_start;
};

static Series load_series(const vector<pair<tuple<int,int>,string>>& files,
                          const string& transform,
                          int64_t step_ns){
  Series s;
  double prev_px = numeric_limits<double>::quiet_NaN();
  bool next_y_starts_segment=false;
  int64_t prev_ts = std::numeric_limits<int64_t>::min();

  for(const auto& pr: files){
    const string path=pr.second;
    try{
      cout<<"path "<<path<<endl;
      auto reader = parquet::ParquetFileReader::OpenFile(path, false);
      const parquet::SchemaDescriptor* schema = reader->metadata()->schema();

      // REQUIRED: ts, bid_px, ask_px
      int ts_col  = col_index_exact(schema, "ts");
      int bid_col = col_index_exact(schema, "bid_px");
      int ask_col = col_index_exact(schema, "ask_px");
      if(ts_col<0 || bid_col<0 || ask_col<0){
        cerr<<"ERROR: required columns ts/bid_px/ask_px not found in "<<path<<"\n";
        continue;
      }

      int row_groups = reader->metadata()->num_row_groups();
      for(int rg=0; rg<row_groups; ++rg){
        auto rgg = reader->RowGroup(rg);
        vector<int64_t> tsv;
        vector<double> bidv, askv;

        read_int64_col(*rgg, ts_col, tsv);
        read_int64_as_double(*rgg, bid_col, bidv);
        read_int64_as_double(*rgg, ask_col, askv);

        size_t n = min(tsv.size(), min(bidv.size(), askv.size()));
        for(size_t i=0;i<n;++i){
          const int64_t tsi = tsv[i];
          double bid = bidv[i], ask = askv[i];
          if(!(isfinite(bid) && isfinite(ask) && bid>0 && ask>0)){
            prev_px=numeric_limits<double>::quiet_NaN();
            next_y_starts_segment=true;
            continue;
          }
          const double pxi = 0.5*(bid + ask);
          //printf("bid %f ask %f\n", bid, ask);

          bool gap=false;
          if(prev_ts!=std::numeric_limits<int64_t>::min()){
            int64_t dt = tsi - prev_ts;
            if(dt <= 0) gap = true;
            else if(step_ns>0 && dt > 2*step_ns) gap = true;
          }
          if(isnan(prev_px) || gap){
            prev_px = pxi;
            prev_ts = tsi;
            next_y_starts_segment = true;
            continue;
          }

          const double y = (transform=="logret") ? (log(pxi)-log(prev_px)) : (pxi/prev_px-1.0);
          prev_px = pxi;
          prev_ts = tsi;
          s.y.push_back(y);
          s.seg_start.push_back(next_y_starts_segment ? 1 : 0);
          next_y_starts_segment=false;
        }
      }

      // file boundary => start new segment next time
      prev_px = std::numeric_limits<double>::quiet_NaN();
      prev_ts = std::numeric_limits<int64_t>::min();
      next_y_starts_segment = true;

    }catch(const std::exception& e){
      cerr<<"ERROR reading "<<path<<": "<<e.what()<<"\n";
    }
  }
  return s;
}

// ---------- ACF + Ljung–Box ----------
struct LbAcf{
  vector<double> rho; // rho[1..h]
  double Q=nan("");
  int df=0;
  double pval=nan("");
};

static double chi2_sf(double x, double k){
  long double a = 0.5L * k, xx = 0.5L * x;
  if(xx<=0) return 1.0;
  if(xx < a+1){
    long double term = 1.0L/a, sum = term;
    for(int n=1;n<200;++n){ term *= xx/(a+n); sum += term; if(fabsl(term) < 1e-18L) break; }
    long double P = expl(a*logl(xx) - xx - lgammal(a)) * sum;
    long double Q = 1.0L - P;
    return (double)max(0.0L, min(1.0L, Q));
  }else{
    long double t = (xx - a) / sqrtl(2*xx);
    long double approx = 0.5L * erfc((double)t);
    if(!isfinite((double)approx)) approx = 0.0L;
    return (double)max(0.0L,min(1.0L,approx));
  }
}

static LbAcf compute_lb_acf(const vector<double>& resid, const vector<int>& seg_id, int h){
  LbAcf R; const int n=(int)resid.size();
  R.rho.assign(h+1,0.0);
  if(n<=1){ R.Q=nan(""); R.df=max(0,h); R.pval=nan(""); return R; }

  long double mean=0; for(double e: resid) mean += e; mean /= (long double)n;
  long double gamma0=0; for(double e: resid){ long double d=e-mean; gamma0 += d*d; }
  if(gamma0<=0){ R.Q=nan(""); R.df=max(0,h); R.pval=nan(""); return R; }

  vector<int> cnt(h+1,0);
  for(int k=1;k<=h;++k){
    long double s=0; int used=0;
    for(int t=k; t<n; ++t){
      if(seg_id[t]==seg_id[t-k]){ s += ((long double)resid[t]-mean)*((long double)resid[t-k]-mean); ++used; }
    }
    cnt[k]=used; R.rho[k] = (double)(s / gamma0);
  }

  long double Q=0;
  for(int k=1;k<=h;++k){
    if(cnt[k]<=0) continue;
    Q += (long double)n*(n+2.0L) * ((long double)R.rho[k]*(long double)R.rho[k]) / (n - k);
  }
  R.Q=(double)Q; R.df=max(0,h); R.pval=chi2_sf(R.Q, R.df);
  return R;
}

// ---------- helpers ----------
static inline void shift_push_front(deque<double>& dq, double v){
  for(int k=(int)dq.size()-1;k>0;--k) dq[k]=dq[k-1];
  if(!dq.empty()) dq[0]=v;
}

static inline double predict_yhat(const vector<double>& theta, int p, int q,
                                  const deque<double>& y_lags,
                                  const deque<double>& e_lags){
  const size_t d = theta.size(); if(d==0) return 0.0;
  double yhat = 0.0;
  if(d == (size_t)(p+q+1)){
    for(int i=0;i<p;++i) yhat += theta[i]* (i<(int)y_lags.size()? y_lags[i]:0.0);
    for(int j=0;j<q;++j) yhat += theta[p+j]* (j<(int)e_lags.size()? e_lags[j]:0.0);
    yhat += theta[p+q];
  }else if(d == (size_t)(p+1)){
    for(int i=0;i<p;++i) yhat += theta[i]* (i<(int)y_lags.size()? y_lags[i]:0.0);
    yhat += theta[p];
  }else{
    for(size_t i=0;i<min(d,(size_t)y_lags.size()); ++i) yhat += theta[i]*y_lags[i];
  }
  return yhat;
}

// ---------- main ----------
int main(int argc, char** argv){
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  Opts opt;
  if(!parse_opts(argc,argv,opt)) return 1;

  auto files = find_parquet_files(opt.in_root, opt.symb, opt.step_label);
  if(files.empty()){
    cerr<<"No parquet matched under "<<opt.in_root<<" for "<<opt.symb<<" step="<<opt.step_label<<"\n";
    return 2;
  }

  // load series
  Series S = load_series(files, opt.transform, opt.step_ns);
  const size_t N = S.y.size();
  if(N==0){ cerr<<"No returns computed\n"; return 3; }

  // train/test split
  size_t test_n = (size_t)floor(opt.oos_frac * (double)N);
  size_t train_n = N - test_n;

  // per-HL workers
  struct W{ double HL, lambda; HLWorker worker; vector<double> theta; deque<double> e_lags; };
  vector<W> ws; ws.reserve(opt.half_lives.size());
  for(double HL: opt.half_lives){
    double lambda = pow(0.5, 1.0/HL);
    ws.push_back(W{HL, lambda, HLWorker(opt.p, (opt.model=="arma"?opt.q:0), lambda, opt.ridge), {}, deque<double>(max(0,opt.q), 0.0)});
  }

  // Train streaming
  deque<double> y_lags(opt.p, 0.0); int y_warm=0;
  auto reset_y_lags = [&](){ for(int i=0;i<opt.p;++i) y_lags[i]=0.0; y_warm=0; };
  auto reset_e_lags = [&](W& w){ for(int j=0;j<opt.q;++j){ if((int)w.e_lags.size()<opt.q) w.e_lags.push_back(0.0); w.e_lags[j]=0.0; } };

  reset_y_lags(); for(auto& w: ws) reset_e_lags(w);

  uint64_t obs_print=0;
  for(size_t i=0;i<train_n; ++i){
    if(S.seg_start[i]){ reset_y_lags(); for(auto& w: ws) reset_e_lags(w); }
    const double y = S.y[i];

    if(y_warm < opt.p){ shift_push_front(y_lags, y); ++y_warm; continue; }

    for(auto& w: ws){
      const double yhat = predict_yhat(w.theta, opt.p, (opt.model=="arma"?opt.q:0), y_lags, w.e_lags);
      const double e = y - yhat;
      w.worker.update(y_lags, w.e_lags, y);
      w.worker.solve(w.theta);
      if(opt.model=="arma"){ if((int)w.e_lags.size()<opt.q) w.e_lags.resize(opt.q,0.0); shift_push_front(w.e_lags, e); }
    }

    ++obs_print;
    if(opt.print_every && (obs_print % opt.print_every)==0){
      for(auto& w: ws){
        w.worker.solve(w.theta);
        vector<double> phi(opt.p,0.0), th(opt.q,0.0);
        if(!w.theta.empty()){
          for(int i2=0;i2<opt.p && i2<(int)w.theta.size(); ++i2) phi[i2]=w.theta[i2];
          for(int j=0;j<opt.q && (opt.p+j)<(int)w.theta.size(); ++j) th[j]=w.theta[opt.p+j];
        }
        double rho_ar = HLWorker::max_abs_root(phi);
        bool stable_ar = isfinite(rho_ar) && (rho_ar < 1.0);
        double rho_ma = opt.q>0 ? HLWorker::max_abs_root(th) : 0.0;
        bool inv_ma = (opt.q==0) || (isfinite(rho_ma) && rho_ma<1.0);

        cout.setf(std::ios::fixed); cout<<setprecision(7);
        cout<<"[half-life экспоненциального затухания EW HL="<<(long long)w.HL<<"] intercept="<<(w.theta.empty()?0.0:w.theta.back());
        for(int i2=0;i2<opt.p;++i2) cout<<" phi"<<(i2+1)<<"="<<phi[i2];
        for(int j=0;j<opt.q;++j)  cout<<" ma"<<(j+1)<<"="<<th[j];
        cout<<" | stable="<<(stable_ar?1:0)<<" max|r|="<<rho_ar;
        if(opt.q>0) cout<<" | invMA="<<(inv_ma?1:0)<<" max|r_ma|="<<rho_ma;
        if(!stable_ar) cout<<"  WARN: "<<(isfinite(rho_ar)?"near-unstable":"unstable");
        cout<<"\n";
      }
    }

    shift_push_front(y_lags, y);
  }

  cout<<(opt.model=="arma" ? "ARMA(" : "AR(")<<opt.p<<(opt.model=="arma" ? (string(",")+to_string(opt.q)+")") : string(")"))
      <<" for "<<opt.symb<<" step="<<opt.step_label
      <<" transform="<<opt.transform<<"\n";
  cout<<"Obs_total="<<N<<" Train="<<train_n<<" Test="<<test_n<<" HLs="<<ws.size()<<"\n";

  for(auto& w: ws) w.worker.solve(w.theta);

  auto csv_name = [&](double HL){
    string s=opt.out_csv; size_t pos = s.find("{HL}");
    if(pos!=string::npos){ ostringstream oss; oss<<(long long)HL; s.replace(pos,4,oss.str()); }
    return s;
  };

  // Evaluate per HL
  for(auto& w: ws){
    vector<double> phi(opt.p, 0.0), th(opt.q, 0.0);
    double icpt=0.0;
    if(!w.theta.empty()){
      for(int i=0;i<opt.p && i<(int)w.theta.size(); ++i) phi[i]=w.theta[i];
      for(int j=0;j<opt.q && (opt.p+j)<(int)w.theta.size(); ++j) th[j]=w.theta[opt.p+j];
      icpt = w.theta.back();
    }

    double max_r_ar = HLWorker::max_abs_root(phi);
    bool stable_ar = isfinite(max_r_ar) && max_r_ar<1.0;
    double max_r_ma = opt.q>0 ? HLWorker::max_abs_root(th) : 0.0;
    bool inv_ma = (opt.q==0) || (isfinite(max_r_ma) && max_r_ma<1.0);

    vector<double> resid; resid.reserve(train_n);
    vector<int> resid_seg; resid_seg.reserve(train_n);
    deque<double> yl(opt.p,0.0), el(opt.q,0.0);
    int warm=0, seg_id=-1;
    for(size_t i=0;i<train_n; ++i){
      if(S.seg_start[i]){ for(int k=0;k<opt.p;++k) yl[k]=0.0; for(int j=0;j<opt.q;++j) el[j]=0.0; warm=0; ++seg_id; }
      const double y = S.y[i];
      if(warm < opt.p){ shift_push_front(yl, y); ++warm; continue; }
      double yhat = icpt; for(int k=0;k<opt.p;++k) yhat += phi[k]*yl[k]; for(int j=0;j<opt.q;++j) yhat += th[j]*el[j];
      double e = y - yhat;
      resid.push_back(e); resid_seg.push_back(seg_id);
      shift_push_front(yl, y); if(opt.q>0) shift_push_front(el, e);
    }

    LbAcf L = compute_lb_acf(resid, resid_seg, opt.lb_lags);
    int df = max(0, opt.lb_lags - opt.p - (opt.model=="arma"?opt.q:0));
    double pval = chi2_sf(L.Q, df);

    deque<double> yq(opt.p,0.0), eq(opt.q,0.0);
    int warm_y=0;
    for(size_t i=0;i<train_n; ++i){
      if(S.seg_start[i]){ for(int k=0;k<opt.p;++k) yq[k]=0.0; for(int j=0;j<opt.q;++j) eq[j]=0.0; warm_y=0; }
      const double y = S.y[i]; shift_push_front(yq, y); if(warm_y<opt.p) ++warm_y;
    }

    vector<double> y_true; y_true.reserve(test_n);
    vector<double> y_pred; y_pred.reserve(test_n);
    int warm_filled = warm_y;
    for(size_t i=train_n; i<N; ++i){
      if(S.seg_start[i]){ for(int k=0;k<opt.p;++k) yq[k]=0.0; for(int j=0;j<opt.q;++j) eq[j]=0.0; warm_filled=0; }
      const double y = S.y[i];
      if(warm_filled >= opt.p){
        double yhat = icpt; for(int k=0;k<opt.p;++k) yhat += phi[k]*yq[k]; for(int j=0;j<opt.q;++j) yhat += th[j]*eq[j];
        y_true.push_back(y); y_pred.push_back(yhat);
        if(opt.q>0) shift_push_front(eq, y - yhat);
      }
      shift_push_front(yq, y); if(warm_filled<opt.p) ++warm_filled;
    }

    auto safe_mean = [](const vector<double>& v)->double{ if(v.empty()) return nan(""); long double s=0; for(double x: v) s+=x; return (double)(s/(long double)v.size()); };
    double mu_y = safe_mean(y_true);
    long double sse=0, sst=0, sae=0, ss2=0, sy2=0, spy=0;
    for(size_t i=0;i<y_true.size();++i){
      double e = y_true[i] - y_pred[i];
      sse += (long double)e*e; sae += fabsl(e);
      long double dy = (long double)y_true[i] - mu_y; sst += dy*dy;
      ss2 += (long double)y_pred[i]*y_pred[i]; sy2 += (long double)y_true[i]*y_true[i];
      spy += (long double)y_pred[i]*y_true[i];
    }
    double rmse = y_true.empty()? nan("") : (double)sqrt(sse / (long double)max<size_t>(1,y_true.size()));
    double mae  = y_true.empty()? nan("") : (double)(sae / (long double)max<size_t>(1,y_true.size()));
    double r2   = (sst<=0 || !isfinite((double)sst)) ? nan("") : (double)(1.0L - sse/sst);
    double corr = (ss2<=0 || sy2<=0) ? nan("") : (double)(spy / sqrtl(ss2*sy2));

    cout.setf(std::ios::fixed); cout<<setprecision(7);
    cout<<"[EW HL="<<(long long)w.HL<<"] stable="<<(stable_ar?1:0)<<" max|r|="<<max_r_ar;
    if(opt.q>0) cout<<" invMA="<<(inv_ma?1:0)<<" max|r_ma|="<<max_r_ma;
    cout<<" | LB(h="<<opt.lb_lags<<"): Q="<<setprecision(10)<<L.Q<<", df="<<df<<", p="<<setprecision(4)<<pval;
    cout<<setprecision(5)<<" | ACF:"; for(int k=1;k<=opt.lb_lags; ++k){ cout<<" r"<<k<<"="<<((int)k<(int)L.rho.size()? L.rho[k]:nan("")); }
    cout<<setprecision(7)<<" | OOS: RMSE="<<rmse<<", MAE="<<mae<<", R2="<<r2<<", corr="<<corr<<"\n";

    string fn = csv_name(w.HL);
    ofstream f(fn); f.setf(std::ios::fixed); f<<setprecision(10);
    f<<"[half-life экспоненциального затухания EW HL="<<(long long)w.HL<<"]\n"<< "эффективный размер выборки N_eff~="<<w.worker.n_eff_est()<<"\n"<<" (коэффициент экспоненциального затухания lambda="<<w.lambda<<")\n";
    f<<"model,"<<(opt.model=="arma"?"arma":"ar")<<"(Параметры модели AR(5): x(t) = intercept + phi1*x(t-1) + phi2*x(t-2) + phi3*x(t-3) + phi4*x(t-4) + phi5*x(t-5) + ϵ(t))\n";
    f<<"intercept,"<<icpt<<"\n";
    for(int i=0;i<opt.p;++i) f<<"phi"<<(i+1)<<","<<phi[i]<<"\n";
    for(int j=0;j<opt.q;++j) f<<"ma"<<(j+1)<<","<<th[j]<<"\n";
    f<<"модель стационарна (корни характеристического уравнения внутри единичного круга) stable_ar= "<<(stable_ar?1:0)<<" , наибольший модуль корня < 1 (значит, процесс устойчивый) max|r_ar| "<<max_r_ar<<"\n"<<
        "Гипотеза: остатки не автокоррелированы. Q-статистика огромная → p-value≈0 → гипотеза отвергается → остатки не похожи на белый шум, значит AR(5) модель не полностью захватила структуру данных.\n";
    if(opt.q>0) f<<"inv_ma="<<(inv_ma?1:0)<<", max|r_ma|,"<<max_r_ma<<"\n";
    f<<"Ljung–Box тест (проверка остатков на белый шум): \n";
    f<<"lb_h,"<<opt.lb_lags<<"\n";
    f<<"lb_Q,"<<L.Q<<"\n";
    f<<"lb_df,"<<df<<"\n";
    f<<"lb_pval,"<<pval<<"\n";
    f<<"ACF (автокорреляционная функция остатков):\n";
    for(int k=1;k<=opt.lb_lags;++k) f<<"acf_rho_"<<k<<","<<((int)k<(int)L.rho.size()? L.rho[k]:nan(""))<<"\n";
    f<<"если остатки заметно коррелированы с лагом 1, сигнал не «вычищен»\n";
    f<<"если дальше коэффициенты быстро падают почти к нулю, значит, основная проблема в коротких лагах\n";
    f<<"Out-of-sample (качество прогноза): \n";
    f<<"oos_rmse,"<<rmse<<"\n";
    f<<"oos_mae,"<<mae<<"\n";
    f<<"oos_r2,"<<r2<<"\n";
    f<<"oos_corr,"<<corr<<"\n";
    f<<"RMSE - среднеквадратичная ошибка прогноза, MAE - средняя абсолютная ошибка, corr - если маленькая, предсказанные значения почти не коррелируют с реальными (случайность)\n";
    f.close();
    cout<<"Done. wrote "<<fn<<"\n";
  }

  return 0;
}

