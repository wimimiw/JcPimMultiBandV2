//
//  JcOffsetDB.h
//  MyTest
//
//  Created by San on 14/12/4.
//  Copyright (c) 2014�� ___SAN___. All rights reserved.
//

#ifndef __SAN__JcOffsetDB__
#define __SAN__JcOffsetDB__

#include "stdafx.h"
#include "3rdParty/sqlite3.h"

#define JCOFFSET_ERROR -10000

#define OFFSET_TX 0
#define OFFSET_RX 1
#define OFFSET_REAL 0
#define OFFSET_DSP 1

//#define JC_SQL_DEBUG

class JcOffsetDB
{
public:
    JcOffsetDB()
		:_pConn(NULL)
		,_bConn(false)
		,_sTxFreqtable("TX_FREQ_LIST")
		, _mode(MODE_HUAWEI)
    {
        //char col_types[][10] = { "" ,"INTEGER", "FLOAT", "Text", "BLOB", "NULL"};
    }
    
    ~JcOffsetDB()
	{
        if (_pConn) {
            sqlite3_close(_pConn);
        }
    }
    
public:
	bool DbConnect(const char* addr);
	void DbInit(uint8_t mode) { _mode = mode; }
	int FreqBand(const uint8_t& tx_or_rx, const char* band, double& f_start, double& f_stop);
    //��ȡУ׼ͷ��Ϣ
	int FreqHeader(const char& tx_or_rx, const char* band, double* freq, int maxnum);
    //��ȡTxУ׼����
	double OffsetTx(const char* band, const char& dut, const char& coup,
					const char& real_or_dsp,
					const double& freq_mhz, const double& tx_dbm);
	//��ȡRxУ׼����
	double OffsetRx(const char* band, const char& dut, const double& freq_now);
	//��ȡvco����
	double OffsetVco(const char* band, const char& dut);
	//�洢У׼����
	int Store_v2(const char& tx_or_rx,
				 const char* band, const char& dut, const char& coup,
				 const char& real_or_dsp,
				 const double tx,
				 const double* val, int num);
	//�洢vcoУ׼����
	int Store_vco_single(const char* band, const char& dut, const double val);

private:
    //����б��
	double SumSlope(double v, double x1, double y1, double x2, double y2);
    //sqlite���ִ��
	int GetSqlVal(const char* strsql, double& a1, double& a2);
    
private:
    sqlite3* _pConn;
    bool _bConn;
    std::string _sTxFreqtable;
	uint8_t _mode;
};
#endif /* defined(__MyTest__JcOffsetDB__) */
