#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>

class VCASSLProcessor final : public juce::AudioProcessor
{
public:
    VCASSLProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "VCA SSL"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> currentGrDb { 0.0f }, currentOutDb { -100.0f };
    std::atomic<int> editorWidth { 1634 }, editorHeight { 252 };
private:
    struct HighPass
    {
        float previousInput = 0, previousOutput = 0, pole = 0;
        float tick(float x) noexcept { const float y = pole * (previousOutput + x - previousInput); previousInput = x; previousOutput = y; return y; }
        void clear() noexcept { previousInput = previousOutput = 0; }
    };
    struct KiHighPass {
        double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
        void prepare(double sr,double q) noexcept {
            const double w=juce::MathConstants<double>::twoPi*18.0/sr,c=std::cos(w),alpha=std::sin(w)/(2*q),d=1+alpha;
            b0=(1+c)/(2*d);b1=-(1+c)/d;b2=b0;a1=-2*c/d;a2=(1-alpha)/d;
        }
        float tick(float x) noexcept {const double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return static_cast<float>(y);}
        void clear() noexcept {z1=z2=0;}
    };
    std::array<HighPass, 2> sideHP;
    std::array<std::array<KiHighPass,2>,2> kiHighPass;

    std::atomic<float> *power, *threshold, *makeup, *attack, *release, *ratio, *bypass,
        *scFilter, *mix, *trim, *fade, *fadeRate, *externalSC, *fix, *ki;
    juce::SmoothedValue<float> thresholdDb, makeupGain, trimGain, wet, enabled, scHz, fixBlend, kiBlend;
    double rate = 44100;
    float envelope = 0, slowEnvelope = 0, fadeGain = 1, rms = 0, grMeter = 0;
    float meterRelease = 0, rmsPole = 0;
    double fixInputEnergy = 0, fixOutputEnergy = 0;
    std::array<double,2> fixInputChannelEnergy{}, fixOutputChannelEnergy{};
    float fixPostDb = 0, fixMeasurePole = 0, fixGainPole = 0;
    float kiPeak=0, kiReleasePole=0;
    bool wasBypassed = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VCASSLProcessor)
};
