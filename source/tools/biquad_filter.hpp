//
//  biquad_filter.h
//
//  Created by Nigel Redmon on 11/24/12
//  EarLevel Engineering: earlevel.com
//  Copyright 2012 Nigel Redmon
//
//  For a complete explanation of the Biquad code:
//  http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
//
//  License:
//
//  This source code is provided as is, without warranty.
//  You may copy and distribute verbatim copies of this document.
//  You may modify and use this source code to create binary code
//  for your own purposes, free or commercial.
//

#pragma once



class BiquadFilter {
public:
    enum class Type{
        Lowpass = 0,
        Highpass,
        Bandpass,
        Notch,
        Peak,
        Lowshelf,
        Highshelf
    } ;
public:
    BiquadFilter(Type type, double Fc, double Q, double peakGainDB);
    ~BiquadFilter();
    void setType(Type type);
    void setQ(double Q);
    void setFc(double Fc);
    void setPeakGain(double peakGainDB);
    void setBiquad(Type type, double Fc, double Q, double peakGainDB);
    float process(float in);
    
protected:
    void calcBiquad(void);

    Type type;
    double a0, a1, a2, b1, b2;
    double Fc, Q, peakGain;
    double z1, z2;
};

inline float BiquadFilter::process(float in) {
    double out = in * a0 + z1;
    z1 = in * a1 + z2 - b1 * out;
    z2 = in * a2 - b2 * out;
    return out;
}

