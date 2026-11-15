#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <limits>
void check(bool x,const char* m){if(!x)throw std::runtime_error(m);}
void set(VCASSLProcessor& p,const char* id,float x){auto* v=p.apvts.getParameter(id);v->setValueNotifyingHost(v->convertTo0to1(x));}
float run(VCASSLProcessor& p,float level,int blocks=3000){juce::AudioBuffer<float>b(2,128);juce::MidiBuffer m;float last=0;
for(int j=0;j<blocks;++j){b.clear();for(int i=0;i<128;++i){b.setSample(0,i,level);b.setSample(1,i,level*.5f);}p.processBlock(b,m);for(int c=0;c<2;++c)for(int i=0;i<128;++i)check(std::isfinite(b.getSample(c,i)),"finite");last=b.getSample(0,127);if(std::abs(last)>1.e-10f)check(std::abs(b.getSample(1,127)/last-.5f)<1.e-5f,"stereo link");}return last;}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI gui;try{
 VCASSLProcessor p;
 if(argc>1&&juce::String(argv[1])=="--measure"){set(p,"attack",4);set(p,"release",4);set(p,"threshold",0);set(p,"ratio",1);p.setRateAndBufferSizeDetails(48000,128);p.prepareToPlay(48000,128);
 for(float level:{-48.f,-36.f,-30.f,-24.f,-18.f,-12.f,-6.f,0.f}){p.reset();double energy=0;int count=0;juce::AudioBuffer<float>b(2,128);juce::MidiBuffer m;
 for(int offset=0;offset<144000;offset+=128){for(int i=0;i<128;++i){float x=std::pow(10.f,level/20.f)*static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*1000*(offset+i)/48000.));b.setSample(0,i,x);b.setSample(1,i,x);}p.processBlock(b,m);if(offset>96000)for(int i=0;i<128;++i){double x=b.getSample(0,i);energy+=x*x;++count;}}
 std::cout<<"TRANSFER,"<<level<<","<<10*std::log10(energy/count)<<"\n";}return 0;}

 if(argc>1){if(argc>2&&juce::String(argv[2])=="--ki-on")set(p,"ki",1);std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());ed->setSize(kBaseWidth,kBaseHeight);ed->resized();const auto image=ed->createComponentSnapshot(ed->getLocalBounds());juce::File f{juce::String(argv[1])};auto stream=f.createOutputStream();check(stream!=nullptr,"preview file");stream->setPosition(0);stream->truncate();juce::PNGImageFormat png;check(png.writeImageToStream(image,*stream),"preview render");return 0;}
 for(double sr:{44100.,48000.,96000.}){p.setRateAndBufferSizeDetails(sr,128);set(p,"threshold",-15);set(p,"attack",0);set(p,"release",0);p.prepareToPlay(sr,128);
 for(int r=0;r<3;++r){set(p,"ratio",static_cast<float>(r));p.reset();float a=run(p,.1f);p.reset();float b=run(p,.2f);float slope=20*std::log10(b/a);float expected=20*std::log10(2.f)/(r==0?2:r==1?4:10);check(std::abs(slope-expected)<.04f,"ratio slope");}
 set(p,"mix",0);p.reset();check(std::abs(run(p,.25f)-.25f)<1.e-7f,"dry mix");set(p,"mix",100);
 set(p,"bypass",1);p.reset();check(run(p,.2f)==.2f,"bypass exact");set(p,"bypass",0);
 set(p,"power",0);p.reset();check(run(p,.3f)==.3f,"power exact");set(p,"power",1);
 for(int a=0;a<6;++a)for(int r=0;r<5;++r){set(p,"attack",static_cast<float>(a));set(p,"release",static_cast<float>(r));p.reset();run(p,.7f,40);}
 set(p,"fadeRate",1);set(p,"fade",1);p.reset();check(std::abs(run(p,.5f,500))<1.e-8f,"fade out");set(p,"fade",0);check(run(p,.5f,900)>0,"fade in");
 }
 // FIX must match RMS while leaving the detector/GR unchanged and preserving stereo balance.
 for(double sr:{44100.,96000.})for(float amplitude:{.05f,.5f}){
  VCASSLProcessor matched,reference;for(auto* q:{&matched,&reference}){q->setRateAndBufferSizeDetails(sr,128);set(*q,"makeup",amplitude<.1f?12.f:0.f);set(*q,"trim",3.f);q->prepareToPlay(sr,128);}set(matched,"fix",1);
  juce::AudioBuffer<float>a(2,128),b(2,128);juce::MidiBuffer m;double inputSum=0,outputSum=0;int count=0;
  for(int offset=0;offset<static_cast<int>(sr*12);offset+=128){if(offset>=static_cast<int>(sr*9))set(matched,"fix",0);
   for(int i=0;i<128;++i){float x=amplitude*static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*997*(offset+i)/sr));a.setSample(0,i,x);a.setSample(1,i,x*.5f);b.setSample(0,i,x);b.setSample(1,i,x*.5f);}
   matched.processBlock(a,m);reference.processBlock(b,m);check(std::abs(matched.currentGrDb.load()-reference.currentGrDb.load())<.00001f,"FIX must not change compression");
   for(int i=0;i<128;++i){const float x=a.getSample(0,i);check(std::isfinite(x),"FIX finite");check(std::abs(a.getSample(1,i)-x*.5f)<.000001f,"FIX stereo balance");
    if(offset>=static_cast<int>(sr*7)&&offset<static_cast<int>(sr*9)){double original=amplitude*std::sin(juce::MathConstants<double>::twoPi*997*(offset+i)/sr);inputSum+=original*original;outputSum+=static_cast<double>(x)*x;++count;}
    if(offset>static_cast<int>(sr*10))check(x==b.getSample(0,i),"FIX off restores exact original DSP");}
  }
  check(count>0,"FIX measurement samples");const double error=10*std::log10(outputSum/inputSum);std::cout<<"FIX RMS error dB at "<<sr<<" Hz, amplitude "<<amplitude<<": "<<error<<"\n";check(std::abs(error)<.1,"FIX RMS match within 0.1 dB");
  set(matched,"fix",1);juce::MemoryBlock state;matched.getStateInformation(state);VCASSLProcessor loaded;loaded.setStateInformation(state.getData(),static_cast<int>(state.getSize()));check(loaded.apvts.getRawParameterValue("fix")->load()>.5f,"FIX state persistence");
  for(int j=0;j<100;++j){a.clear();matched.processBlock(a,m);check(a.getMagnitude(0,128)==0,"FIX adds no noise in silence");}
  set(matched,"power",0);matched.reset();for(int i=0;i<128;++i){a.setSample(0,i,.2f);a.setSample(1,i,.1f);}matched.processBlock(a,m);check(a.getSample(0,127)==.2f,"FIX respects Power off");
 }
 // KI reuses the PWM even transfer: predominantly H2, DC blocked, exact clean path when off.
 auto kiSpectrum=[&](bool active,double frequency,bool levelMatch=false){VCASSLProcessor q;set(q,"ki",active?1.f:0.f);set(q,"fix",levelMatch?1.f:0.f);set(q,"threshold",15);q.setRateAndBufferSizeDetails(48000,128);q.prepareToPlay(48000,128);
  juce::AudioBuffer<float>b(2,128);juce::MidiBuffer m;double re[3]{},im[3]{},sum=0,energy=0;int count=0;
  for(int offset=0;offset<48000*9;offset+=128){for(int i=0;i<128;++i){float x=.2f*static_cast<float>(std::sin(juce::MathConstants<double>::twoPi*frequency*(offset+i)/48000));b.setSample(0,i,x);b.setSample(1,i,x);}q.processBlock(b,m);
   if(offset>=48000*8)for(int i=0;i<128&&offset+i<48000*9;++i){const double x=b.getSample(0,i),t=juce::MathConstants<double>::twoPi*frequency*(offset+i)/48000;sum+=x;energy+=x*x;++count;for(int h=0;h<3;++h){re[h]+=x*std::cos((h+1)*t);im[h]+=x*std::sin((h+1)*t);}}}
  std::array<double,5> result{};for(int h=0;h<3;++h)result[static_cast<size_t>(h)]=2*std::hypot(re[h],im[h])/count;result[3]=sum/count;result[4]=std::sqrt(energy/count);return result;};
 const auto cleanKi=kiSpectrum(false,1000),activeKi=kiSpectrum(true,1000),infraKi=kiSpectrum(true,5),kiWithFix=kiSpectrum(true,1000,true);
 const double h2=20*std::log10(activeKi[1]/activeKi[0]),h3=20*std::log10(juce::jmax(1.e-12,activeKi[2]/activeKi[0]));
 std::cout<<"KI H2/H3 dBc: "<<h2<<" / "<<h3<<"\n";
 check(h2>-65&&h2<-59&&h2>h3+20,"KI produces gentle predominantly second harmonic");
 check(activeKi[1]>cleanKi[1]*100,"KI H2 differs from clean path");check(std::abs(activeKi[3])<.000001,"KI DC blocked");
 check(20*std::log10(infraKi[0]/activeKi[0])<-40,"KI 18Hz fourth-order infra filter");
 check(std::abs(20*std::log10(kiWithFix[4]/(.2/std::sqrt(2.))))<.1,"FIX RMS matching works with KI");
 {VCASSLProcessor q;set(q,"ki",1);q.setRateAndBufferSizeDetails(48000,128);q.prepareToPlay(48000,128);juce::AudioBuffer<float>b(2,128);b.clear();juce::MidiBuffer m;q.processBlock(b,m);check(b.getMagnitude(0,128)==0,"KI adds no noise at rest");juce::MemoryBlock state;q.getStateInformation(state);VCASSLProcessor loaded;loaded.setStateInformation(state.getData(),static_cast<int>(state.getSize()));check(loaded.apvts.getRawParameterValue("ki")->load()>.5f,"KI state persistence");}
 // With no analog stage, digital silence remains exact.
 {VCASSLProcessor q;q.setRateAndBufferSizeDetails(48000,128);q.prepareToPlay(48000,128);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer m;
 for(int j=0;j<500;++j){b.clear();q.processBlock(b,m);check(b.getMagnitude(0,128)==0,"digital silence, no added noise");}}
 // Attack timing must distinguish the endpoints on an identical level step.
 auto attackAt=[&](int setting){VCASSLProcessor q;set(q,"threshold",-15);set(q,"attack",static_cast<float>(setting));q.setRateAndBufferSizeDetails(48000,48);q.prepareToPlay(48000,48);juce::AudioBuffer<float>b(2,48);juce::MidiBuffer m;for(int c=0;c<2;++c)for(int i=0;i<48;++i)b.setSample(c,i,.5f);q.processBlock(b,m);return b.getSample(0,47);};
 check(attackAt(0)<attackAt(5)*.8f,"attack timing");
 // External detector: silence must not compress the main input, loud SC must.
 {VCASSLProcessor q;auto layout=q.getBusesLayout();layout.inputBuses.getReference(1)=juce::AudioChannelSet::stereo();check(q.setBusesLayout(layout),"external layout");set(q,"externalSC",1);set(q,"threshold",-15);q.setRateAndBufferSizeDetails(48000,128);q.prepareToPlay(48000,128);juce::AudioBuffer<float>b(4,128);juce::MidiBuffer m;
 auto scRun=[&](float sc){for(int j=0;j<1500;++j){for(int i=0;i<128;++i){b.setSample(0,i,.1f);b.setSample(1,i,.1f);b.setSample(2,i,sc);b.setSample(3,i,sc);}q.processBlock(b,m);}return b.getSample(0,127);};
 check(std::abs(scRun(0)-.1f)<1.e-6f,"silent external SC");check(scRun(.9f)<.03f,"external SC compression");}
 // Mono host layout.
 {VCASSLProcessor q;auto layout=q.getBusesLayout();layout.inputBuses.getReference(0)=juce::AudioChannelSet::mono();layout.outputBuses.getReference(0)=juce::AudioChannelSet::mono();check(q.setBusesLayout(layout),"mono layout");q.setRateAndBufferSizeDetails(48000,128);q.prepareToPlay(48000,128);juce::AudioBuffer<float>b(1,128);b.clear();juce::MidiBuffer m;q.processBlock(b,m);check(std::isfinite(b.getSample(0,0)),"mono process");}
 set(p,"threshold",-7.3f);juce::MemoryBlock state;p.getStateInformation(state);VCASSLProcessor restored;restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));check(std::abs(restored.apvts.getRawParameterValue("threshold")->load()+7.3f)<.001f,"state");
 // Audit regression: a legacy state without new controls must restore defaults, not inherit last toggles.
 {VCASSLProcessor legacy;auto tree=legacy.apvts.copyState();for(const char* id:{"fix","ki"})tree.removeChild(tree.getChildWithProperty("id",id),nullptr);
  juce::MemoryBlock old;auto xml=tree.createXml();juce::AudioProcessor::copyXmlToBinary(*xml,old);VCASSLProcessor q;set(q,"fix",1);set(q,"ki",1);q.setStateInformation(old.getData(),static_cast<int>(old.getSize()));
  check(q.apvts.getRawParameterValue("fix")->load()==0&&q.apvts.getRawParameterValue("ki")->load()==0,"legacy state must reset absent FIX and KI to defaults");}
 // Audit regression: invalid/exceptional host audio must not poison KI/filter state or meters.
 for(bool coloured:{false,true}){VCASSLProcessor q;set(q,"ki",coloured?1.f:0.f);set(q,"fix",1);set(q,"makeup",15);set(q,"trim",18);q.setRateAndBufferSizeDetails(192000,128);q.prepareToPlay(192000,128);juce::AudioBuffer<float>b(2,128);juce::MidiBuffer m;
  const float hostile[]{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};
  for(float x:hostile){for(int c=0;c<2;++c)for(int i=0;i<128;++i)b.setSample(c,i,x);q.processBlock(b,m);for(int c=0;c<2;++c)for(int i=0;i<128;++i)check(std::isfinite(b.getSample(c,i)),"hostile input must not poison output");check(std::isfinite(q.currentOutDb.load()),"hostile input must not poison meters");}
  for(int j=0;j<1000;++j){b.clear();q.processBlock(b,m);check(std::isfinite(b.getSample(0,127)),"recovery after hostile input");}}
 // Audit: arbitrary host block lengths (including zero), mono and stereo, and all KI/FIX combinations.
 for(int channels:{1,2})for(double sr:{8000.,192000.,384000.})for(int flags=0;flags<4;++flags){VCASSLProcessor q;auto layout=q.getBusesLayout();layout.inputBuses.getReference(0)=channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();layout.outputBuses.getReference(0)=layout.inputBuses[0];check(q.setBusesLayout(layout),"audit layout");set(q,"ki",static_cast<float>(flags&1));set(q,"fix",static_cast<float>((flags>>1)&1));q.setRateAndBufferSizeDetails(sr,1024);q.prepareToPlay(sr,1024);juce::MidiBuffer m;
  for(int size:{0,1,7,31,127,511,1024}){juce::AudioBuffer<float>b(channels,size);for(int c=0;c<channels;++c)for(int i=0;i<size;++i)b.setSample(c,i,.3f*static_cast<float>(std::sin((i+1)*.17)));q.processBlock(b,m);for(int c=0;c<channels;++c)for(int i=0;i<size;++i)check(std::isfinite(b.getSample(c,i)),"arbitrary blocks finite");q.processBlockBypassed(b,m);check(std::isfinite(q.currentOutDb.load()),"zero/arbitrary bypass meter finite");}
  q.reset();juce::AudioBuffer<float>silence(channels,127);silence.clear();q.processBlock(silence,m);check(silence.getMagnitude(0,127)==0,"reset restores silence");}
 std::cout<<"PASS: slopes 2/4/10, stereo link, mix, bypass, power, all attack/release settings, fade, state; 44.1/48/96 kHz\n";
 return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
