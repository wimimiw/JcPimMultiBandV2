#include "JcApi.h"
#include "stdafx.h"
#include "JcPimObject.h"
#include "MyUtil\JcCommonAPI.h"

#define __pobj JcPimObject::Instance()

#define OFFSET_PROTECT_TX -10
#define OFFSET_PROTECT_RX -90
#define SIGNAL_SOURCE_MAX_POW 8

#define OFFSET_TX_THREASOLD 0.05
#define SMOOTH_TX_THREASOLD 2
#define SMOOTH_VCO_THREASOLD 5

//#define JC_OFFSET_TX_SINGLE_DEBUG
//#define JC_OFFSET_TX_DEBUG
//#define JC_OFFSET_RX_DEBUG

//���ʼƱ�ʶ
#define INSTR_AG_U2000_SERIES 0
#define INSTR_RS_NRT_SERIES 1
#define INSTR_RS_NRPZ_SERIES 2

//�ź�Դ��ʶ
#define INSTR_AG_MXG_SERIES 10
#define INSTR_RS_SM_SERIES 11

//Ƶ���Ǳ�ʶ
#define INSTR_AG_MXA_SERIES 20
#define INSTR_RS_FS_SERIES 21

JcBool _switch_enable[7] = { 1, 1, 1, 1, 1, 1, 1 };
JcBool _debug_enable = 0;

std::wstring _startPath = [](){
	wchar_t wcBuff[512] = { 0 };
	Util::getMyPath(wcBuff, 256, L"JcPimMultiBandV2.dll");
	std::wstring wsPath_ini = std::wstring(wcBuff) + L"\\JcConfig.ini";
	_switch_enable[0] = GetPrivateProfileIntW(L"Connect_Enable", L"band0", 1, wsPath_ini.c_str());
	_switch_enable[1] = GetPrivateProfileIntW(L"Connect_Enable", L"band1", 1, wsPath_ini.c_str());
	_switch_enable[2] = GetPrivateProfileIntW(L"Connect_Enable", L"band2", 1, wsPath_ini.c_str());
	_switch_enable[3] = GetPrivateProfileIntW(L"Connect_Enable", L"band3", 1, wsPath_ini.c_str());
	_switch_enable[4] = GetPrivateProfileIntW(L"Connect_Enable", L"band4", 1, wsPath_ini.c_str());
	_switch_enable[5] = GetPrivateProfileIntW(L"Connect_Enable", L"band5", 1, wsPath_ini.c_str());
	_switch_enable[6] = GetPrivateProfileIntW(L"Connect_Enable", L"band6", 1, wsPath_ini.c_str());
	_debug_enable = GetPrivateProfileIntW(L"Settings", L"debug", 0, wsPath_ini.c_str());

	return std::wstring(wcBuff);
}();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//��ʼ�����ͷ�
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fnSetInit(JC_ADDRESS cDeviceAddr) {
	bool isSqlConn = false;
	//����pim����
	if (NULL != __pobj) {

		//����ģ��
		//__pobj->vna = std::make_shared<ClsVnaAgE5062A>();

		//�����ַ
		std::istringstream iss(cDeviceAddr);
		std::vector<std::string> vaddr;
		std::string stemp = "";
		while (std::getline(iss, stemp, ',')) {
			vaddr.push_back(stemp);
		}
		//��λ,Ĭ�Ͽ�������
		if (vaddr.size() == 4)
			vaddr.push_back("1");

		//��ʼ�������ݿ�
#ifdef WIN32
		std::wstring wsPath = _startPath + L"\\JcOffset.db";
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		std::string sPath = conv.to_bytes(wsPath);
		isSqlConn = __pobj->offset.Dbconnect(sPath.c_str());
		if (!isSqlConn) {
			Util::logged(L"fnSetInit: DB Connected Error!");
			//return JC_STATUS_ERROR_DATABASE_CONN_FAIL;
		}
#else
		//isconn = __pobj->offset.Dbconnect("D:\\Sync_ProJects\\Jointcom\\JcPimMultiBandV2\\Debug\\JcOffset.db");
#endif
		std::wstring wsPath_ini = _startPath + L"\\JcConfig.ini";
		double vco_limit = Util::getIniDouble(L"Settings", L"vco_limit", 5, wsPath_ini.c_str());
		double tx_smooth = Util::getIniDouble(L"Settings", L"tx_smooth", 2, wsPath_ini.c_str());
		double tx_accuracy = Util::getIniDouble(L"Settings", L"tx_accuracy", 2, wsPath_ini.c_str());
		__pobj->now_vco_threasold = vco_limit <= 0 ? SMOOTH_VCO_THREASOLD : vco_limit;
		__pobj->now_tx_smooth_threasold = tx_smooth <= 0 ? SMOOTH_TX_THREASOLD : tx_smooth;
		__pobj->now_tx_smooth_accuracy = tx_accuracy <= 0 ? 0.15 : tx_accuracy;

		__pobj->debug_time = GetPrivateProfileIntW(L"Settings", L"time", 200, wsPath_ini.c_str());
		__pobj->now_vco_enbale[0] = GetPrivateProfileIntW(L"VCO_Enable", L"vco_band0", 1, wsPath_ini.c_str());
		__pobj->now_vco_enbale[1] = GetPrivateProfileIntW(L"VCO_Enable", L"vco_band1", 1, wsPath_ini.c_str());
		__pobj->now_vco_enbale[2] = GetPrivateProfileIntW(L"VCO_Enable", L"vco_band2", 1, wsPath_ini.c_str());
		__pobj->now_vco_enbale[3] = GetPrivateProfileIntW(L"VCO_Enable", L"vco_band3", 1, wsPath_ini.c_str());
		__pobj->now_vco_enbale[4] = GetPrivateProfileIntW(L"VCO_Enable", L"vco_band4", 1, wsPath_ini.c_str());
		__pobj->now_vco_enbale[5] = GetPrivateProfileIntW(L"VCO_Enable", L"vco_band5", 1, wsPath_ini.c_str());
		__pobj->now_vco_enbale[6] = GetPrivateProfileIntW(L"VCO_Enable", L"vco_band6", 1, wsPath_ini.c_str());

		//��ʼ����
		std::string strConnMsg = "Connected Info:\r\n";

		ViSession viSession_Ana = VI_NULL;
		ViSession viSession_Sen = VI_NULL;
		ViSession viSession_Sig1 = VI_NULL;
		ViSession viSession_Sig2 = VI_NULL;
		ViStatus s = VI_NULL;
		int index_sensor = -1;
		int index_analyzer = -1;
		int index_signal1 = -1;
		int index_signal2 = -1;

		s = viOpenDefaultRM(&__pobj->viDefaultRM);
		if (s) return false;

		if (vaddr.size() >= 4) {
			//��ʼ�����ź�Դ1
			if (vaddr[JC_DEVICE::SIGNAL1] != JC_DEVICE_NOT_ENABLE) {
				//__pobj->now_status[JC_DEVICE::SIGNAL1] = __pobj->sig1->InstrConnect(vaddr[0].c_str());
				s = viOpen(__pobj->viDefaultRM, const_cast<char*>(vaddr[0].c_str()), VI_NULL, VI_NULL, &viSession_Sig1);
				if (s == VI_SUCCESS) {
					__pobj->now_status[JC_DEVICE::SIGNAL1] = true;
					index_signal1 = JcGetIDN(viSession_Sen);
					if (index_signal1 == INSTR_AG_MXG_SERIES){
						__pobj->sig1 = std::make_shared<ClsSigAgN5181A>();
						__pobj->sig1->InstrSession(viSession_Sig1);
					}
					else if (index_signal1 == INSTR_RS_SM_SERIES) {
						__pobj->sig1 = std::make_shared<ClsSigRsSMxSerial>();
						__pobj->sig1->InstrSession(viSession_Sig2);
					}
				}
				else 
					Util::logged(L"fnSetInit: Connect SG1 Fail! (%s)", conv.from_bytes(vaddr[0]).c_str());	
			}
			//��ʼ�����ź�Դ2
			if (vaddr[JC_DEVICE::SIGNAL2] != JC_DEVICE_NOT_ENABLE) {
				s = viOpen(__pobj->viDefaultRM, const_cast<char*>(vaddr[1].c_str()), VI_NULL, VI_NULL, &viSession_Sig2);
				if (s == VI_SUCCESS) {
					__pobj->now_status[JC_DEVICE::SIGNAL2] = true;
					index_signal2 = JcGetIDN(viSession_Sen);
					if (index_signal2 == INSTR_AG_MXG_SERIES) {
						__pobj->sig2 = std::make_shared<ClsSigAgN5181A>();
						__pobj->sig2->InstrSession(viSession_Sig2);
					}
					else if (index_signal2 == INSTR_RS_SM_SERIES) {
						__pobj->sig2 = std::make_shared<ClsSigRsSMxSerial>();
						__pobj->sig2->InstrSession(viSession_Sig2);
					}
				}
				else 
					Util::logged(L"fnSetInit: Connect SG2 Fail! (%s)", conv.from_bytes(vaddr[1]).c_str());			
			}
			//��ʼ���ӹ��ʼ�
			if (vaddr[JC_DEVICE::SENSOR] != JC_DEVICE_NOT_ENABLE) {
				//�������Ǳ�����
				__pobj->now_status[JC_DEVICE::SENSOR] = false;
				s = viOpen(__pobj->viDefaultRM, const_cast<char*>(vaddr[3].c_str()), VI_NULL, VI_NULL, &viSession_Sen);
				if (s == VI_SUCCESS) {
					__pobj->now_status[JC_DEVICE::SENSOR] = true;
					index_sensor = JcGetIDN(viSession_Sen);
					if (index_sensor == INSTR_AG_U2000_SERIES) {
						__pobj->sen = std::make_shared<ClsSenAgU2000A>();
						__pobj->sen->InstrSession(viSession_Sen);
					}
					else if (index_sensor == INSTR_RS_NRT_SERIES) {
						__pobj->sen = std::make_shared<ClsSenRsNrt>();
						__pobj->sen->InstrSession(viSession_Sen);
					}
				}
				//RS�Ǳ�����
				else {
					index_sensor = INSTR_RS_NRPZ_SERIES;
					__pobj->sen = std::make_shared<ClsSenRsNrpz>();
					__pobj->now_status[JC_DEVICE::SENSOR] = __pobj->sen->InstrConnect(vaddr[3].c_str());
				}

				if (false == __pobj->now_status[3])
					Util::logged(L"fnSetInit: Connect PowerMeter Fail! (%s)", conv.from_bytes(vaddr[3]).c_str());
			}
			//��ʼ����Ƶ����,��ʼ��Ƶ����
			if (vaddr[JC_DEVICE::ANALYZER] != JC_DEVICE_NOT_ENABLE) {
				s = viOpen(__pobj->viDefaultRM, const_cast<char*>(vaddr[2].c_str()), VI_NULL, VI_NULL, &viSession_Ana);
				if (s == VI_SUCCESS) {
					__pobj->now_status[JC_DEVICE::ANALYZER] = true;
					__pobj->ana = std::make_shared<ClsAnaAgN9020A>();
					__pobj->ana->InstrSession(viSession_Ana);
				}
				else {
					Util::logged(L"fnSetInit: Connect SA Fail! (%s)", conv.from_bytes(vaddr[2]).c_str());
				}

			}
			//��ʼ���ӿ���
			__pobj->swh = std::make_shared<ClsJcSwitch>();
			if (__pobj->swh->SwitchInit()){

				if (vaddr[JC_DEVICE::SWITCH] != JC_DEVICE_NOT_ENABLE) {
					//����ʹ��
					for (int i = 0; i < 7; ++i) {
						if (_switch_enable[i] == 0)
							__pobj->swh->SwitchSetEnable(i, false);
						else
							__pobj->swh->SwitchSetEnable(i, true);
					}

					//��ʼ����
					__pobj->isSwhConn = __pobj->swh->SwitchConnect();
				}
			}
			else
				Util::logged(L"fnSetInit: Switch LoadMap Error! (%s)");
		}

		//�ж�����
		__pobj->isAllConn = __pobj->now_status[0] & __pobj->now_status[1] & __pobj->now_status[2] & __pobj->now_status[3];
		__pobj->isAllConn &= __pobj->isSwhConn;
		__pobj->isAllConn &= isSqlConn;
		//��¼������Ϣ
		__pobj->strErrorInfo = ("SIG1 Connected: " + std::to_string(__pobj->now_status[0]) + "\r\n");
		__pobj->strErrorInfo += ("SIG2 Connected: " + std::to_string(__pobj->now_status[1]) + "\r\n");
		__pobj->strErrorInfo += ("Spectrum Connected: " + std::to_string(__pobj->now_status[2]) + "\r\n");
		__pobj->strErrorInfo += ("Sensor Connected: " + std::to_string(__pobj->now_status[3]) + "\r\n");
		std::string strSwhInfo = __pobj->swh->SwitchGetInfo();
		__pobj->strErrorInfo += strSwhInfo;
		if (!isSqlConn) {
			__pobj->strErrorInfo += "DataBaseConnected: ";
			__pobj->strErrorInfo += std::to_string(isSqlConn);
			__pobj->strErrorInfo += "(JcOffset.db 's Path Error!)\r\n";
		}

		//���ý����ⲿƵ��
		HwSetIsExtBand(TRUE);

		if (false == __pobj->isAllConn)
			return JC_STATUS_ERROR;
	}

	return 0;
}

