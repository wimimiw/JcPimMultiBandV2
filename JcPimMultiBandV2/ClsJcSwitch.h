#ifndef _CLS_JCSWITCH_H_
#define _CLS_JCSWITCH_H_

#include "Switch/com_io_ctl.h"
#include "IfSwitch.hpp"

class ClsJcSwitch: Implements_ IfSwitch
{
public:
	ClsJcSwitch();
	~ClsJcSwitch();

public:
	//���س�ʼ��
	bool SwitchInit();
	//���ÿ���Enable
	void SwitchSetEnable(int iIndex, bool isEnable);
	//��ȡ��Ϣ
	std::string SwitchGetInfo();
	//��ȡ�豸������Ϣ
	void SwitchGetInfo(std::string& sInfo, int iIndex);
	//��ʼ����
	bool SwitchConnect();
	//����ִ���л�
	bool SwitchExcut(const int& iSwitchTx1, const int& iSwitchTx2, const int& iSwitchPim, const int& iSwitchDet);
	//�ͷ���Դ
	void SwitchClose();

private:
	bool CheckStatus();

private:
	bool _isConn;
	ns_com_io_ctl::com_io_ctl _cic;
	std::map<std::string, ns_com_io_ctl::com_io_ctl::stHostControl> _hosts;

	std::vector<std::string> _moduleList;
	std::vector<std::string> _nltx1;
	std::vector<std::string> _nltx2;
	std::vector<std::string> _nlpim;
	std::vector<std::string> _nldet;

	std::string _info;
};
#endif