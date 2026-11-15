#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
float gain(float x) { return std::pow(10.f,x/20.f); }
float db(float x) { return 20.f*std::log10(juce::jmax(x,1.e-9f)); }
constexpr float attacks[]{0.0001f,0.0003f,0.001f,0.003f,0.01f,0.03f};
constexpr float releases[]{0.1f,0.3f,0.6f,1.2f};
constexpr float ratios[]{2,4,10};
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
 b("fade","Fade Out",false); b("externalSC","External Sidechain",false);
 b("meterMode","Meter Output",false); b("bypass","Bypass",false); return p;
}
VCASSLProcessor::VCASSLProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true).withInput("Sidechain",juce::AudioChannelSet::stereo(),false)),apvts(*this,nullptr,"VCASSL",makeParameters()) {
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
}
bool VCASSLProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
 const auto main=l.getMainInputChannelSet(); const auto sc=l.getChannelSet(true,1);
 return (main==juce::AudioChannelSet::mono()||main==juce::AudioChannelSet::stereo())&&main==l.getMainOutputChannelSet()
 &&(sc.isDisabled()||sc==juce::AudioChannelSet::mono()||sc==juce::AudioChannelSet::stereo());
}
void VCASSLProcessor::prepareToPlay(double sr,int) {
 rate=juce::jmax(8000.0,sr);
 for(auto* s:{&thresholdDb,&makeupGain,&trimGain,&wet,&enabled,&scHz}) s->reset(rate,0.02);
 meterRelease=static_cast<float>(std::exp(-1.0/(rate*0.25))); rmsPole=static_cast<float>(std::exp(-1.0/(rate*0.3))); reset();
}
void VCASSLProcessor::reset() {
 for(auto& h:sideHP) h.clear(); envelope=slowEnvelope=rms=grMeter=0; fadeGain=fade->load()>0.5f?0.f:1.f;
 thresholdDb.setCurrentAndTargetValue(threshold->load()); makeupGain.setCurrentAndTargetValue(gain(makeup->load()));
 trimGain.setCurrentAndTargetValue(gain(trim->load())); wet.setCurrentAndTargetValue(mix->load()/100.f);
 enabled.setCurrentAndTargetValue(power->load()>0.5f&&bypass->load()<0.5f?1.f:0.f);
 scHz.setCurrentAndTargetValue(scFilter->load()); currentGrDb.store(0);currentOutDb.store(-100);wasBypassed=false;
}
void VCASSLProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) {
 juce::ScopedNoDenormals guard; midi.clear(); if(wasBypassed) reset();
 auto b=getBusBuffer(buffer,false,0); const int n=b.getNumChannels(); if(n==0)return;
 auto side=getBusBuffer(buffer,true,1);
 const bool ext=externalSC->load()>0.5f&&side.getNumChannels()>0;
 thresholdDb.setTargetValue(threshold->load()); makeupGain.setTargetValue(gain(makeup->load()));trimGain.setTargetValue(gain(trim->load()));
 wet.setTargetValue(mix->load()/100.f);enabled.setTargetValue(power->load()>0.5f&&bypass->load()<0.5f?1.f:0.f);scHz.setTargetValue(scFilter->load());
 const int ai=juce::jlimit(0,5,juce::roundToInt(attack->load())),ri=juce::jlimit(0,4,juce::roundToInt(release->load()));
 const float r=ratios[juce::jlimit(0,2,juce::roundToInt(ratio->load()))];
 const float ac=static_cast<float>(std::exp(-1.0/(rate*attacks[ai])));
 const float slowCharge=static_cast<float>(std::exp(-1.0/(rate*0.4)));
 const float fadeStep=1.f/static_cast<float>(rate*fadeRate->load());
 for(int i=0;i<b.getNumSamples();++i) {
  float dry[2]{};for(int ch=0;ch<n;++ch){const float x=b.getSample(ch,i);dry[ch]=std::isfinite(x)?x:0.f;}
  const float hz=scHz.getNextValue(); const float pole=static_cast<float>(std::exp(-juce::MathConstants<double>::twoPi*juce::jmax(21.f,hz)/rate));
  float detector=0;const int sn=ext?side.getNumChannels():n;
  for(int ch=0;ch<sn;++ch){float x=ext?side.getSample(ch,i):dry[ch];if(!std::isfinite(x))x=0;
   auto& hp=sideHP[static_cast<size_t>(ch)];hp.pole=pole;const float filtered=hp.tick(x);detector=juce::jmax(detector,std::abs(hz>0.5f?filtered:x));}
  // Detector calibration fitted to SSLComp Stereo V14, 1 kHz, threshold 0, 4:1.
  // Linked feed-forward VCA control with ratio-dependent soft knee.
  const float over=db(detector)+24.1f-thresholdDb.getNextValue();const float knee=r==2?6.f:r==4?3.f:1.f;
  const float slope=1.f-1.f/r;float target=0;
  if(over>=knee/2)target=slope*over;else if(over>-knee/2){const float v=over+knee/2;target=slope*v*v/(2*knee);}
  slowEnvelope=slowCharge*slowEnvelope+(1-slowCharge)*target;
  const float seconds=ri==4?0.1f+1.1f*juce::jlimit(0.f,1.f,slowEnvelope/12.f):releases[ri];
  const float rc=static_cast<float>(std::exp(-1.0/(rate*seconds)));const float coeff=target>envelope?ac:rc;
  envelope=coeff*envelope+(1-coeff)*target;
  fadeGain=juce::jlimit(0.f,1.f,fadeGain+(fade->load()>0.5f?-fadeStep:fadeStep));
  const float mg=makeupGain.getNextValue(),tg=trimGain.getNextValue(),w=wet.getNextValue(),on=enabled.getNextValue();
  float energy=0;
  for(int ch=0;ch<n;++ch){float x=dry[ch]*gain(-envelope)*mg;

   const float processed=(dry[ch]+w*(x-dry[ch]))*tg*fadeGain;
   const float out=on<=0?dry[ch]:dry[ch]+on*(processed-dry[ch]);b.setSample(ch,i,out);energy=juce::jmax(energy,out*out);}
  rms=rmsPole*rms+(1-rmsPole)*energy;grMeter=juce::jmax(envelope*on,grMeter*meterRelease);
 }
 currentGrDb.store(grMeter);currentOutDb.store(10.f*std::log10(juce::jmax(1.e-10f,rms)));
}
void VCASSLProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer& m) {
 m.clear();wasBypassed=true;currentGrDb.store(0);
 auto out=getBusBuffer(b,false,0);float e=0;for(int ch=0;ch<out.getNumChannels();++ch)e=juce::jmax(e,out.getRMSLevel(ch,0,out.getNumSamples()));currentOutDb.store(db(e));
}
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
