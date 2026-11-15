#pragma once
#include "BusReference.h"
#include <array>
#include <algorithm>
#include <cmath>
// Original dynamics model calibrated from independent input/output measurements.
class BusDynamics final {
public:
 void prepare(double sr) noexcept {rate=sr;window=std::clamp(static_cast<int>(std::lround(sr*.0005)),1,1024);attackIndex=releaseIndex=-1;reset();}
 void reset() noexcept {squares.fill(0);sum=0;position=0;envelope=settle=memory=0;}
 void configure(int ratio,int attack,int release) noexcept {
  ratioIndex=ratio;
  if(attack==attackIndex&&release==releaseIndex)return;
  const int previous=releaseIndex;attackIndex=attack;releaseIndex=release;
  const auto& a=automatic[static_cast<size_t>(attack)];fastFraction=a[0];memoryFraction=a[6];
  attackPole=std::exp(-1.0/(rate*manualAttack[static_cast<size_t>(attack*4+std::min(release,3))]));
  releasePole=std::exp(-1.0/(rate*manualRelease[static_cast<size_t>(std::min(release,3))]));
  autoAttackPole=std::exp(-1.0/(rate*a[1]));
  settlePole=std::exp(-1.0/(rate*a[2]));fastPole=std::exp(-1.0/(rate*a[3]));chargePole=std::exp(-1.0/(rate*a[4]));memoryPole=std::exp(-1.0/(rate*a[5]));
  if(previous>=0&&previous!=release){settle=envelope;memory=release==4?memoryFraction*envelope:0;}
 }
 float tick(float rectified,double threshold) noexcept {
  const double square=static_cast<double>(rectified)*rectified;sum+=square-squares[static_cast<size_t>(position)];squares[static_cast<size_t>(position)]=square;
  if(++position==window){position=0;sum=0;for(int i=0;i<window;++i)sum+=squares[static_cast<size_t>(i)];}
  const double detector=std::max(static_cast<double>(rectified),std::sqrt(std::max(0.0,2*sum/window)));
  const double level=20*std::log10(std::max(1.e-12,detector));const double target=BusReference::reduction(level,threshold,ratioIndex,attackIndex,releaseIndex);
  if(releaseIndex<4)envelope=target+(envelope-target)*(target>envelope?attackPole:releasePole);
  else {
   settle=target+(settle-target)*(target>settle?settlePole:fastPole);
   const double goal=memoryFraction*envelope;memory=goal>memory?goal+(memory-goal)*chargePole:memory*memoryPole;
   const double desired=std::max(fastFraction*target+(1-fastFraction)*settle,memory);
   envelope=desired+(envelope-desired)*(desired>envelope?autoAttackPole:fastPole);
  }
  return static_cast<float>(std::clamp(envelope,0.0,120.0));
 }
private:
 inline static constexpr std::array<double,24> manualAttack{{5.636334901054899e-06, 5.519410989777129e-06, 4.054308721616777e-05, 4.832238621785792e-05, 0.0003345612932642283, 0.000340735123510483, 0.000350007615862518, 0.0003549402121733334, 0.001270999814848509, 0.001303791527593186, 0.001350681909613788, 0.00138336434453153, 0.003794028229061952, 0.003972827123230042, 0.004231655860871778, 0.004424271504432498, 0.009706928607382575, 0.01044617573184417, 0.01162385390786025, 0.01256477227470262, 0.02329437292137156, 0.02639743990321879, 0.03164002755691075, 0.03614692116338955}};
 inline static constexpr std::array<double,4> manualRelease{{0.08415273129816142, 0.1264254093863606, 0.2627303838691636, 0.5633703856801889}};
 inline static constexpr std::array<std::array<double,7>,6> automatic{{
{{0.9883736166029679, 2.334062686432841e-06, 0.7545927140675415, 0.04507845375970326, 0.590008153198559, 5.107953070142422, 0.8830616419533043}},
{{0.9765941042444564, 0.0002718514064111588, 0.747398723018613, 0.04264532607798555, 0.3376648631665314, 5.099654087879893, 0.7711940454429207}},
{{0.9497404809053392, 0.0009117957084432012, 0.7302589580115776, 0.04267099325477557, 0.3310526999142884, 5.099723746286152, 0.7677572593110525}},
{{0.8908688372532, 0.003352697940278945, 0.7852841527354675, 0.04258953768116411, 0.2909566421237441, 5.099461300366298, 0.751901133419699}},
{{0.7993437797968846, 0.008101345973227637, 0.8328876299826166, 0.0446170950953844, 0.5959870098666527, 5.105138061914233, 0.8844340546877013}},
{{0.6275277451067275, 0.01796060919772979, 1.036834596977348, 0.04429087679681498, 0.6030293178463625, 5.105846370166593, 0.8844069049520148}}
 }};
 std::array<double,1024> squares{};
 double rate=44100,sum=0,envelope=0,settle=0,memory=0,attackPole=0,releasePole=0,autoAttackPole=0,settlePole=0,fastPole=0,chargePole=0,memoryPole=0,fastFraction=0,memoryFraction=0;
 int position=0,window=22,ratioIndex=1,attackIndex=-1,releaseIndex=-1;
};
