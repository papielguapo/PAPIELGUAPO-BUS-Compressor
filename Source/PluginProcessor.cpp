#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
float safeAudio(float x) noexcept { return std::isfinite(x)?juce::jlimit(-1.e6f,1.e6f,x):0.f; }
float gain(float x) { return std::pow(10.f,x/20.f); }
float db(float x) { return 20.f*std::log10(juce::jmax(x,1.e-9f)); }
}
juce::AudioProcessorValueTreeState::ParameterLayout VCASSLProcessor::makeParameters() {
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def,float step=0.01f) {
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,step),def)); };
 auto b=[&](const char* id,const char* name,bool def) { p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},name,def)); };
 auto c=[&](const char* id,const char* name,juce::StringArray values,int def) {p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,values,def));};
 b("power","Power",true); f("threshold","Threshold (dBu)",-15,15,0); f("makeup","Make-Up (dB)",-5,15,0);
 c("attack","Attack",{"0.1 ms","0.3 ms","1 ms","3 ms","10 ms","30 ms"},4);
 c("release","Release",{"0.1 s","0.3 s","0.6 s","1.2 s","Auto"},4);
 c("ratio","Ratio",{"2:1","4:1","10:1"},1);
 f("scFilter","SC Filter (0=Off, 21-500 Hz)",0,500,0,1); f("mix","Mix (%)",0,100,100,0.1f);
 f("trim","Trim (dB)",-18,18,0); f("fadeRate","Fade duration (s)",1,60,10,0.1f);
 b("fade","Fade Out",false); b("externalSC","Legacy External SC (automatic)",false);
 b("meterMode","Meter Output",false); b("bypass","Bypass",false); b("fix","FIX Level Match",false); b("ki","Legacy KI (unused)",true); b("scEnabled","SC Filter 80 Hz",false); c("sidechainSource","Sidechain Source",{"Auto","Internal","External"},0); return p;
}
VCASSLProcessor::VCASSLProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true).withInput("Sidechain",juce::AudioChannelSet::stereo(),false)),apvts(*this,nullptr,"VCASSL",makeParameters()) {
 setLatencySamples(HarmonicStage::latency);
 power=apvts.getRawParameterValue("power");
 threshold=apvts.getRawParameterValue("threshold");
 makeup=apvts.getRawParameterValue("makeup");
 attack=apvts.getRawParameterValue("attack");
 release=apvts.getRawParameterValue("release");
 ratio=apvts.getRawParameterValue("ratio");
 bypass=apvts.getRawParameterValue("bypass");
 scFilter=apvts.getRawParameterValue("scFilter");
 mix=apvts.getRawParameterValue("mix");
 trim=apvts.getRawParameterValue("trim");
 fade=apvts.getRawParameterValue("fade");
 fadeRate=apvts.getRawParameterValue("fadeRate");
 externalSC=apvts.getRawParameterValue("externalSC");
 fix=apvts.getRawParameterValue("fix");
 scEnabled=apvts.getRawParameterValue("scEnabled");
 sidechainSource=apvts.getRawParameterValue("sidechainSource");
}
bool VCASSLProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
 const auto main=l.getMainInputChannelSet(); const auto sc=l.getChannelSet(true,1);
 return (main==juce::AudioChannelSet::mono()||main==juce::AudioChannelSet::stereo())&&main==l.getMainOutputChannelSet()
 &&(sc.isDisabled()||sc==juce::AudioChannelSet::mono()||sc==juce::AudioChannelSet::stereo());
}
void VCASSLProcessor::prepareToPlay(double sr,int) {
 rate=juce::jmax(8000.0,sr);
 for(auto* s:{&thresholdDb,&makeupGain,&trimGain,&wet,&enabled,&scHz}) s->reset(rate,0.02);
 dynamics.prepare(rate);
 for(auto& h:harmonics)h.prepare(rate);
 setLatencySamples(HarmonicStage::latency);
 fixBlend.reset(rate,0.05);
 fixMeasurePole=std::exp(-1.0/(rate*0.4));
 fixGainPole=std::exp(-1.0/(rate*0.75));
 meterRelease=static_cast<float>(std::exp(-1.0/(rate*0.25))); rmsPole=static_cast<float>(std::exp(-1.0/(rate*0.3))); reset();
}
void VCASSLProcessor::reset() {
 dynamics.reset();
 for(auto& h:harmonics)h.reset();for(auto& h:dryHistory)h.fill(0);dryPosition=0;
 fixInputChannelEnergy.fill(0);fixOutputChannelEnergy.fill(0);fixInputEnergy=fixOutputEnergy=0;fixPostDb=0;fixBlend.setCurrentAndTargetValue(fix->load()>0.5f?1.f:0.f);
 for(auto& h:sideHP) h.clear(); envelope=rms=grMeter=0; fadeGain=fade->load()>0.5f?0.f:1.f;
 thresholdDb.setCurrentAndTargetValue(threshold->load()); makeupGain.setCurrentAndTargetValue(gain(makeup->load()));
 trimGain.setCurrentAndTargetValue(gain(trim->load())); wet.setCurrentAndTargetValue(mix->load()/100.f);
 enabled.setCurrentAndTargetValue(power->load()>0.5f&&bypass->load()<0.5f?1.f:0.f);
 scHz.setCurrentAndTargetValue(scEnabled->load()>0.5f?80.f:0.f); currentGrDb.store(0);currentOutDb.store(-100);
}
void VCASSLProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) {processAudio(buffer,midi,false);}
void VCASSLProcessor::processAudio(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi,bool hostBypassed) {
 juce::ScopedNoDenormals guard; midi.clear();
 auto b=getBusBuffer(buffer,false,0); const int n=b.getNumChannels(); if(n==0)return;
 auto side=getBusBuffer(buffer,true,1);
 const int scMode=juce::jlimit(0,2,juce::roundToInt(sidechainSource->load()));
 const bool ext=scMode==2||(scMode==0&&side.getNumChannels()>0);
 thresholdDb.setTargetValue(threshold->load()); makeupGain.setTargetValue(gain(makeup->load()));trimGain.setTargetValue(gain(trim->load()));
 wet.setTargetValue(mix->load()/100.f);enabled.setTargetValue(power->load()>0.5f&&bypass->load()<0.5f?1.f:0.f);scHz.setTargetValue(scEnabled->load()>0.5f?80.f:0.f);
 const int ai=juce::jlimit(0,5,juce::roundToInt(attack->load())),ri=juce::jlimit(0,4,juce::roundToInt(release->load()));
 dynamics.configure(juce::jlimit(0,2,juce::roundToInt(ratio->load())),ai,ri);
 fixBlend.setTargetValue(fix->load()>0.5f?1.f:0.f);
 const float fadeStep=1.f/static_cast<float>(rate*fadeRate->load());
 for(int i=0;i<b.getNumSamples();++i) {
  float input[2]{},dry[2]{};for(int ch=0;ch<n;++ch){input[ch]=safeAudio(b.getSample(ch,i));
   auto& history=dryHistory[static_cast<size_t>(ch)];dry[ch]=history[static_cast<size_t>(dryPosition)];history[static_cast<size_t>(dryPosition)]=input[ch];}
  if(++dryPosition==HarmonicStage::latency)dryPosition=0;
  // Measure the original input independently; FIX never changes the compressor drive.
  constexpr double fixGateEnergy = 1.e-7;
  fixInputEnergy=0;for(int ch=0;ch<n;++ch){auto& e=fixInputChannelEnergy[static_cast<size_t>(ch)];
   e=fixMeasurePole*e+(1-fixMeasurePole)*static_cast<double>(dry[ch])*dry[ch];fixInputEnergy=juce::jmax(fixInputEnergy,e);}
  const float fixing=fixBlend.getNextValue();
  const float hz=scHz.getNextValue(); const float pole=static_cast<float>(std::exp(-juce::MathConstants<double>::twoPi*juce::jmax(21.f,hz)/rate));
  float detector=0;const int sn=ext?side.getNumChannels():n;
  for(int ch=0;ch<sn;++ch){float x=ext?side.getSample(ch,i):input[ch];x=safeAudio(x);
   auto& hp=sideHP[static_cast<size_t>(ch)];hp.pole=pole;const float filtered=hp.tick(x);detector=juce::jmax(detector,std::abs(hz>0.5f?filtered:x));}
  envelope=dynamics.tick(detector,thresholdDb.getNextValue());
  fadeGain=juce::jlimit(0.f,1.f,fadeGain+(fade->load()>0.5f?-fadeStep:fadeStep));
  const float mg=makeupGain.getNextValue(),tg=trimGain.getNextValue(),w=wet.getNextValue(),on=enabled.getNextValue();
  const float compressionGain=gain(-envelope);
  float compressed[2]{};fixOutputEnergy=0;
  for(int ch=0;ch<n;++ch){const float clean=input[ch]*compressionGain;
   const float coloured=harmonics[static_cast<size_t>(ch)].tick(clean);
   const float x=coloured*mg;
   compressed[ch]=(dry[ch]+w*(x-dry[ch]))*tg;auto& e=fixOutputChannelEnergy[static_cast<size_t>(ch)];
   e=fixMeasurePole*e+(1-fixMeasurePole)*static_cast<double>(compressed[ch])*compressed[ch];fixOutputEnergy=juce::jmax(fixOutputEnergy,e);}
  if(fixInputEnergy>fixGateEnergy&&fixOutputEnergy>fixGateEnergy){
   const double wanted=juce::jlimit(-24.0,24.0,10.0*std::log10(fixInputEnergy/fixOutputEnergy));
   fixPostDb=fixGainPole*fixPostDb+(1-fixGainPole)*wanted;}
  const float postGain=1.f+fixing*(gain(static_cast<float>(fixPostDb))-1.f);
  float energy=0;
  for(int ch=0;ch<n;++ch){const float processed=compressed[ch]*postGain*fadeGain;
   const float out=hostBypassed||on<=0?dry[ch]:dry[ch]+on*(processed-dry[ch]);b.setSample(ch,i,out);energy=juce::jmax(energy,out*out);}

  rms=rmsPole*rms+(1-rmsPole)*energy;grMeter=juce::jmax(envelope*on,grMeter*meterRelease);
 }
 currentGrDb.store(grMeter);currentOutDb.store(10.f*std::log10(juce::jmax(1.e-10f,rms)));
}
void VCASSLProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer& m) {processAudio(b,m,true);currentGrDb.store(0);}
juce::AudioProcessorParameter* VCASSLProcessor::getBypassParameter() const {return apvts.getParameter("bypass");}
void VCASSLProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty("editorWidth", editorWidth.load(), nullptr);
    state.setProperty("editorHeight", editorHeight.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}
void VCASSLProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xml);
            editorWidth.store(juce::jlimit(817, 4085, static_cast<int>(state.getProperty("editorWidth", 1634))));
            editorHeight.store(juce::jlimit(126, 630, static_cast<int>(state.getProperty("editorHeight", 252))));
            apvts.replaceState(state);
        }
}
juce::AudioProcessorEditor* VCASSLProcessor::createEditor() { return new VCASSLEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VCASSLProcessor(); }
