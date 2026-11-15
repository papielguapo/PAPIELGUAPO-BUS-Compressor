#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
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

 if(argc>1){std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());ed->setSize(kBaseWidth,kBaseHeight);ed->resized();const auto image=ed->createComponentSnapshot(ed->getLocalBounds());juce::File f{juce::String(argv[1])};auto stream=f.createOutputStream();check(stream!=nullptr,"preview file");stream->setPosition(0);stream->truncate();juce::PNGImageFormat png;check(png.writeImageToStream(image,*stream),"preview render");return 0;}
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
 std::cout<<"PASS: slopes 2/4/10, stereo link, mix, bypass, power, all attack/release settings, fade, state; 44.1/48/96 kHz\n";
 return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
