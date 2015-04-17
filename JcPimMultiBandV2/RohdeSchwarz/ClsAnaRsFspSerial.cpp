/**
 * @file ClaAnaRsFspSerial.cpp
 * @brief
 *
 *
 * @author
 * @note
 * @copyright Jointcom
 * @date 2015.4.10
 * @version v0.1 alpha
 */


#include "ClsAnaRsFspSerial.h"

using namespace std;

bool ClsAnaRsFspSerial::InstrConnect(const char* c_addr)
{
	bool isconn = AgConnect(c_addr);
	//连接成功即开始初始化
	if (isconn)
		InstrInit();
	return isconn;
}

void ClsAnaRsFspSerial::InstrSession(unsigned long viConnectedSession)
{
	AgSession(viConnectedSession);
	//连接成功即开始初始化
	InstrInit();
}

bool ClsAnaRsFspSerial::InstrWrite(const char* c_cmd)
{
	return AgWrite(c_cmd);
}

long ClsAnaRsFspSerial::InstrWriteAndRead(const char* c_cmd, char* rbuf)
{
	return AgWriteAndRead(c_cmd, rbuf);
}

bool ClsAnaRsFspSerial::InstrConnStatus()const
{
	return AgConnStatus();
}

void ClsAnaRsFspSerial::InstrClose()
{
	AgClose();
}

void ClsAnaRsFspSerial::InstrInit()
{
    AgWrite("*CLS\n");
    AgWrite("*RST\n");
    AgWrite("SYST:DISP:UPD ON\n");
//    Preset (preset_default);
}

double ClsAnaRsFspSerial::InstrGetAnalyzer(double freq_khz, bool isMax)
{
	char const *mark_x = "CALC:MARK1:X %.3f%s\n";
	char const *mark_max = "CALC:MARK1:MAX\n";

	char const *mark_y = "CALC:MARK1:Y?\n";

	if (isMax == true)
		CommonSet(mark_max);
	else
		CommonSet(mark_x, freq_khz, "KHz");

	char buf[1024] = { 0 };
	long retCount = AgWriteAndRead(mark_y, buf);

	double val = -10000;

	if (retCount)
		val = atof(buf);

	return val;
}
void ClsAnaRsFspSerial::InstrSetAvg(const int& avg_time)
{
    //[SENSe<1|2>:]AVERage:COUNt 0 to 32767
    char const *set_aver_count = "AVER:COUN %d%s\n";
	if (avg_time > 0) 
		CommonSet("AVER:COUN %d\n", avg_time);
	else 
		InstrClose();
}
void ClsAnaRsFspSerial::InstrClosgAvg()
{
    //[SENSe<1|2>:]AVERage[:STATe<1...3>] ON | OFF
    //char const *set_aver_state = "AVER OFF\n";
	CommonSet("AVER OFF\n");
}
void ClsAnaRsFspSerial::InstrSetOffset(const double& pow_dbm)
{
    //[SENSe<1|2>:]FREQuency:OFFSet <numeric_value>
    //char const *set_freq_offset = "FREQ:OFFS %.0f%s\n";
	CommonSet("DISP:TRAC:Y:RLEV:OFFS %lf dBm\n", pow_dbm);
}

// void InstrSetAttRef(const int& att, const int& reflevel) = 0;
// void InstrSetRbwSpan(const int& rbw_hz, const int& span_hz) = 0;

