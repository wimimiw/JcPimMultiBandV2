#ifndef _CLS_AGILENT_HPP_
#define _CLS_AGILENT_HPP_

#include "stdafx.h"
#include "MyUtil\JcCommonAPI.h"

class ClsInstrAgilent
{
public:
	ClsInstrAgilent()
		:_viStatus(VI_NULL),
		_viDefaultRM(VI_NULL),
		_viSession(VI_NULL),
		_isConn(false),
		_esr(0)
	{}

	~ClsInstrAgilent() {}

public:
	//��ʼ����
	bool AgConnect(const char* c_addr) {
		if (_isConn) return true;

		_viStatus = viOpenDefaultRM(&_viDefaultRM);
		_viStatus = viOpen(_viDefaultRM, const_cast<char*>(c_addr), VI_NULL, 3000, &_viSession);

		//if (_viStatus == VI_SUCCESS)
		//	AgWrite("*RST\n");

		_isConn = !_viStatus;
		return _isConn;
	}

	//��ʼ����
	void AgSession(ViSession viConnectedSession) {
		//_viDefaultRM = viRM;
		_viSession = viConnectedSession;

		//if (_viSession != VI_NULL)
		//	AgWrite("*RST\n");

		_isConn = true;
	}

	//д������
	bool AgWrite(const char* c_cmd) {
		_viStatus = viPrintf(_viSession, const_cast<char*>(c_cmd));
		return !_viStatus;
	}

	//д�벢�ȴ���ȡ�����ض�ȡ�ֽڳ��ȣ�
	long AgWriteAndRead(const char* c_cmd, char* rbuf) {
		//��ʼд��
		_viStatus = viPrintf(_viSession, const_cast<char*>(c_cmd));
		if (_viStatus) return 0;
		//��ʼ��ȡ
		unsigned char buf[128] = {0};
		unsigned long retCount = 0;
		_viStatus = viRead(_viSession, buf, 128, &retCount);

		//��%#t���ĸ�ʽ����ͨ��
		//char buf[128] = {0};
		//unsigned long retCount = 0;
		//viQueryf(_viSession, const_cast<char*>(c_cmd), "%#t", &retCount, buf);

		if (retCount) 
			std::memcpy(rbuf, buf, retCount);

		return retCount;
	}

	//��������״��
	bool AgConnStatus() const {
		return _isConn;
	}

	//���ش�����Ϣ
	ViStatus AgError() const {
		return _viSession;
	}

	void AgClose() {
		_viStatus = viClose(_viSession);
		_isConn = false;
	}

	//�ȴ�
	bool AgWait() {
		viPrintf(_viSession, "*OPC\n");
		_esr = 0;
		for (int i = 0; i <= 500; i++)
		{
			if (i == 500)
				return false;

			viQueryf(_viSession, "*ESR?\n", "%ld", &_esr);
			if (_esr & 1)
				break;

			Util::setSleep(20);
		}
		return true;
	}

	void PrintError(long err)
	{
		if (err < VI_SUCCESS)
		{
			unsigned long retCount = 0;
			unsigned char  error_message[256] = { 0 };

			_viStatus = viPrintf(_viSession, "SYST:ERR?\n");
			_viStatus = viRead(_viSession, error_message, 256, &retCount);
			//viQueryf(_viSession, "")
			printf("Error: %s\n", error_message);
		}

	}

protected:
	bool _isConn;
	ViStatus _viStatus;
	ViSession _viDefaultRM;
	ViSession _viSession;
	ViUInt16 _esr;
};

#endif