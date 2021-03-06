
#ifndef CLS_SIG_RS_SMX_SERIAL_H
#define CLS_SIG_RS_SMX_SERIAL_H


#include "../ClsInstrAgilent.hpp"
#include "../IfSignalSource.hpp"


class ClsSigRsSMxSerial : public ClsInstrAgilent, Implements_ IfSignalSource
{
#define AG_POW_OPEN true
#define AG_POW_CLOSE false
public:
    ClsSigRsSMxSerial()
        :_bOpen(false),
        _isCmdSucc(false),
        _freq_now(0),
        _pow_now(0),
        _sourTunnel(1),
        ClsInstrAgilent()
    {}

    ~ClsSigRsSMxSerial() {}

public:
    bool InstrConnect(const char* c_addr);

	void InstrSession(unsigned long viConnectedSession, const char* cIdn);

    bool InstrWrite(const char* c_cmd);

    long InstrWriteAndRead(const char* c_cmd, char* rbuf);

    bool InstrConnStatus() const;

    void InstrClose();

public:
    bool InstrSetFreq(double freq_khz);

    bool InstrSetPow(double pow_dbm);

    //设置频率功率
    bool InstrSetFreqPow(double freq_khz, double pow_dbm);

    //功放开关
    bool InstrOpenPow(bool bOpen);

    //功放状态
    bool InstrPowStatus() const;

    //检查同步线是否插入/错误
    bool InstrGetReferenceStatus();

    //
    bool InstrInit();

private:
    bool _bOpen;
    bool _isCmdSucc;
    double _freq_now;
    double _pow_now;
    int   _sourTunnel;                 //输出通道 1 = A; 2 = B

    const double _maxFreq_khz = 6000 * 1000;
    const double _minFreq_khz = 100;
    const double _maxPow_dbm = 17;
    const double _minPow_dbm = -110;

};



#endif