void ClsAnaRsFspSerial::InstrSetAtt(const int& att)
{//INPut<1|2>:ATTenuation 0 to 70/75dB
    //char const *set_att = "INP:ATT %d%s\n";
	CommonSet("INP:ATT %d dB\n", att);
}
void ClsAnaRsFspSerial::InstrSetRef(const int& reflevel)
{
    //DISPlay[:WINDow<1|2>]:TRACe<1...3>:Y[:SCALe]:RLEVel -130dBm to 30dBm
    //char const *set_disp_rlevel = "DISP:TRAC:Y:RLEV %d%s\n";
	CommonSet("DISP:TRAC:Y:RLEV %d dBm\n", reflevel);
}
void ClsAnaRsFspSerial::InstrSetRbw(const double& rbw_hz)
{
    //[SENSe<1|2>:]BANDwidth|BWIDth[:RESolution] 10 Hz to 10 MHz
    char const *set_rbw = "BAND %.0f%s\n";
    CommonSet (set_rbw, rbw_hz, "Hz");
}
void ClsAnaRsFspSerial::InstrSetVbw(const double& vbw_hz)
{
    //[SENSe<1|2>:]BANDwidth|BWIDth:VIDeo 1 Hz to 10 MHz
    char const *set_vbw = "BAND:VID %.0f%s\n";
    CommonSet (set_vbw, vbw_hz, "Hz");
}
void ClsAnaRsFspSerial::InstrSetSpan(const double& span_hz)
{
    //[SENSe<1|2>:]FREQuency:SPAN 0 to fmax
    char const *set_freq_span = "FREQ:SPAN %.0f%s\n";
    CommonSet (set_freq_span, span_hz, "Hz");
}
void ClsAnaRsFspSerial::InstrSetCenterFreq(const double& freq_khz)
{
    //[SENSe<1|2>:]FREQuency:CENTer 0 to fmax
    char const *set_freq_center = "FREQ:CENT %.3f%s\n";
    CommonSet (set_freq_center, freq_khz, "KHz");
}

void ClsAnaRsFspSerial::InstrSetSweepTime(int count_ms) {
	CommonSet("SWE:TIME %d ms", count_ms);
}

void ClsAnaRsFspSerial::Preset(enum preset_parameter pp)
{
	static const double freq_span[PRESET_PARAMETER_TOTAL] = { 500, 0, 0 };
	static const int freq_aver[PRESET_PARAMETER_TOTAL] = { 0, 1, 0 };
	static const double freq_vbw[PRESET_PARAMETER_TOTAL] = { 10, 100, 0 };
	static const double freq_rbw[PRESET_PARAMETER_TOTAL] = { 10, 30, 0 };

	static const int sweep_time[PRESET_PARAMETER_TOTAL] = { 1, 1, 0 };

	static const int disp_rlev[PRESET_PARAMETER_TOTAL] = { -60, 0, 0 };
	static const double disp_rlev_offset[PRESET_PARAMETER_TOTAL] = { 0, 0, 0 };

	static const int input_att[PRESET_PARAMETER_TOTAL] = { 0, 0, 0 };

	if (pp < 0 || pp >= PRESET_PARAMETER_TOTAL)
		return;

	if (pp_ == pp)
		return;

	pp_ = pp;

	InstrSetSpan(freq_span[pp]);
	InstrSetVbw(freq_vbw[pp]);
	InstrSetRbw(freq_rbw[pp]);
	InstrSetAvg(freq_aver[pp]);

	InstrSetSweepTime(sweep_time[pp]);

	InstrSetRef(disp_rlev[pp]);
	InstrSetOffset(disp_rlev_offset[pp]);

	InstrSetAtt(input_att[pp]);
    
}

bool ClsAnaRsFspSerial::CommonSet(char const *command, ...)
{
    
    char command_data[1024] = {0};
    
    va_list ap;
    va_start(ap, command);
    vsnprintf_s(command_data, 1023, command, ap);
    va_end(ap);
    
    cout<<command_data<<endl;
    
    return InstrWrite(command_data);
}

void ClsAnaRsFspSerial::InstrPimSetting() {
	Preset(preset_parameter::preset_default);
}

void ClsAnaRsFspSerial::InstrVcoSetting() {
	Preset(preset_parameter::preset_mensuration);
}

void ClsAnaRsFspSerial::InstrTxOffsetSetting() {
	Preset(preset_parameter::preset_calibration);
}

void ClsAnaRsFspSerial::InstrRxOffsetSetting() {
	Preset(preset_parameter::preset_default);
}

