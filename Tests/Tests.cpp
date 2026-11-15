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
struct Tone {double rms=0,mean=0;std::array<double,3> harmonic{};float gr=0;};
Tone tone(VCASSLProcessor& p,double sr,double seconds,double frequency,float amplitude,int block=128){
 juce::AudioBuffer<float>b(2,block);juce::MidiBuffer midi;double energy=0,sum=0,re[3]{},im[3]{};int count=0;
 const int total=static_cast<int>(sr*seconds),start=total-static_cast<int>(sr);
 for(int offset=0;offset<total;offset+=block){const int size=juce::jmin(block,total-offset);b.setSize(2,size,false,false,true);
  for(int i=0;i<size;++i){float x=amplitude*static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*frequency*(offset+i)/sr));b.setSample(0,i,x);b.setSample(1,i,x);}
  p.processBlock(b,midi);for(int i=0;i<size;++i){const double x=b.getSample(0,i);check(std::isfinite(x),"tone finite");check(b.getSample(0,i)==b.getSample(1,i),"identical stereo remains identical");if(offset+i>=start){energy+=x*x;sum+=x;++count;const double t=juce::MathConstants<double>::twoPi*frequency*(offset+i)/sr;for(int h=0;h<3;++h){re[h]+=x*std::cos((h+1)*t);im[h]+=x*std::sin((h+1)*t);}}}
 }
 Tone result;result.rms=std::sqrt(energy/count);result.mean=sum/count;for(int h=0;h<3;++h)result.harmonic[static_cast<size_t>(h)]=2*std::hypot(re[h],im[h])/count;result.gr=p.currentGrDb.load();return result;
}
}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI gui;try{
 if(argc>1){VCASSLProcessor p;if(argc>2&&juce::String(argv[2])=="--sc-on")set(p,"scEnabled",1);std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());ed->resized();const auto image=ed->createComponentSnapshot(ed->getLocalBounds());juce::FileOutputStream stream{juce::File{juce::String(argv[1])}};check(stream.openedOk(),"preview stream");stream.setPosition(0);stream.truncate();juce::PNGImageFormat png;check(png.writeImageToStream(image,stream),"preview PNG");return 0;}
 // Integrated KI is +25 dB in H2, without changing the VCA detector.
 VCASSLProcessor spectral;set(spectral,"threshold",15);prepare(spectral);const auto ki=tone(spectral,48000,3,1000,.2f);
 const double h2=20*std::log10(ki.harmonic[1]/ki.harmonic[0]),h3=20*std::log10(juce::jmax(1.e-12,ki.harmonic[2]/ki.harmonic[0]));
 std::cout<<"Integrated KI H2/H3 dBc: "<<h2<<" / "<<h3<<"\n";check(std::abs(h2-(-62.0248+25))<.1,"KI H2 +25 dB");check(h2>h3+20,"KI even harmonics dominant");check(std::abs(ki.mean)<1.e-6,"KI DC blocked");
 spectral.reset();const auto infra=tone(spectral,48000,3,5,.2f);check(20*std::log10(infra.harmonic[0]/ki.harmonic[0])<-40,"KI infra filter 18 Hz");
 // Compression slopes remain correct with the integrated downstream colour.
 for(int ratio=0;ratio<3;++ratio){VCASSLProcessor p;set(p,"threshold",-15);set(p,"ratio",static_cast<float>(ratio));prepare(p);const auto a=tone(p,48000,4,1000,.1f);p.reset();const auto b=tone(p,48000,4,1000,.2f);const double measured=20*std::log10(b.rms/a.rms),expected=20*std::log10(2.)/(ratio==0?2:ratio==1?4:10);std::cout<<"Ratio slope "<<ratio<<": "<<measured<<" dB\n";check(std::abs(measured-expected)<.1,"ratio slope");}
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
  const double error=10*std::log10(eo/ei);std::cout<<"FIX RMS error "<<sr<<" Hz, "<<amplitude<<": "<<error<<" dB\n";check(std::abs(error)<.01,"FIX with KI +25dB within .01 dB");
 }
 // No independent noise source, and no extra gain on an existing very low noise floor.
 {VCASSLProcessor p;set(p,"threshold",15);prepare(p);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer midi;unsigned seed=42;double ei=0,eo=0;
  for(int offset=0;offset<144000;offset+=128){float dry[128]{};for(int i=0;i<128;++i){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;dry[i]=(static_cast<float>(seed)/4294967295.f-.5f)*.000002f;b.setSample(0,i,dry[i]);b.setSample(1,i,dry[i]);}p.processBlock(b,midi);if(offset>=96000)for(int i=0;i<128;++i){ei+=static_cast<double>(dry[i])*dry[i];const double x=b.getSample(0,i);eo+=x*x;}}
  const double change=10*std::log10(eo/ei);std::cout<<"Low-level noise gain: "<<change<<" dB\n";check(change<.05,"no low-level noise boost");p.reset();for(int j=0;j<100;++j){b.clear();p.processBlock(b,midi);check(b.getMagnitude(0,128)==0,"exact silence from rest");}}
 // Host automation, unusual blocks, high rates, mono/stereo, reset and bypass.
 for(int channels:{1,2})for(double sr:{8000.,48000.,192000.,384000.})for(int flags=0;flags<4;++flags){VCASSLProcessor p;auto layout=p.getBusesLayout();layout.inputBuses.getReference(0)=channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();layout.outputBuses.getReference(0)=layout.inputBuses[0];check(p.setBusesLayout(layout),"mono/stereo layout");set(p,"scEnabled",static_cast<float>(flags&1));set(p,"fix",static_cast<float>((flags>>1)&1));prepare(p,sr,1024);juce::MidiBuffer midi;
  for(int size:{0,1,7,31,127,511,1024}){juce::AudioBuffer<float>b(channels,size);for(int c=0;c<channels;++c)for(int i=0;i<size;++i)b.setSample(c,i,.3f*static_cast<float>(std::sin((i+1)*.17)));p.processBlock(b,midi);for(int c=0;c<channels;++c)for(int i=0;i<size;++i)check(std::isfinite(b.getSample(c,i)),"arbitrary blocks finite");p.processBlockBypassed(b,midi);check(std::isfinite(p.currentOutDb.load()),"zero-size bypass meter finite");}
  p.reset();juce::AudioBuffer<float>b(channels,127);b.clear();p.processBlock(b,midi);check(b.getMagnitude(0,127)==0,"reset silence");}
 {VCASSLProcessor p;set(p,"power",0);set(p,"fix",1);set(p,"scEnabled",1);prepare(p);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer midi;for(int i=0;i<128;++i){b.setSample(0,i,.2f);b.setSample(1,i,-.1f);}p.processBlock(b,midi);check(b.getSample(0,127)==.2f&&b.getSample(1,127)==-.1f,"POWER dry exact");set(p,"power",1);set(p,"bypass",1);p.reset();p.processBlock(b,midi);check(b.getSample(0,127)==.2f,"host bypass parameter dry exact");}
 // The legacy KI switch is ignored: the integrated branch stays on even for old states.
 {VCASSLProcessor a,b;set(a,"threshold",15);set(b,"threshold",15);set(a,"ki",0);set(b,"ki",1);prepare(a);prepare(b);const auto x=tone(a,48000,2,1000,.2f),y=tone(b,48000,2,1000,.2f);check(x.rms==y.rms&&x.harmonic==y.harmonic,"KI always integrated regardless of legacy toggle");set(a,"scEnabled",1);set(a,"fix",1);juce::MemoryBlock state;a.getStateInformation(state);VCASSLProcessor loaded;loaded.setStateInformation(state.getData(),static_cast<int>(state.getSize()));check(loaded.apvts.getRawParameterValue("scEnabled")->load()==1&&loaded.apvts.getRawParameterValue("fix")->load()==1,"state SC/FIX persistence");auto old=a.apvts.copyState();old.removeChild(old.getChildWithProperty("id","scEnabled"),nullptr);auto xml=old.createXml();juce::AudioProcessor::copyXmlToBinary(*xml,state);loaded.setStateInformation(state.getData(),static_cast<int>(state.getSize()));check(loaded.apvts.getRawParameterValue("scEnabled")->load()==0,"legacy state SC defaults off");}
 // Exceptional host audio never poisons filters/meters, even with FIX enabled.
 {VCASSLProcessor p;set(p,"fix",1);set(p,"makeup",15);set(p,"trim",18);prepare(p,192000);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer midi;const float hostile[]{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};for(float x:hostile){for(int c=0;c<2;++c)for(int i=0;i<128;++i)b.setSample(c,i,x);p.processBlock(b,midi);for(int c=0;c<2;++c)for(int i=0;i<128;++i)check(std::isfinite(b.getSample(c,i)),"hostile audio finite");check(std::isfinite(p.currentOutDb.load()),"hostile meters finite");}for(int j=0;j<1000;++j){b.clear();p.processBlock(b,midi);check(std::isfinite(b.getSample(0,127)),"hostile recovery");}}
 // The inherited external sidechain still routes its own detector signal.
 {VCASSLProcessor p;auto layout=p.getBusesLayout();layout.inputBuses.getReference(1)=juce::AudioChannelSet::stereo();check(p.setBusesLayout(layout),"external layout");set(p,"externalSC",1);prepare(p);juce::AudioBuffer<float>b(4,128);juce::MidiBuffer midi;
  auto drive=[&](float sc){p.reset();for(int offset=0;offset<144000;offset+=128){for(int i=0;i<128;++i){const float sine=static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*1000*(offset+i)/48000));b.setSample(0,i,sine*.1f);b.setSample(1,i,sine*.1f);b.setSample(2,i,sine*sc);b.setSample(3,i,sine*sc);}p.processBlock(b,midi);}return p.currentGrDb.load();};check(drive(0)<.001f,"silent external SC no compression");check(drive(.9f)>8,"loud external SC drives compression");}
 // Every attack/release selection remains stable under abrupt signal changes.
 for(int a=0;a<6;++a)for(int r=0;r<5;++r){VCASSLProcessor p;set(p,"attack",static_cast<float>(a));set(p,"release",static_cast<float>(r));prepare(p);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer midi;for(int j=0;j<20;++j){for(int c=0;c<2;++c)for(int i=0;i<128;++i)b.setSample(c,i,j<10?.7f:0);p.processBlock(b,midi);check(std::isfinite(b.getSample(0,127)),"all attack/release settings stable");}}
 std::cout<<"PASS: "<<checks<<" checks; integrated KI +25dB, SC detector-only, FIX, noise, states, buses, hostile audio and automation\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