//�ͷ�
int fnSetExit(){
	JcPimObject::release();
	//����2s����ʱ�Ż�ر�����
	Util::setSleep(2000);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//��ΪAPI
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//int SetInit(char *csDeviceAddr)	��ʼ������	�����ź�Դ��Ƶ���ǡ����ʼƵ�ַ���ö��Ÿ���,���ʽ��SG1Addr, SG2Addr, SAAddr, PMAddr
//int SetExit()	�ر������ͷ���Դ
//int SetMeasBand(BYTE byBandIndex)	ѡ��Ƶ��	7��Ƶ�Σ��������磺 0��DD800 1:900��������6 : LTE2600��7��LTE700
//int SetImAvg(BYTE byAvgTime)	����ƽ������	byAvgTime : 0 - None, 1 - Minimal, 2 - Normal, 3 - High, 4 - Maximum
//int SetDutPort(BYTE byPort)	���ò��Զ˿�	0��port1�� 1��port2
//int SetImOrder(BYTE byImOrder)	����IM���Խ���	3��5��7��
//int SetTxPower(double dTxPower1,
//int double dTxPower2, double dPowerOffset1, double dPowerOffset2)	���ù��ʼ����𲹳�	�磺SetTxPower(43, 43, 0.5��0.5)
//int SetTxFreqs(double dCarrierFreq1, double dCarrierFreq2, CString csUnits)	����F1��F2Ƶ�㣬��λMHz	����������ӿ�����
//                                                                          ����·���ʷֱ���й��ʼ�أ�
//                                                                          ������ʲ���43�����������
//int SetTxOn(BOOL bOn, BYTE byCarrier = 0)	�򿪻�رչ���	��һ��������ʾ�򿪻��ǹرգ��ڶ���������0��ʾF1F2��ִ�У�1��ʾF1    2��ʾF2
//int GetImResult(double & dFreq, double& dPimResult, CString csUnits)	��ȡPIM���Խ��	���һ������dBm
//int SetSpan(int iSpan, CString csUnit)	��	����ֵ�͵�λ
//int SetRBW(int iRBW, CString csUnit)
//int SetVBW(int iVBW, CString csUnits)
//int SendCmd(BYTE byDevice, CString
//int csCmd, CString & csResult)	ͨ��͸���ӿ�	byDevice : �������(00:sg1, 01 : sg2, 02 : sa,03 : Pm), csCmd : ����ָ��, csResult : ָ��ؽ��
//int GetSpectrumType(CString & csSpectrumType)	��ȡƵ��������

int fnSetMeasBand(JcInt8 byBandIndex){
	if (__pobj->isUseExtBand)
		__pobj->now_band = __pobj->GetExtBandToIntBand(byBandIndex);
	else
		__pobj->now_band = byBandIndex;

	return 0;
}

int fnSetImAvg(JcInt8 byAvgTime) {
	if (byAvgTime < 1) byAvgTime = 1;
	__pobj->now_imAvg = byAvgTime;
	//����Ƶ����ƽ��
	__pobj->ana->InstrSetAvg(byAvgTime);
	return 0;
}

//�������� HwSetMeasBand
int fnSetDutPort(JcInt8 byPort) {
	__pobj->dd1 = 0;
	__pobj->dd2 = 0;
	__pobj->now_dut_port = byPort;
	//Bandת�����ز���
	//byPort = JC_DUTPORT_A ��JC_DUTPORT_B
	int iSwitch = __pobj->now_band * 2 + byPort;
	JcBool b = JcSetSwitch(iSwitch, iSwitch, iSwitch, JC_COUP_TX2);
	if (TRUE == b)
		return 0;
	else
		return JC_STATUS_ERROR_SET_SWITCH_FAIL;
}

int fnSetImOrder(JcInt8 byImOrder) {
	//���õ�ǰ���Ի�������,Ĭ��3
	switch (byImOrder)
	{
	case 3:
		__pobj->now_order = 3;  break;
	case 5:
		__pobj->now_order = 5;  break;
	case 7:
		__pobj->now_order = 7;  break;
	case 9:
		__pobj->now_order = 9;  break;
	case 11:
		__pobj->now_order = 11;  break;
	default:
		return JC_STATUS_ERROR_SET_IM_FAIL;
	}

	return 0;
}

int fnCheckReceiveChannel(JcInt8 byBandIndex, JcInt8 byPort) {
	if (__pobj->isUseExtBand){
		JcInt8 byTemp = __pobj->GetExtBandToIntBand(byBandIndex);
		if (__pobj->now_vco_enbale[byTemp] == 0)
			return 0;
	}
	
	fnSetMeasBand(byBandIndex);
	int s = fnSetDutPort(byPort);
	if (s <= -10000){
		Util::logged(L"fnVco: Set Switch Error��");
		return JC_STATUS_ERROR_SET_SWITCH_FAIL;
	}

	Util::setSleep(500);

	//��ʼ����
	double real_val = 0;
	double vco_val = 0;

	if (HwGet_Vco(real_val, vco_val) == FALSE){
		Util::logged(L"fnVco: VCO Error (%lf)", real_val - vco_val);
		return JC_STATUS_ERROR_CHECK_VCO_FAIL;	
	}
	else
		return 0;
}

int fnCheckTwoSignalROSC() {
	if (JcGetSig_ExtRefStatus(JC_CARRIER_TX1) == FALSE)
		return JC_STATUS_ERROR_CHECK_SIG1_ROSC_FAIL;

	if (JcGetSig_ExtRefStatus(JC_CARRIER_TX2) == FALSE)
		return JC_STATUS_ERROR_CHECK_SIG2_ROSC_FAIL;
	return 0;
}

int fnSetTxPower(double dTxPower1, double dTxPower2,
	double dPowerOffset1, double dPowerOffset2) {
	//__pobj->now_txPow1 = dTxPower1;
	//__pobj->now_txPow2 = dTxPower2;
	////�����ⲿУ׼
	//__pobj->offset_txPow1 = dPowerOffset1;
	//__pobj->offset_txPow2 = dPowerOffset2;
	__pobj->now_txPow1 = 43;
	__pobj->now_txPow2 = 43;
	__pobj->offset_txPow1 = dTxPower1 + dPowerOffset1 - 43;
	__pobj->offset_txPow2 = dTxPower1 + dPowerOffset2 - 43;

	return 0;
}

//����Ƶ��
JC_STATUS fnSetTxFreqs(double dCarrierFreq1, double dCarrierFreq2, const JC_UNIT cUnits) {
	//��λת��
	__pobj->now_txFreq1 = __pobj->TransKhz(dCarrierFreq1, cUnits);
	__pobj->now_txFreq2 = __pobj->TransKhz(dCarrierFreq2, cUnits);
	
	JC_STATUS js;
	double dd;
	//---------------------------------------------------------------------------------
	//���ù���2
	js = JcSetSig_Advanced(JC_CARRIER_TX2, __pobj->now_band, __pobj->now_dut_port,
		__pobj->now_txFreq2, __pobj->now_txPow2,
		true, __pobj->offset_txPow2);
	if (js) return js;
	//���ù���1
	js = JcSetSig_Advanced(JC_CARRIER_TX1, __pobj->now_band, __pobj->now_dut_port,
		__pobj->now_txFreq1, __pobj->now_txPow1,
		true, __pobj->offset_txPow1);
	if (js) return js;
	//---------------------------------------------------------------------------------
	//��������
	js = fnSetTxOn(true, JC_CARRIER_TX1TX2);
	if (0 != js) return js;
	//---------------------------------------------------------------------------------
	//�л������tx2����
	JcBool b = HwSetCoup(JC_COUP_TX2);
	if (FALSE == b) {
		//�رչ���
		fnSetTxOn(false, JC_CARRIER_TX1TX2);
		return -10000;
	}
	Util::setSleep(100);
	//���tx1����ƽ�ȶ�
	js = HwGetSig_Smooth(dd, JC_CARRIER_TX2);
	if (js <= -10000) {
		//�رչ���
		fnSetTxOn(false, JC_CARRIER_TX1TX2);
		if (js == JC_STATUS_ERROR_NO_FIND_POWER)
			Util::logged(L"���� TX2δ��⵽���ʣ���칦�������");
		else
			Util::logged(L"���� TX2����ƫ�����");
		return js;
	}
	//---------------------------------------------------------------------------------
	//�л������tx1����
	b = HwSetCoup(JC_COUP_TX1);
	if (FALSE == b) {
		//�رչ���
		fnSetTxOn(false, JC_CARRIER_TX1TX2);
		return -10000;
	}
	Util::setSleep(100);
	//���tx2����ƽ�ȶ�
	js = HwGetSig_Smooth(dd, JC_CARRIER_TX1);
	if (js <= -10000) {
		//�رչ���
		fnSetTxOn(false, JC_CARRIER_TX1TX2);
		if (js == JC_STATUS_ERROR_NO_FIND_POWER)
			Util::logged(L"���� TX2δ��⵽���ʣ���칦�������");
		else
			Util::logged(L"���� TX2����ƫ�����");
		return js;
	}
	//---------------------------------------------------------------------------------
	//�رչ���
	js = fnSetTxOn(false, JC_CARRIER_TX1TX2);
	if (0 != js) return js;
	//---------------------------------------------------------------------------------
	//��������Ƶ��
	double freq_pim_khz = __pobj->GetPimFreq();
	__pobj->ana->InstrSetCenterFreq(freq_pim_khz);

	return 0;
}

//��������
int fnSetTxOn(JcBool bOn, JcInt8 byCarrier){
	bool isSucc = false;
	bool isOn = bOn == 0 ? false : true;
	if (byCarrier == JC_CARRIER_TX1TX2){
		isSucc = __pobj->sig1->InstrOpenPow(isOn);
		if (!isSucc)
			return JC_STATUS_ERROR_SET_TX_ONOFF_FAIL;
		isSucc &= __pobj->sig2->InstrOpenPow(isOn);
	}
	else if (byCarrier == JC_CARRIER_TX1)
		isSucc = __pobj->sig1->InstrOpenPow(isOn);
	else if (byCarrier == JC_CARRIER_TX2)
		isSucc = __pobj->sig2->InstrOpenPow(isOn);

	if (isSucc)
		return 0;
	else
		return JC_STATUS_ERROR_SET_TX_ONOFF_FAIL;
}

int fnGetImResult(JC_RETURN_VALUE dFreq, JC_RETURN_VALUE dPimResult, const JC_UNIT cUnits) {
	double freq_pim_khz = __pobj->GetPimFreq();
	//��ȡ�ڲ�У׼
	double rxoff;
	JC_STATUS s = JcGetOffsetRx(rxoff, __pobj->now_band, __pobj->now_dut_port, freq_pim_khz / 1000);
	if (s) rxoff = 0;
	//��ȡ����
	//dPimResult = __pobj->ana->InstrGetAnalyzer(dFreq, false);
	JcSetAna_RefLevelOffset(rxoff);
	dPimResult = JcGetAna(freq_pim_khz, false);	
	//��������
	//dPimResult += rxoff;
	dFreq = __pobj->TransToUnit(freq_pim_khz, cUnits);
	if (dPimResult == JC_STATUS_ERROR){
		Util::logged(L"fnGetImResult: Spectrum Read Error!");
		__pobj->strErrorInfo = "Spectrum read error!\r\n";
		return JC_STATUS_ERROR_READ_SPECTRUM_FAIL;
	}
	return 0;
}

int fnSetSpan(int iSpan, const JC_UNIT cUnits) {
	if (NULL == __pobj) return JC_STATUS_ERROR;
	__pobj->ana->InstrSetSpan(__pobj->TransKhz(iSpan, cUnits) * 1000);
	return 0;
}

int fnSetRBW(int iRBW, const JC_UNIT cUnits) {
	if (NULL == __pobj) return JC_STATUS_ERROR;
	__pobj->ana->InstrSetRbw(__pobj->TransKhz(iRBW, cUnits) * 1000);
	return 0;
}

int fnSetVBW(int iVBW, const JC_UNIT cUnits) {
	if (NULL == __pobj) return JC_STATUS_ERROR;
	__pobj->ana->InstrSetVbw(__pobj->TransKhz(iVBW, cUnits) * 1000);
	return 0;
}

int fnSendCmd(JcInt8 byDevice, const JC_CMD cmd, char* cResult, long& lCount) {
	bool b = 0;
	int num = 0;
	std::string scmd(cmd);
	int n = scmd.find('?');
	switch (byDevice)
	{
	case 0://SIG1
		if (n > 0)
			num = __pobj->sig1->InstrWriteAndRead(cmd, cResult);
		else
			b = __pobj->sig1->InstrWrite(cmd);
		break;
	case 1://SIG2
		if (n > 0)
			num = __pobj->sig2->InstrWriteAndRead(cmd, cResult);
		else
			b = __pobj->sig2->InstrWrite(cmd);
		break;
	case 2://SA
		if (n > 0)
			num = __pobj->sen->InstrWriteAndRead(cmd, cResult);
		else
			b = __pobj->sen->InstrWrite(cmd);
		break;
	case 3://PM
		if (n > 0)
			num = __pobj->ana->InstrWriteAndRead(cmd, cResult);
		else
			b = __pobj->ana->InstrWrite(cmd);
		break;
	default:
		break;
	}

	return 0;
}

int fnGetSpectrumType(char* cSpectrumType) {
	long num =  __pobj->ana->InstrWriteAndRead("*IDN?\n", cSpectrumType);
	if (num > 0)
		return 0;
	else
		return JC_STATUS_ERROR_READ_SPECTRUM_IDN_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//HW-API
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HwSetBandEnable(int iBand, JcBool isEnable) {
	if (iBand < 0 || iBand > 6)
		return;
	_switch_enable[iBand] = isEnable;
}

void HwSetExit(){
	JcPimObject::release();
}

//vco���
JcBool FnGet_Vco() {
	//��ʼ����
	double real_val = 0;
	double vco_val = 0;
	return HwGet_Vco(real_val, vco_val);

}

void HwSetIsExtBand(JcBool isUse) {
	if (isUse == 0)
		__pobj->isUseExtBand = false;
	else
		__pobj->isUseExtBand = true;
}

//���õ�ǰ���ŵ������
JcBool HwSetCoup(JcInt8 byCoup) {
	int iSwitch = __pobj->now_band * 2 + __pobj->now_dut_port;
	JcBool r = JcSetSwitch(iSwitch, iSwitch, iSwitch, byCoup);	
	Util::setSleep(250);
	if (FALSE == r) 
		Util::logged(L"HwSetCoup: Switch-Coup-%d Error!", (int)byCoup);		
	
	return r;
}

//��ȡ��ǰ���Ź���ֵ(tx1��tx2)
double HwGetCoup_Dsp(JcInt8 byCoup) {
	double val = 0;
	double tx_temp = 0;
	if (byCoup == JC_COUP_TX1) {
		//����У׼������mhzΪ��λ��ע��ת��
		int s = JcGetOffsetTx(val, __pobj->now_band, __pobj->now_dut_port,
							  byCoup, OFFSET_DSP,
							  __pobj->now_txFreq1 / 1000, __pobj->now_txPow1);
		if (s) return s;
		//�����ⲿ����
		//val -= __pobj->offset_txPow1;
		tx_temp = __pobj->now_txPow1 + __pobj->offset_txPow1;
	}
	else if (byCoup == JC_COUP_TX2) {
		//����У׼������mhzΪ��λ��ע��ת��
		int s = JcGetOffsetTx(val, __pobj->now_band, __pobj->now_dut_port,
							  byCoup, OFFSET_DSP,
							  __pobj->now_txFreq2 / 1000, __pobj->now_txPow2);
		if (s) return s;
		//�����ⲿ����
		//val -= __pobj->offset_txPow2;
		tx_temp = __pobj->now_txPow2 + __pobj->offset_txPow2;
	}
	//��ȡ���ʼ�
	double sen = JcGetSen();
	sen += JcGetSen();
	sen += JcGetSen();
	//���㲹��
	sen = sen / 3 + val;
	//sen += val;

	std::string strLog = "start Dsp-Coup-" + std::to_string(byCoup) + "\r\n";
	strLog += "   Avg3rd_1: " + std::to_string(sen) + " \r\n";

	double dd = tx_temp - sen;
	if (dd > __pobj->now_tx_smooth_threasold || dd < (__pobj->now_tx_smooth_threasold * -1)) {
		Util::setSleep(100);
		//��2�κ�ƽ��
		sen = JcGetSen();
		sen += JcGetSen();
		sen += JcGetSen();
		sen = sen / 3 + val;
		strLog += "   Avg3rd_2: " + std::to_string(sen) + " \r\n";
		JcPimObject::Instance()->LoggingWrite(strLog.c_str());
	}
	return sen;
}

JcBool HwGet_Vco(double& real_val, double& vco_val) {
	double vco_freq_mhz = 1334 + 2 * (2 * __pobj->now_band + __pobj->now_dut_port);
	//OFFSET����
	__pobj->ana->InstrVcoSetting();
	__pobj->ana->InstrSetCenterFreq(vco_freq_mhz * 1000);
	Util::setSleep(100);
	//��ʼ����
	real_val = __pobj->ana->InstrGetAnalyzer(vco_freq_mhz * 1000, true);
	__pobj->ana->InstrPimSetting();

	JcGetOffsetVco(vco_val, __pobj->now_band, __pobj->now_dut_port);
	double dd = real_val - vco_val;

	//if (dd > SMOOTH_VCO_THREASOLD || dd < (SMOOTH_VCO_THREASOLD*-1))
	if (dd > __pobj->now_vco_threasold || dd < (__pobj->now_vco_threasold * -1))
		return 0;
	else
		return 1;
}

//��⹦���ȶ���(���빦�ſ�������) return dd
JC_STATUS HwGetSig_Smooth(JC_RETURN_VALUE dd, JcInt8 byCarrier){
	double tx_dsp = 0;
	dd = 0;
	double tx_deviate = 0;
	std::string strLog = "start smooth-tx-" + std::to_string(byCarrier) + "\r\n";

	for (int i = 0; i < 4; i++){
		if (i == 0)
			Util::setSleep(100);
		if (byCarrier == JC_CARRIER_TX1) {
			//��ȡ��⹦��
			tx_dsp = HwGetCoup_Dsp(JC_COUP_TX1);
			//��ȡƫ��ֵ
			tx_deviate = __pobj->now_txPow1 + __pobj->offset_txPow1 - tx_dsp;	
		}
		else if (byCarrier == JC_CARRIER_TX2) {
			tx_dsp = HwGetCoup_Dsp(JC_COUP_TX2);
			tx_deviate = __pobj->now_txPow2 + __pobj->offset_txPow2 - tx_dsp;
		}
		else
			return JC_STATUS_ERROR_SET_BOSH_USE_TX1TX2;

		strLog += "   No.: " + std::to_string(i) + "\r\n";
		strLog += "   tx_dsp: " + std::to_string(tx_dsp) + "\r\n";
		strLog += "   tx_deviate: " + std::to_string(tx_deviate) + "\r\n";
		strLog += "   dd: " + std::to_string(dd) + "\r\n";

		//δ��⹦��ʱ
		if (tx_dsp <= 33){
			__pobj->strErrorInfo = "   PowerSmooth: No find Power!\r\n";
			strLog += __pobj->strErrorInfo;
			JcPimObject::Instance()->LoggingWrite(strLog.c_str());
			return JC_STATUS_ERROR_NO_FIND_POWER;
		}

		if (tx_deviate > __pobj->now_tx_smooth_threasold || tx_deviate < (__pobj->now_tx_smooth_threasold * -1)) {
			__pobj->dd1 = 0;
			__pobj->dd2 = 0;
			//������󣬹رչ���
			//FnSetTxOn(false, byCarrier);		
			__pobj->strErrorInfo = "   PowerSmooth: Power's Smooth out Allowable Range\r\n";
			strLog += __pobj->strErrorInfo;
			JcPimObject::Instance()->LoggingWrite(strLog.c_str());
			return JC_STATUS_ERROR_SET_TX_OUT_SMOOTH;
		}
		else {
			if (tx_deviate >= (__pobj->now_tx_smooth_accuracy * -1) && tx_deviate <= __pobj->now_tx_smooth_accuracy)
				return JC_STATUS_SUCCESS;	

			if (i == 0)
				dd += tx_deviate * 0.9;
			else
				dd += (tx_deviate * 0.6);

			if (byCarrier == JC_CARRIER_TX1) {
				__pobj->dd1 = dd;
				JcBool b = JcSetSig(JC_CARRIER_TX1, __pobj->now_txFreq1, __pobj->now_txPow1 + __pobj->offset_txPow1 + __pobj->offset_internal_txPow1 + dd);	
				if (FALSE == b)
					return -10000;
			}
			else if (byCarrier == JC_CARRIER_TX2) {
				__pobj->dd2 = dd;
				JcBool b = JcSetSig(JC_CARRIER_TX2, __pobj->now_txFreq2, __pobj->now_txPow2 + __pobj->offset_txPow2 + __pobj->offset_internal_txPow2 + dd);
				if (FALSE == b)
					return -10000;
			}
			Util::setSleep(__pobj->debug_time);
		}
	}
	return JC_STATUS_SUCCESS;
}

//����Ƶ��
JC_STATUS HwSetTxFreqs(double dCarrierFreq1, double dCarrierFreq2, const JC_UNIT cUnits) {
	//��λת��
	__pobj->now_txFreq1 = __pobj->TransKhz(dCarrierFreq1, cUnits);
	__pobj->now_txFreq2 = __pobj->TransKhz(dCarrierFreq2, cUnits);

	//���ù���
	JC_STATUS js;
	js = JcSetSig_Advanced(JC_CARRIER_TX1, __pobj->now_band, __pobj->now_dut_port,
		__pobj->now_txFreq1, __pobj->now_txPow1,
		true, __pobj->offset_txPow1 + __pobj->dd1);
	if (js) return js;
	js = JcSetSig_Advanced(JC_CARRIER_TX2, __pobj->now_band, __pobj->now_dut_port,
		__pobj->now_txFreq2, __pobj->now_txPow2,
		true, __pobj->offset_txPow2 + __pobj->dd2);
	if (js) return js;
	//��������Ƶ��
	double freq_pim_khz = __pobj->GetPimFreq();
	__pobj->ana->InstrSetCenterFreq(freq_pim_khz);

	return JC_STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//��չAPI
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JcBool JcConnSig(JcInt8 byDevice, JC_UNIT cAddr) {
	ViSession viSession = VI_NULL;
	ViStatus s = viOpen(__pobj->viDefaultRM, cAddr, VI_NULL, VI_NULL, &viSession);
	if (s == VI_SUCCESS) {
		int index = JcGetIDN(viSession);
		//������
		if (index == INSTR_AG_MXG_SERIES){
			if (byDevice == JC_DEVICE::SIGNAL1) {
				__pobj->sig1 = std::make_shared<ClsSigAgN5181A>();
				__pobj->sig1->InstrSession(viSession);
			}
			else if (byDevice == JC_DEVICE::SIGNAL2) {
				__pobj->sig2 = std::make_shared<ClsSigAgN5181A>();
				__pobj->sig2->InstrSession(viSession);
			}
		}
		//�޵�˹�ߴ�
		else if (index == INSTR_RS_SM_SERIES) {
			if (byDevice == JC_DEVICE::SIGNAL1) {
				__pobj->sig1 = std::make_shared<ClsSigRsSMxSerial>();
				__pobj->sig1->InstrSession(viSession);
			}
			else if (byDevice == JC_DEVICE::SIGNAL2) {
				__pobj->sig2 = std::make_shared<ClsSigRsSMxSerial>();
				__pobj->sig2->InstrSession(viSession);
			}
		}
	}

	__pobj->now_status[byDevice] = !s;
	return !s;
}

JcBool JcConnAna(JC_UNIT cAddr) {
	ViSession viSession = VI_NULL;
	ViStatus s = viOpen(__pobj->viDefaultRM, cAddr, VI_NULL, VI_NULL, &viSession);
	if (s == VI_SUCCESS) {
		int index = JcGetIDN(viSession);
		//������
		if (index == INSTR_AG_MXA_SERIES) {
			__pobj->ana = std::make_shared<ClsAnaAgN9020A>();
			__pobj->ana->InstrSession(viSession);
		}
		//�޵�˹�ߴ�
		else if (index == INSTR_RS_FS_SERIES) {
			__pobj->ana = std::make_shared<ClsAnaRsFspSerial>();
			__pobj->ana->InstrSession(viSession);
		}
	}

	__pobj->now_status[JC_DEVICE::ANALYZER] = !s;
	return !s;
}

JcBool JcConnSen(JC_UNIT cAddr) {
	ViSession viSession = VI_NULL;
	bool isConn = false;
	//�������Ǳ�����
	ViStatus s = viOpen(__pobj->viDefaultRM, cAddr, VI_NULL, VI_NULL, &viSession);
	if (s == VI_SUCCESS) {
		int index = JcGetIDN(viSession);
		if (index == INSTR_AG_U2000_SERIES) {
			__pobj->sen = std::make_shared<ClsSenAgU2000A>();
			__pobj->sen->InstrSession(viSession);
		}
		else if (index == INSTR_RS_NRT_SERIES) {
			__pobj->sen = std::make_shared<ClsSenRsNrt>();
			__pobj->sen->InstrSession(viSession);
		}
		isConn = !s;
	}
	//RS�Ǳ�����
	else {
		//int index = INSTR_RS_NRPZ_SERIES;
		__pobj->sen = std::make_shared<ClsSenRsNrpz>();
		isConn = __pobj->sen->InstrConnect(cAddr);
	}

	__pobj->now_status[JC_DEVICE::SENSOR] = isConn;
	return isConn;
}

JcBool JcGetVcoDsp(JC_RETURN_VALUE vco, JcInt8 bySwitchBand) {
	if (NULL == __pobj) return false;

	double vco_freq_mhz = 1334 + 2 * bySwitchBand;
	__pobj->ana->InstrVcoSetting();
	Util::setSleep(100);
	vco = __pobj->ana->InstrGetAnalyzer(vco_freq_mhz * 1000, true);
	__pobj->ana->InstrPimSetting();
	return vco >= -90 ? true : false;
}
//��ȡ����
void JcGetError(char* msg, size_t max) {
	if (NULL == __pobj) return;

	size_t n = __pobj->strErrorInfo.size();
	if (n > max) n = max;
	memcpy(msg, __pobj->strErrorInfo.c_str(), n);
	__pobj->strErrorInfo = "Not";
}

//��ȡ����ģ��״̬
JcBool JcGetDeviceStatus(JcInt8 byDevice) {
	if (NULL == __pobj) return false;

	if (byDevice > 4 || byDevice < 0)
		return false;
	else if (byDevice == 4)
		return __pobj->isSwhConn;
	else
		return __pobj->now_status[byDevice];
}

//��ȡ�ⲿrefence״̬
JcBool JcGetSig_ExtRefStatus(JcInt8 byCarrier) {
	if (NULL == __pobj) return 0;
	bool isExt = false;

	if (byCarrier == JC_CARRIER_TX1)
		isExt = __pobj->sig1->InstrGetReferenceStatus();
	else if (byCarrier == JC_CARRIER_TX2)
		isExt = __pobj->sig2->InstrGetReferenceStatus();
	else if (byCarrier == JC_CARRIER_TX1TX2) {
		isExt = __pobj->sig1->InstrGetReferenceStatus();
		isExt &= __pobj->sig2->InstrGetReferenceStatus();
	}

	return isExt;
}

//���ù��Ų��� (�򿪹رչ�����ʹ��HwSetTxOn())
JcBool JcSetSig(JcInt8 byCarrier, double freq_khz, double pow_dbm) {
	if (NULL == __pobj) return FALSE;

	bool b = false;
	if (byCarrier == JC_CARRIER_TX1)
		b = __pobj->sig1->InstrSetFreqPow(freq_khz, pow_dbm);
	else if (byCarrier == JC_CARRIER_TX2)
		b = __pobj->sig2->InstrSetFreqPow(freq_khz, pow_dbm);	
	else if (byCarrier == JC_CARRIER_TX1TX2) {
		b = __pobj->sig1->InstrSetFreqPow(freq_khz, pow_dbm);
		b &= __pobj->sig2->InstrSetFreqPow(freq_khz, pow_dbm);
	}

	if (false == b)
		Util::logged(L"JcSetSig: (%d)Error!", (int)byCarrier);

	return b;
}
//���ù��Ų���(�߼�)
JC_STATUS JcSetSig_Advanced(JcInt8 byCarrier, JcInt8 byBand, JcInt8 byPort,
							double freq_khz, double pow_dbm,
							bool isOffset, double dExtOffset) {
	if (NULL == __pobj) return JC_STATUS_ERROR;
	if (byCarrier != JC_CARRIER_TX1 && byCarrier != JC_CARRIER_TX2) return JC_STATUS_ERROR_SET_BOSH_USE_TX1TX2;

	double tx_true = pow_dbm;
	double internal_offset = 0;
	if (isOffset) {
		//��ʼ��ȡ�ڲ�У׼
		JcInt8 coup = byCarrier - 1;
		//����У׼������mhzΪ��λ��ע��ת��
		int s = JcGetOffsetTx(internal_offset, byBand, byPort, coup, JC_OFFSET_REAL, freq_khz / 1000, pow_dbm);
		if (s) {
			__pobj->strErrorInfo = "SetTx" + std::to_string(byCarrier) + ": Read Offset's Data Error!\r\n";
			//���ش���
			return JC_STATUS_ERROR_GET_TX1_OFFSET - 1 + byCarrier;
		}

		//����ʵ�����ù���ֵ
		tx_true = pow_dbm + dExtOffset + internal_offset;
		if (tx_true > SIGNAL_SOURCE_MAX_POW) {
			__pobj->strErrorInfo = "SetTx" + std::to_string(byCarrier) + ": SIG's Power out range��Maybe Offset's Data Error����\r\n";
			return JC_STATUS_ERROR_SET_TX_OUT_RANGE;
		}
	}
	if (byCarrier == JC_CARRIER_TX1) {
		__pobj->offset_internal_txPow1 = internal_offset;
		bool b = __pobj->sig1->InstrSetFreqPow(freq_khz, tx_true);
		if (false == b) {
			Util::logged(L"JcSetSig: (%d)Error!", (int)byCarrier);
			return -10000;
		}
	}
	else if (byCarrier == JC_CARRIER_TX2){
		__pobj->offset_internal_txPow2 = internal_offset;
		bool b = __pobj->sig2->InstrSetFreqPow(freq_khz, tx_true);
		if (false == b) {
			Util::logged(L"JcSetSig: (%d)Error!", (int)byCarrier);
			return -10000;
		}
	}
	return JC_STATUS_SUCCESS;
}
//��ȡ��ǰSIG����ֵ(tx1��tx2) return sen
double JcGetSig_CoupDsp(JcInt8 byCoup, JcInt8 byBand, JcInt8 byPort,
						double freq_khz, double pow_dbm, double dExtOffset) {
	double val = 0;
	//����У׼������mhzΪ��λ��ע��ת��
	int s = JcGetOffsetTx(val, byBand, byPort,
						  byCoup, OFFSET_DSP,
						  freq_khz / 1000, pow_dbm);
	if (s) val = 0;
	//��ȡ���ʼ�
	double sen = JcGetSen();
	//���㲹��
	sen += val;
	sen -= dExtOffset;
	return sen;
}

//��ȡ���ʼ�
double JcGetSen() {
	if (NULL == __pobj) return JC_STATUS_ERROR;
	return __pobj->sen->InstrGetSesnor();
}


//����Ƶ��REF LEVEL OFFSET
void JcSetAna_RefLevelOffset(double offset) {
	if (NULL == __pobj) return;
	__pobj->ana->InstrSetOffset(offset);
}

//��ȡƵ��
double JcGetAna(double freq_khz, bool isMax){
	if (NULL == __pobj) return JC_STATUS_ERROR;
	return __pobj->ana->InstrGetAnalyzer(freq_khz, isMax);
}

//���ÿ���(iSwitchTx: 0 ~ 13)
JcBool JcSetSwitch(int iSwitchTx1, int iSwitchTx2,
				  int iSwitchPim, int iSwitchCoup) {
	if (NULL == __pobj) {
		__pobj->strErrorInfo = "object: Not init!\r\n";
		return false; 
	}
	if (false == __pobj->isSwhConn) {
		__pobj->strErrorInfo = "Switch: All not connected\r\n";
		return false;
	}

	//�����жϵ�ǰģ��
	int a, b, c;
	a = iSwitchTx1 / 2;
	b = iSwitchTx2 / 2;
	c = iSwitchPim / 2;

	bool isSucc = false;
	if (_switch_enable[a] && _switch_enable[b] && _switch_enable[c]) {
		//ת�������JC_COUP���
		int coup = 0;
		if (iSwitchTx1 % 2 == 0)
			coup = iSwitchTx1 + iSwitchCoup;
		else
			coup = (iSwitchTx1 - 1) + iSwitchCoup;
		isSucc = __pobj->swh->SwitchExcut(iSwitchTx1, iSwitchTx2, iSwitchPim, coup);
		if (!isSucc) __pobj->strErrorInfo = "Switch: Excut Error!\r\n";
	}
	else 
		__pobj->strErrorInfo = "Switch: This Module is not connected\r\n";

	return isSucc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Rx У׼API
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//��ȡRxУ׼
JC_STATUS JcGetOffsetRx(JC_RETURN_VALUE offset_val,
						JcInt8 byInternalBand, JcInt8 byDutPort,
						double freq_mhz){
	//��ȡ��ǰƵ����ʾ�ַ�
	std::string sband = __pobj->GetBandString(byInternalBand);
	//����У׼���ݵ�
	offset_val = __pobj->offset.OffsetRx(sband.c_str(), byDutPort, freq_mhz);
	if (offset_val == JC_STATUS_ERROR) {

		__pobj->strErrorInfo = "RxOffset: Read error!\r\n";
		return JC_STATUS_ERROR_GET_RX_OFFSET;
	}
	else 
		return JC_STATUS_SUCCESS;
}

long JcGetOffsetRxNum(JcInt8 byInternalBand){
	//��ȡ��ǰƵ����ʾ�ַ�
	std::string sband = __pobj->GetBandString(byInternalBand);
	//��ȡRxУ׼��
	double Rxfreq[256] = { 0 };
	int freq_num = __pobj->offset.FreqHeader(OFFSET_RX, sband.c_str(), Rxfreq, 256);
	return freq_num;
}

//�Զ�У׼����Rx (У׼ǰ��ȷ������)����ֻʹ��JC_TX1��У׼��
JC_STATUS JcSetOffsetRx(JcInt8 byInternalBand, JcInt8 byDutPort,
						double loss_db, Callback_Get_RX_Offset_Point pHandler) {

	//��ȡ��ǰƵ����ʾ�ַ�
	std::string sband = __pobj->GetBandString(byInternalBand);
	//��ȡRxУ׼��
	double Rxfreq[256] = {0};
	int freq_num = __pobj->offset.FreqHeader(OFFSET_RX, sband.c_str(), Rxfreq, 256);
	double off[256] = {0};
	__pobj->ana->InstrRxOffsetSetting();

	std::string strLog = "start offset-rx-" +
		std::to_string(byInternalBand) + "-" +
		std::to_string(byDutPort) + "\r\n";

	//���ñ���ֵ
	JcSetSig(JC_CARRIER_TX1, Rxfreq[0] * 1000, OFFSET_PROTECT_RX);
	//��������
	fnSetTxOn(true, JC_CARRIER_TX1);
	Util::setSleep(300);
	//VCO
	double vco = 0;
	if (JcGetVcoDsp(vco, byInternalBand * 2 + byDutPort) == false) {
		//__pobj->strErrorInfo = "RxOffset: vco < -90!\r\n";
		//return JC_STATUS_ERROR;
		//�����ش������
	}
	if (pHandler)
		pHandler(0, vco);
	JcSetOffsetVco(byInternalBand, byDutPort, vco);

	for (int i = 0; i < freq_num; ++i) {		
		//����
		JcSetSig(JC_CARRIER_TX1, Rxfreq[i] * 1000, OFFSET_PROTECT_RX);
		Util::setSleep(200);
		//��ȡ
		double v = JcGetAna(Rxfreq[i] * 1000, false);
		if (v == JC_STATUS_ERROR){
			//�رչ���
			fnSetTxOn(false, JC_CARRIER_TX1);
			__pobj->strErrorInfo = "Spectrum read error!\r\n";
			return JC_STATUS_ERROR_READ_SPECTRUM_FAIL;
		}
		//�������: Ŀ��ֵ��OFFSET_PROTECT_RX�� = ʵ��ֵ��v�� + У׼ֵ ��off�� +����loss_db��
		off[i] = OFFSET_PROTECT_RX - v - loss_db;
		if (off[i] > 10 || off[i] < -10){
			//���󣬹رչ���
			fnSetTxOn(false, JC_CARRIER_TX1);
			//__pobj->ana->InstrSetAvg(2);
			__pobj->strErrorInfo = "   RxOffset: No Find Power(-90)!\r\n";
			strLog += __pobj->strErrorInfo;
			JcPimObject::Instance()->LoggingWrite(strLog.c_str());
			return JC_STATUS_ERROR;
		}
		//��ʼ�ص�
		if (pHandler)
			pHandler(Rxfreq[i], off[i]);
#ifdef JC_OFFSET_RX_DEBUG
		std::cout << "Freq = " << Rxfreq[i] << "MHz " << std::endl;
		std::cout << "Now = " << v << "dBm ; Off = " << off[i] << std::endl;
#endif
	}
	//�رչ���
	fnSetTxOn(false, JC_CARRIER_TX1);

	JC_STATUS s = __pobj->offset.Store_v2(OFFSET_RX, sband.c_str(), byDutPort, 0, 0, 0, off, freq_num);
	if (s) {
		__pobj->strErrorInfo = "RxOffset: Save Error!\r\n";
		return JC_STATUS_ERROR;
	}

	return JC_STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Tx У׼API
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//��ȡtxУ׼(band:��ǰƵ�Σ�dutport:��ǰ���Զ˿ڣ�coup:��ǰ���Թ���)
JC_STATUS JcGetOffsetTx(JC_RETURN_VALUE offset_val,
						JcInt8 byInternalBand, JcInt8 byDutPort,
						JcInt8 coup, JcInt8 real_or_dsp,
						double freq_mhz, double tx_dbm) {

	//��ȡ��ǰƵ����ʾ�ַ�
	std::string sband = __pobj->GetBandString(byInternalBand);
	//����У׼���ݵ�
	offset_val = __pobj->offset.OffsetTx(sband.c_str(), byDutPort, coup, real_or_dsp, freq_mhz, tx_dbm);
	if (offset_val == JC_STATUS_ERROR){
		offset_val = 0;
		__pobj->strErrorInfo = "TxOffset: Read data error!\r\n";
		return JC_STATUS_ERROR;
	}
	else 
		return JC_STATUS_SUCCESS;
}

long JcGetOffsetTxNum(JcInt8 byInternalBand) {
	//��ȡ��ǰƵ����ʾ�ַ�
	std::string sband = __pobj->GetBandString(byInternalBand);
	//��ȡTxУ׼Ƶ��
	double txfreq[256] = { 0 };
	int freq_num = __pobj->offset.FreqHeader(OFFSET_TX, sband.c_str(), txfreq, 256);
	return freq_num;
}

//�Զ�У׼����Tx (У׼ǰ��ȷ������)��(band:��ǰƵ�Σ�dutport:��ǰ���Զ�kou), ͬʱУ׼tx1,tx2
JC_STATUS JcSetOffsetTx(JcInt8 byInternalBand, JcInt8 byDutPort,
						double des_p_dbm, double loss_db,
						Callback_Get_TX_Offset_Point pHandler) {

	if (__pobj->ext_sen_index == 1){
		//to do
		//�ⲿ������ʹ��
	}
	else {
		//Ĭ�Ϸ�ʽ��������Ƶ��
		__pobj->ana->InstrTxOffsetSetting();
		Util::setSleep(500);
	}

	//��ȡ��ǰƵ����ʾ�ַ�
	std::string sband = __pobj->GetBandString(byInternalBand);
	//��ȡTxУ׼Ƶ��
	double txfreq[256] = { 0 };
	int freq_num = __pobj->offset.FreqHeader(OFFSET_TX, sband.c_str(), txfreq, 256);

	double off_real[256] = { 0 };
	double off_dsp[256] = { 0 };

	//Bandת�����ز���
	int iswitch = byInternalBand * 2 + byDutPort;
	//----------------------------------------------------------------------------------------------
	//coup ==> JC_COUP_TX1 to JC_COUP_TX2
	for (int coup = 0; coup < 2; ++coup)
	{
#ifdef JC_OFFSET_TX_DEBUG
		std::cout << "\n";
		std::cout << "Start: " << coup << std::endl;
#endif
		//----------------------------------------------------------------------------------------------
		//�л�����
		JcBool isSwhConn = JcSetSwitch(iswitch, iswitch, iswitch, coup);
		if (isSwhConn == FALSE) {
			__pobj->strErrorInfo = "TxOffset: Switch-Coup Fail!\r\n";
			return JC_STATUS_ERROR;
		}
		//----------------------------------------------------------------------------------------------
		//��������,���ñ���ֵ
		std::shared_ptr<IfSignalSource> pow = coup == JC_COUP_TX1 ? __pobj->sig1 : __pobj->sig2;
		pow->InstrSetFreqPow(txfreq[0] * 1000, OFFSET_PROTECT_TX);
		pow->InstrOpenPow(true);
		//----------------------------------------------------------------------------------------------
		Util::setSleep(400);
		//��ʼ �Զ����� TX1 У׼����
		for (int i = 0; i < freq_num; ++i) {
			//����У׼
			int s = JcSetOffsetTx_Single(off_real[i], off_dsp[i], coup, des_p_dbm, txfreq[i], loss_db);
			if (s) return s;
			//��ʼ�ص�
			if(pHandler)
				pHandler(txfreq[i], off_real[i], off_dsp[i]);
#ifdef JC_OFFSET_TX_DEBUG
			std::cout << "Run: " << txfreq[i] << " MHz" << std::endl;
			std::cout << "Off = " << off_real[i] << " ; Sen= " << off_dsp[i] << std::endl;
#endif
		}
		//----------------------------------------------------------------------------------------------
		//�رչ���
		pow->InstrOpenPow(false);
		//----------------------------------------------------------------------------------------------
		//�洢У׼����
		int s = __pobj->offset.Store_v2(OFFSET_TX, sband.c_str(), byDutPort, coup, JC_OFFSET_REAL, des_p_dbm, off_real, freq_num);
		if (s) {
			__pobj->strErrorInfo = "TxOffset: Tx1's data save error!\r\n";
			return JC_STATUS_ERROR;
		}
		s = __pobj->offset.Store_v2(OFFSET_TX, sband.c_str(), byDutPort, coup, JC_OFFSET_DSP, des_p_dbm, off_dsp, freq_num);
		if (s) {
			__pobj->strErrorInfo = "TxOffset: Tx2's data save error!\r\n";
			return JC_STATUS_ERROR;
		}
		//----------------------------------------------------------------------------------------------
#ifdef JC_OFFSET_TX_DEBUG
		std::cout << "Save Success!" << std::endl;
#endif
	}

	//��ԭƵ������
	if (__pobj->ext_sen_index == 0)
		__pobj->ana->InstrPimSetting();
	
	return JC_STATUS_SUCCESS;
}

JC_STATUS JcSetOffsetTx_Single(JC_RETURN_VALUE resulte,
							   JC_RETURN_VALUE resulte_sen,
							   int coup, 
							   double des_p_dbm, 
							   double des_f_mhz, 
							   double loss_db) {
	std::shared_ptr<IfSignalSource> pow = coup == JC_COUP_TX1 ? __pobj->sig1 : __pobj->sig2;
	//���ñ���ֵ
	pow->InstrSetFreqPow(des_f_mhz * 1000, OFFSET_PROTECT_TX);
	//-------------------------------------------------------------------------------------
	//pow->InstrOpenPow(true);
	//-------------------------------------------------------------------------------------

	if (__pobj->ext_sen_index == 1){
		//to do
		//�ⲿ������ʹ��
	}
	else {
		//Ĭ�Ϸ�ʽ��������Ƶ��		
		__pobj->ana->InstrTxOffsetSetting();
	}

	double p_true = OFFSET_PROTECT_TX;
	resulte = 0;
	std::string strLog = "start offset-tx\r\n";

	for (int i = 0; i < 6; i++) {
		//����
		pow->InstrSetFreqPow(des_f_mhz * 1000, p_true);
		Util::setSleep(100);
		//��ȡ
		double v = -10000;
		double r = 0;

		for (size_t a = 0; a < 3; a++) {
			if (__pobj->ext_sen_index == 1)
				//�ⲿ����������ȡ��ֵ
				v = __pobj->ext_sen->InstrGetSesnor();
			else {
				//Ĭ�Ϸ�ʽ����ȡƵ��
				v = JcGetAna(des_f_mhz * 1000, false);
				if (v == JC_STATUS_ERROR){
					//�رչ���
					fnSetTxOn(false, JC_CARRIER_TX1);
					__pobj->strErrorInfo = "Spectrum read error!\r\n";
					return JC_STATUS_ERROR_READ_SPECTRUM_FAIL;
				}
			}

			strLog += "   freq: " + std::to_string(des_f_mhz) +
						"  val: " + std::to_string(v) + "\r\n";
			
			//�ж�1
            if (v <= -50) {
                Util::setSleep(1000);
                continue;
            }
	
			//�ж�2
			r = des_p_dbm - (v + loss_db);
			double temp = p_true + r;
			if (p_true >= SIGNAL_SOURCE_MAX_POW){
				Util::setSleep(1000);
				continue;
			}
			else
				break;
		}

		//���
		if (v <= -50) {
			pow->InstrOpenPow(false);
			__pobj->strErrorInfo = "   TxOffset: This Channel can not find Power!\r\n";
			strLog += __pobj->strErrorInfo;
			JcPimObject::Instance()->LoggingWrite(strLog.c_str());
			if (v <= -10000)
				__pobj->strErrorInfo = "TxOffset: External Sensor read error!\r\n";
			return JC_STATUS_ERROR;
		}

#ifdef JC_OFFSET_TX_SINGLE_DEBUG
		std::cout << "No: " << i << std::endl;
		std::cout << "Set: " << des_f_mhz << " Mhz, " << p_true << "dBm\n";
		std::cout << "Get: " << v << std::endl;
		std::cout << "r = " << r << std::endl;
#endif

		//TX����: Ŀ��ֵ��des_p_dbm�� + У׼ֵ��resulte�� = ��ʵ��ֵ��p_true��
		resulte = p_true - des_p_dbm;
		if (r <= OFFSET_TX_THREASOLD && r >= (OFFSET_TX_THREASOLD * -1))
			break;
		
		if (i >= 4)
			r = r / 2;
		else
			p_true += (r * 3 / 4);

		if (p_true >= SIGNAL_SOURCE_MAX_POW) {
			pow->InstrOpenPow(false);
			__pobj->strErrorInfo = "   TxOffset: This Channel's power so big!\r\n";
			strLog += __pobj->strErrorInfo;
			JcPimObject::Instance()->LoggingWrite(strLog.c_str());
			return JC_STATUS_ERROR_SET_TX_OUT_RANGE;
		}	
	}
	Util::setSleep(300);
	double sen = JcGetSen();
	sen += JcGetSen();
	sen += JcGetSen();
	
	//TX_DSP����: ʵ��ֵ��sen�� + У׼ֵ��resulte_sen�� = ����ʾֵ��des_p_dbm��
	resulte_sen = des_p_dbm - sen / 3;
	//-------------------------------------------------------------------------------------
	//�رչ���
	//pow->InstrOpenPow(false);
	//-------------------------------------------------------------------------------------
#ifdef JC_OFFSET_TX_SINGLE_DEBUG
	std::cout << "sen = " << resulte_sen << std::endl;
	std::cout << "Complete Offset!" << std::endl;
#endif
	return JC_STATUS_SUCCESS;
}


JC_STATUS JcGetOffsetVco(JC_RETURN_VALUE offset_vco, JcInt8 byInternalBand, JcInt8 byDutport) {
	std::string sband = __pobj->GetBandString(byInternalBand);

	offset_vco = __pobj->offset.OffsetVco(sband.c_str(), byDutport);
	if (offset_vco <= -10000) {
		offset_vco = 0;
		return JC_STATUS_ERROR;
	}
	else
		return JC_STATUS_SUCCESS;
}

JC_STATUS JcSetOffsetVco(JcInt8 byInternalBand, JcInt8 byDutport, double val) {
	std::string sband = __pobj->GetBandString(byInternalBand);

	int s = __pobj->offset.Store_vco_single(sband.c_str(), byDutport, val);
	if (s){
		__pobj->strErrorInfo = "VCO_Offset: VCO's data save error!\r\n";
		return JC_STATUS_ERROR;
	}
	else
		return JC_STATUS_SUCCESS;
}

JcBool JcSetOffsetTX_Config(int iAnalyzer, const JC_ADDRESS Device_Info) {
	__pobj->ext_sen_index = iAnalyzer;
	if (__pobj->ext_sen_index == 0) 
		return true;
	else if (__pobj->ext_sen_index == 1) {
		if (__pobj->isExtSenConn) return __pobj->isExtSenConn;
		//��ʼ����
		ViSession ext_visession;
		int index_sensor;
		//visa�Ǳ�����
		ViStatus s = viOpen(__pobj->viDefaultRM, const_cast<char*>(Device_Info), VI_NULL, 5000, &ext_visession);
		if (s == VI_SUCCESS) {
			__pobj->isExtSenConn = true;
			index_sensor = JcGetIDN(ext_visession);
			if (index_sensor == INSTR_AG_U2000_SERIES) {
				__pobj->ext_sen = std::make_shared<ClsSenAgU2000A>();
				__pobj->ext_sen->InstrSession(ext_visession);
			}
			else if (index_sensor == INSTR_RS_NRT_SERIES) {
				__pobj->ext_sen = std::make_shared<ClsSenRsNrt>();
				__pobj->ext_sen->InstrSession(ext_visession);
			}
		}
		//RS�Ǳ�����
		else {
			index_sensor = INSTR_RS_NRPZ_SERIES;
			__pobj->ext_sen = std::make_shared<ClsSenRsNrpz>();
			__pobj->isExtSenConn = __pobj->ext_sen->InstrConnect(Device_Info);
		}
	}
	else {
		__pobj->ext_sen_index = 0;
		__pobj->isExtSenConn = false;
	}

	return __pobj->isExtSenConn;
}

void JcSetOffsetTX_Config_Close() {
	if (__pobj->isExtSenConn) {
		__pobj->ext_sen->InstrClose();
		__pobj->ext_sen.reset();
		__pobj->isExtSenConn = false;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//OTHER FUNC
//////////////////////////////////////////////////////////////////////////////////////////////

int JcGetIDN(unsigned long viSession) {
	int iDeviceIDN = -1;
	unsigned char buf[128] = { 0 };
	unsigned long retCount = 0;
	viQueryf(viSession, "*IDN?\n", "%#t", &retCount, buf);

	if (retCount) {
		std::istringstream iss((char*)buf);
		std::vector<std::string> vinfo;
		std::string stemp;
		while (std::getline(iss, stemp, ',')) {
			vinfo.push_back(stemp);
		}

		//���ʼ�
		if (vinfo[1] == "U2000A" || vinfo[1] == "U2001A" || vinfo[1] == "U2002A")
			iDeviceIDN = INSTR_AG_U2000_SERIES;
		else if (vinfo[1] == "NRT01" || vinfo[1] == "NRT02" || vinfo[1] == "NRT03")
			iDeviceIDN = INSTR_RS_NRT_SERIES;
		else if (vinfo[1] == "NRPZ")
			iDeviceIDN = INSTR_RS_NRPZ_SERIES;
		//�ź�Դ
		else if (vinfo[1] == "N5171A" || vinfo[1] == "N5172A" || vinfo[1] == "N5181A" || vinfo[1] == "N5182A" || vinfo[1] == "N5183A")
			iDeviceIDN = INSTR_AG_MXG_SERIES;
		else if (vinfo[1] == "SMA100A" || vinfo[1] == "SMB100A" || vinfo[1] == "SMC100A" || vinfo[1] == "SMU200A")
			iDeviceIDN = INSTR_RS_SM_SERIES;
		//Ƶ����
		else if (vinfo[1] == "N9000A" || vinfo[1] == "N9010A" || vinfo[1] == "N9020A" || vinfo[1] == "N9030A" || vinfo[1] == "N9038A")
			iDeviceIDN = INSTR_AG_MXA_SERIES;
		else if (vinfo[1] == "FSP" || vinfo[1] == "FSU" || vinfo[1] == "FSV")
			iDeviceIDN = INSTR_RS_FS_SERIES;
	}
	return iDeviceIDN;
}

int JcGetSwtichEnable(int byInternalBandIndex){
	if (byInternalBandIndex < 0 || byInternalBandIndex > 6)
		return 0;
	return _switch_enable[byInternalBandIndex];
}

int JcGetDllVersion(int &major, int &minor, int &build, int &revision) {
	
	DWORD   verBufferSize;
	char    verBuffer[2048];

	std::wstring wPath = _startPath + L"\\JcPimMultiBandV2.dll";
	verBufferSize = GetFileVersionInfoSize(wPath.c_str(), NULL);
	if (verBufferSize > 0 && verBufferSize <= sizeof(verBuffer))
	{
		if (TRUE == GetFileVersionInfo(wPath.c_str(), NULL, verBufferSize, verBuffer))
		{
			UINT length;
			VS_FIXEDFILEINFO *verInfo = NULL;

			if (TRUE == VerQueryValue(
				verBuffer,
				TEXT("\\"),
				reinterpret_cast<LPVOID*>(&verInfo),
				&length))
			{
				major = HIWORD(verInfo->dwProductVersionMS);
				minor = LOWORD(verInfo->dwProductVersionMS);
				build = HIWORD(verInfo->dwProductVersionLS);
				revision = LOWORD(verInfo->dwProductVersionLS);
				return true;
			}
		}
	}

	return 0;
}

void JcFindRsrc() {
	char instrAddr[VI_FIND_BUFLEN];
	ViUInt32 num;
	ViFindList findlist;
	ViSession _defaultRm;
	ViStatus _status;
	
	_status = viOpenDefaultRM(&_defaultRm);
	_status = viFindRsrc(_defaultRm, "?*INSTR", &findlist, &num, instrAddr);
	
	std::cout << "Num:  " << num << std::endl;
	std::cout << "Addr: " << instrAddr << std::endl;
	
	while (--num)
	{
		_status = viFindNext(findlist, instrAddr);
		std::cout << "Addr: " << instrAddr << std::endl;
	}
	
	_status = viClose(_defaultRm);
	fflush(stdin);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//VNA ���� ��(������)
//////////////////////////////////////////////////////////////////////////////////////////////


void testcb(Callback_Get_RX_Offset_Point pHandler) {
	double d = 1;
	double f = 930;

	for (int i = 0; i < 5; ++i) {
		d += i;
		f++;
		Util::setSleep(1000);
		if (pHandler != 0)
			pHandler(f, d);
	}
}

int gettestval(int a, int b) {
	double d = 637;
	return a + b;
}
