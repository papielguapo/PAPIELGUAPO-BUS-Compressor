#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
unsigned long long checks=0;
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
void set(VCASSLProcessor& p,const char* id,float value){auto* v=p.apvts.getParameter(id);check(v!=nullptr,"parameter exists");v->setValueNotifyingHost(v->convertTo0to1(value));}
void prepare(VCASSLProcessor& p,double sr=48000,int block=128){p.setRateAndBufferSizeDetails(sr,block);p.prepareToPlay(sr,block);}
struct Tone {double rms=0,mean=0;std::array<double,12> harmonic{};float gr=0;};
Tone tone(VCASSLProcessor& p,double sr,double seconds,double frequency,float amplitude,int block=128){
 juce::AudioBuffer<float>b(2,block);juce::MidiBuffer midi;double energy=0,sum=0,re[12]{},im[12]{};int count=0;
 const int total=static_cast<int>(sr*seconds),start=total-static_cast<int>(sr);
 for(int offset=0;offset<total;offset+=block){const int size=juce::jmin(block,total-offset);b.setSize(2,size,false,false,true);
  for(int i=0;i<size;++i){float x=amplitude*static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*frequency*(offset+i)/sr));b.setSample(0,i,x);b.setSample(1,i,x);}
  p.processBlock(b,midi);for(int i=0;i<size;++i){const double x=b.getSample(0,i);check(std::isfinite(x),"tone finite");check(b.getSample(0,i)==b.getSample(1,i),"identical stereo remains identical");if(offset+i>=start){energy+=x*x;sum+=x;++count;const double t=juce::MathConstants<double>::twoPi*frequency*(offset+i)/sr;for(int h=0;h<12;++h){re[h]+=x*std::cos((h+1)*t);im[h]+=x*std::sin((h+1)*t);}}}
 }
 Tone result;result.rms=std::sqrt(energy/count);result.mean=sum/count;for(int h=0;h<12;++h)result.harmonic[static_cast<size_t>(h)]=2*std::hypot(re[h],im[h])/count;result.gr=p.currentGrDb.load();return result;
}
}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI gui;try{
 if(argc>1){VCASSLProcessor p;if(argc>2&&juce::String(argv[2])=="--sc-on")set(p,"scEnabled",1);std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());ed->resized();const auto image=ed->createComponentSnapshot(ed->getLocalBounds());juce::FileOutputStream stream{juce::File{juce::String(argv[1])}};check(stream.openedOk(),"preview stream");stream.setPosition(0);stream.truncate();juce::PNGImageFormat png;check(png.writeImageToStream(image,stream),"preview PNG");return 0;}
 // Clean additive H2/H3 stage, isolated from the VCA detector.
 VCASSLProcessor spectral;set(spectral,"threshold",15);prepare(spectral);const auto ki=tone(spectral,48000,3,1000,.2f);
 const double h2=20*std::log10(ki.harmonic[1]/ki.harmonic[0]),h3=20*std::log10(juce::jmax(1.e-12,ki.harmonic[2]/ki.harmonic[0]));
 std::cout<<"Clean H2/H3 dBc: "<<h2<<" / "<<h3<<"\n";check(std::abs(h2+60)<.25,"clean H2 target -60 dBc");check(std::abs(h3+73.98)<.3,"clean H3 target -73.98 dBc");
 for(int h=3;h<12;++h)check(20*std::log10(juce::jmax(1.e-12,ki.harmonic[static_cast<size_t>(h)]/ki.harmonic[0]))<-110,"no significant H4-H12");check(std::abs(ki.mean)<1.e-6,"harmonic DC blocked");
 spectral.reset();const auto infra=tone(spectral,48000,3,5,.2f);check(std::abs(20*std::log10(infra.harmonic[0]/ki.harmonic[0]))<.02,"no 18 Hz audio filter, bass preserved");
 spectral.reset();const auto alias=tone(spectral,48000,3,18000,.2f);const double alias2=20*std::log10(juce::jmax(1.e-12,alias.harmonic[1]/alias.harmonic[0])),alias3=20*std::log10(juce::jmax(1.e-12,alias.harmonic[2]/alias.harmonic[0]));std::cout<<"Aliasing 18kHz, folded H2/H3 dBc: "<<alias2<<" / "<<alias3<<"\n";check(alias2<-100&&alias3<-100,"antialias H2/H3 below -100 dBc");
 check(spectral.getLatencySamples()==HarmonicStage::latency,"reported latency matches harmonic stage");
 // Regression against independently measured Waves outputs, not nominal ideal-ratio math.
 const std::array<std::array<double,3>,3> golden={{{{-30.4108963,-24.2132899,-18.7243262}},{{-28.0112498,-24.0638024,-20.8617389}},{{-27.0103,-23.6525455,-22.3642038}}}};
 for(int ratio=0;ratio<3;++ratio){VCASSLProcessor p;set(p,"ratio",static_cast<float>(ratio));prepare(p);const float levels[]{-24,-12,0};for(int i=0;i<3;++i){p.reset();const auto actual=tone(p,48000,6,1000,std::pow(10.f,levels[i]/20.f));const double level=20*std::log10(actual.rms);std::cout<<"Static ratio "<<ratio<<", input "<<levels[i]<<": "<<level<<" dBFS\n";check(std::abs(level-golden[static_cast<size_t>(ratio)][static_cast<size_t>(i)])<.15,"calibrated static curve matches Waves reference");}}
 // SC filters detection, never the audio path. 50 Hz drives less GR with SC ON.
 VCASSLProcessor scOff,scOn;set(scOn,"scEnabled",1);prepare(scOff);prepare(scOn);const auto off=tone(scOff,48000,4,50,.5f),on=tone(scOn,48000,4,50,.5f);
 std::cout<<"SC 50Hz GR off/on dB: "<<off.gr<<" / "<<on.gr<<"\n";check(on.gr<off.gr-2,"SC reduces bass-driven compression");
 for(auto* p:{&scOff,&scOn}){set(*p,"threshold",15);p->reset();}const auto idleOff=tone(scOff,48000,3,50,.02f),idleOn=tone(scOn,48000,3,50,.02f);check(idleOff.rms==idleOn.rms&&idleOff.harmonic==idleOn.harmonic,"SC never cuts audio bass when detector is idle");
 // FIX compares the final coloured signal to the original input, including Make-Up and Trim.
 for(double sr:{44100.,96000.})for(float amplitude:{.05f,.5f}){VCASSLProcessor matched,reference;for(auto* p:{&matched,&reference}){set(*p,"makeup",amplitude<.1f?12.f:0.f);set(*p,"trim",3);prepare(*p,sr);}set(matched,"fix",1);
  juce::AudioBuffer<float>a(2,128),b(2,128);juce::MidiBuffer midi;double ei=0,eo=0;
  for(int offset=0;offset<static_cast<int>(sr*12);offset+=128){if(offset>=static_cast<int>(sr*9))set(matched,"fix",0);for(int i=0;i<128;++i){float x=amplitude*static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*997*(offset+i)/sr));for(int ch=0;ch<2;++ch){a.setSample(ch,i,x);b.setSample(ch,i,x);}}
   matched.processBlock(a,midi);reference.processBlock(b,midi);check(matched.currentGrDb.load()==reference.currentGrDb.load(),"FIX must not change detector/GR");
   for(int i=0;i<128;++i){check(std::isfinite(a.getSample(0,i)),"FIX finite");if(offset>=static_cast<int>(sr*7)&&offset<static_cast<int>(sr*9)){const double x=amplitude*std::sin(juce::MathConstants<double>::twoPi*997*(offset+i)/sr),y=a.getSample(0,i);ei+=x*x;eo+=y*y;}if(offset>static_cast<int>(sr*10))check(a.getSample(0,i)==b.getSample(0,i),"FIX off exact base path");}}
  const double error=10*std::log10(eo/ei);std::cout<<"FIX RMS error "<<sr<<" Hz, "<<amplitude<<": "<<error<<" dB\n";check(std::abs(error)<.01,"FIX with clean harmonics within .01 dB");
 }
 // No independent noise source, and no extra gain on an existing very low noise floor.
 {VCASSLProcessor p;set(p,"threshold",15);prepare(p);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer midi;unsigned seed=42;double ei=0,eo=0;
  for(int offset=0;offset<144000;offset+=128){float dry[128]{};for(int i=0;i<128;++i){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;dry[i]=(static_cast<float>(seed)/4294967295.f-.5f)*.000002f;b.setSample(0,i,dry[i]);b.setSample(1,i,dry[i]);}p.processBlock(b,midi);if(offset>=96000)for(int i=0;i<128;++i){ei+=static_cast<double>(dry[i])*dry[i];const double x=b.getSample(0,i);eo+=x*x;}}
  const double change=10*std::log10(eo/ei);std::cout<<"Low-level noise gain: "<<change<<" dB\n";check(change<.05,"no low-level noise boost");p.reset();for(int j=0;j<100;++j){b.clear();p.processBlock(b,midi);check(b.getMagnitude(0,128)==0,"exact silence from rest");}}
 // The dry and host-bypass paths have exactly the reported latency, including split blocks.
 for(bool hostBypass:{false,true}){VCASSLProcessor p;set(p,"mix",0);prepare(p);juce::MidiBuffer midi;int offset=0;
  for(int size:{1,7,31,64,127}){juce::AudioBuffer<float>b(2,size);b.clear();for(int i=0;i<size;++i)for(int c=0;c<2;++c){const int n=offset+i;b.setSample(c,i,n==0?.5f:n==17?.25f:0);}
   if(hostBypass)p.processBlockBypassed(b,midi);else p.processBlock(b,midi);
   for(int i=0;i<size;++i){const int n=offset+i;const float expected=n==64?.5f:n==81?.25f:0;check(b.getSample(0,i)==expected,"dry/bypass latency aligned exactly");}offset+=size;}}
 // The dynamics must reproduce the measured charge and recovery of both release modes.
 {const int times[]{2005,2010,2030,2100,2500,2900,3000,3010,3030,3050,3100,3200,3300,3500,4000,5000,7000,10000};const double reference[2][18]{{5.36567311,7.731155639,10.31043838,10.83075098,11.77748686,12.37899203,12.45426904,11.68495054,10.58414522,9.881795076,9.024165867,8.541407644,8.345895779,8.021857658,7.272694513,5.977772662,4.038568848,2.242640294},{1.970043574,3.403994407,6.89771977,9.753775302,10.03091868,10.03091943,9.993973827,9.236690337,7.889921794,6.739520185,4.54484961,2.066808185,0.9398975074,0.1943751913,0.003780408722,1.668753981e-06,-4.607580451e-08,-4.607580451e-08}};
  for(int mode=0;mode<2;++mode){VCASSLProcessor p;set(p,"attack",mode==0?4.f:5.f);set(p,"release",mode==0?4.f:1.f);prepare(p);std::vector<double> energy(11000);juce::MidiBuffer midi;
   for(int offset=0;offset<528000+64;offset+=128){const int count=std::min(128,528000+64-offset);juce::AudioBuffer<float>b(2,count);for(int i=0;i<count;++i){const int pos=offset+i;const float amp=pos>=96000&&pos<144000?std::pow(10.f,-6.f/20):std::pow(10.f,-36.f/20);const float x=pos<528000?amp*static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*1000*pos/48000.)):0;for(int c=0;c<2;++c)b.setSample(c,i,x);}p.processBlock(b,midi);for(int i=0;i<count;++i){const int pos=offset+i-64;if(pos>=0&&pos<528000){const double x=b.getSample(0,i);energy[static_cast<size_t>(pos/48)]+=x*x/48;}}}
   double worst=0;for(int i=0;i<18;++i){const int t=times[i];const double in=(t>=2000&&t<3000?-6.:-36.)-3.01029995664;const double gr=in-10*std::log10(energy[static_cast<size_t>(t)]);worst=std::max(worst,std::abs(gr-reference[mode][i]));check(std::abs(gr-reference[mode][i])<.5,"measured Waves attack/release regression");}std::cout<<"Transient reference max selected error, mode "<<mode<<": "<<worst<<" dB\n";
  }}
 // Host automation, unusual blocks, high rates, mono/stereo, reset and bypass.
 for(int channels:{1,2})for(double sr:{8000.,48000.,192000.,384000.})for(int flags=0;flags<4;++flags){VCASSLProcessor p;auto layout=p.getBusesLayout();layout.inputBuses.getReference(0)=channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();layout.outputBuses.getReference(0)=layout.inputBuses[0];check(p.setBusesLayout(layout),"mono/stereo layout");set(p,"scEnabled",static_cast<float>(flags&1));set(p,"fix",static_cast<float>((flags>>1)&1));prepare(p,sr,1024);juce::MidiBuffer midi;
  for(int size:{0,1,7,31,127,511,1024}){juce::AudioBuffer<float>b(channels,size);for(int c=0;c<channels;++c)for(int i=0;i<size;++i)b.setSample(c,i,.3f*static_cast<float>(std::sin((i+1)*.17)));p.processBlock(b,midi);for(int c=0;c<channels;++c)for(int i=0;i<size;++i)check(std::isfinite(b.getSample(c,i)),"arbitrary blocks finite");p.processBlockBypassed(b,midi);check(std::isfinite(p.currentOutDb.load()),"zero-size bypass meter finite");}
  p.reset();juce::AudioBuffer<float>b(channels,127);b.clear();p.processBlock(b,midi);check(b.getMagnitude(0,127)==0,"reset silence");}
 {VCASSLProcessor p;set(p,"power",0);set(p,"fix",1);set(p,"scEnabled",1);prepare(p);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer midi;for(int i=0;i<128;++i){b.setSample(0,i,.2f);b.setSample(1,i,-.1f);}p.processBlock(b,midi);check(b.getSample(0,127)==.2f&&b.getSample(1,127)==-.1f,"POWER dry exact");set(p,"power",1);set(p,"bypass",1);p.reset();for(int i=0;i<128;++i){b.setSample(0,i,.2f);b.setSample(1,i,-.1f);}p.processBlock(b,midi);check(b.getSample(0,127)==.2f,"host bypass parameter dry exact");}
 // The legacy KI switch is ignored: the integrated branch stays on even for old states.
 {VCASSLProcessor a,b;set(a,"threshold",15);set(b,"threshold",15);set(a,"ki",0);set(b,"ki",1);prepare(a);prepare(b);const auto x=tone(a,48000,2,1000,.2f),y=tone(b,48000,2,1000,.2f);check(x.rms==y.rms&&x.harmonic==y.harmonic,"Legacy KI toggle has no effect on new harmonic stage");set(a,"scEnabled",1);set(a,"fix",1);juce::MemoryBlock state;a.getStateInformation(state);VCASSLProcessor loaded;loaded.setStateInformation(state.getData(),static_cast<int>(state.getSize()));check(loaded.apvts.getRawParameterValue("scEnabled")->load()==1&&loaded.apvts.getRawParameterValue("fix")->load()==1,"state SC/FIX persistence");auto old=a.apvts.copyState();old.removeChild(old.getChildWithProperty("id","scEnabled"),nullptr);auto xml=old.createXml();juce::AudioProcessor::copyXmlToBinary(*xml,state);loaded.setStateInformation(state.getData(),static_cast<int>(state.getSize()));check(loaded.apvts.getRawParameterValue("scEnabled")->load()==0,"legacy state SC defaults off");}
 // Exceptional host audio never poisons filters/meters, even with FIX enabled.
 {VCASSLProcessor p;set(p,"fix",1);set(p,"makeup",15);set(p,"trim",18);prepare(p,192000);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer midi;const float hostile[]{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};for(float x:hostile){for(int c=0;c<2;++c)for(int i=0;i<128;++i)b.setSample(c,i,x);p.processBlock(b,midi);for(int c=0;c<2;++c)for(int i=0;i<128;++i)check(std::isfinite(b.getSample(c,i)),"hostile audio finite");check(std::isfinite(p.currentOutDb.load()),"hostile meters finite");}for(int j=0;j<1000;++j){b.clear();p.processBlock(b,midi);check(std::isfinite(b.getSample(0,127)),"hostile recovery");}}
 // The inherited external sidechain still routes its own detector signal.
 {VCASSLProcessor p;auto layout=p.getBusesLayout();layout.inputBuses.getReference(1)=juce::AudioChannelSet::stereo();check(p.setBusesLayout(layout),"external layout");prepare(p);juce::AudioBuffer<float>b(4,128);juce::MidiBuffer midi;
  auto drive=[&](float sc){p.reset();for(int offset=0;offset<144000;offset+=128){for(int i=0;i<128;++i){const float sine=static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*1000*(offset+i)/48000));b.setSample(0,i,sine*.1f);b.setSample(1,i,sine*.1f);b.setSample(2,i,sine*sc);b.setSample(3,i,sine*sc);}p.processBlock(b,midi);}return p.currentGrDb.load();};check(drive(0)<.001f,"silent external SC no compression");check(drive(.9f)>8,"automatic external SC drives compression without UI/legacy toggle");set(p,"sidechainSource",1);check(drive(0)>1,"host parameter can force Internal with auxiliary bus active");set(p,"sidechainSource",2);p.reset();for(int j=0;j<100;++j){b.clear();for(int i=0;i<128;++i){b.setSample(2,i,.5f);b.setSample(3,i,.5f);}p.processBlock(b,midi);check(b.getMagnitude(0,0,128)==0&&b.getMagnitude(1,0,128)==0,"sidechain audio never leaks to output");}}
 // Every attack/release selection remains stable under abrupt signal changes.
 for(int a=0;a<6;++a)for(int r=0;r<5;++r){VCASSLProcessor p;set(p,"attack",static_cast<float>(a));set(p,"release",static_cast<float>(r));prepare(p);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer midi;for(int j=0;j<20;++j){for(int c=0;c<2;++c)for(int i=0;i<128;++i)b.setSample(c,i,j<10?.7f:0);p.processBlock(b,midi);check(std::isfinite(b.getSample(0,127)),"all attack/release settings stable");}}
 std::cout<<"PASS: "<<checks<<" checks; clean H2/H3 antialiased, SC detector-only, FIX, noise, states, buses, hostile audio and automation\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
