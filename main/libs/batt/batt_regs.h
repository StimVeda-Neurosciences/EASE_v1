#pragma once

/////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
///////// define the battery parameters here

/////// define the batt capacity
#define Batt_Mah 600

///// define the vempty and vrecovery

#define BATT_VEMPTY 340

#define BATT_VRECOVERY 83

/// define the clock freq
#define scl_freq 400000 // 400 khz

#define i2c_wait_time 10

#define FUEL_GAUGE_DEV_ID 0x4031

///////////// defination of fuel gauge register

#define Sens_addr 0x36

/////////// regiater definations of max17 IC

#define SOCHold 0xD3

#define ScOcvLim 0xD1

#define CGain 0x2E

#define COff 0x2F

#define Curve 0xB9

#define CGTempCo 0xB8

#define AIN 0x27

#define TGain 0x2C

#define TOff 0x2D

#define MPPCurrent 0xD9

#define SPPCurrent 0xDA

#define MaxPeakPower 0xD4

#define SusPeakPower 0xD5

#define PackResistance 0xD6

#define SysResistance 0xD7

#define MinSysVoltage 0xD8

#define RGain 0x43

#define VEmpty 0x3A

#define DesignCap 0x18

#define AvgVcell 0x19

#define MAXMinVolt 0x1B

#define ModelCFG 0xDB

#define ICFGTerm 0x1E

#define FullSOCThr 0x13

#define OCVTable0 0x80

#define OCVTable15 0x8F

#define XTable0  0x90
#define XTable15 0x9F

#define QRTable00 0x12
#define QRTable30 0x42

#define RComp0 0x38

#define TempCo 0x39

#define ReCap 0x05

#define ATAVCap 0xDF

#define RepSOC 0x06

#define battcurrent 0x0A

#define ATAVSOC 0xDE

#define FullCapRep 0x10

#define FullCap 0x35

#define FullCapNom 0x23

#define TTE 0x11

#define AtTTE 0xDD

#define TTF 0x20

#define CYcles 0x17

#define STATUS 0x00

#define AGE 0x07

#define Vcell 0x09

#define TimerH 0xBE

#define Timer 0x3E

#define RCell 0x14

#define VRipple 0xBC

#define AtRate 0x04

#define FilterCFG 0x29

#define RelaxCFG 0x2A

#define LearnCFG 0x28

#define MiscCFG 0x2B

#define ConvgCFG 0x49

#define RippleCFG 0xBD

#define dQAcc 0x45

#define dPAcc 0x46

#define QResidual 0x0c

#define AtQesidual 0xDC

#define VFSOC 0xFF

#define VFOCV 0xFB

#define QH 0x4D

#define AvCap 0x1F

#define AvSOC 0x0E

#define MixCap 0x0F

#define MixSOC 0x0D

#define VFREMCap 0x4A

#define FStat 0x3D

#define Config1 0x1D

#define Config2 0xBB

#define DevName 0x21

#define ShdnTimer 0x3F

#define Status2 0xB0

#define HibCFG 0xBA

#define SOFTWAKEUP 0x60

#define DieTemp 0x34

#define AvgCurrnt 0x0B

#define MaxminCurnt 0x1C

/////////////// function defination of the ic
