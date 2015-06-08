#include "JcOffsetDB.h"

//У׼����1M
#define OFFSET_STEP 1

bool JcOffsetDB::DbConnect(const char* addr) {
	_bConn = !sqlite3_open(addr, &_pConn);
	return _bConn;
}

int JcOffsetDB::FreqBand(const uint8_t& tx_or_rx, const char* band, double& f_start, double& f_stop) {
	if (_mode == MODE_POI) {
		char sql[1024] = { 0 };
		if (tx_or_rx == OFFSET_TX)
			sprintf_s(sql, "select [tx_start],[tx_end] from [JC_BAND_INFO] where band = '%s'", band);
		else
			sprintf_s(sql, "select [rx_start],[rx_end] from [JC_BAND_INFO] where band = '%s'", band);

		if (GetSqlVal(sql, f_start, f_stop))
			return JCOFFSET_ERROR;
	}
	return 0;
}

int JcOffsetDB::FreqHeader(const char& tx_or_rx, const char* band, double* freq, int maxnum) {
	int i;
	std::string shead = tx_or_rx == OFFSET_TX ? "TX_" : "RX_";

	std::string sband(band);
	std::string stable = shead + sband;
	std::string sql = "select [" + sband + "] from [" + stable + "]";

	sqlite3_stmt* pstmt = NULL;
	sqlite3_prepare(_pConn, sql.c_str(), -1, &pstmt, NULL);

	for (i = 0; i < maxnum; ++i) {
		if (sqlite3_step(pstmt) == SQLITE_ROW) {

			double val = sqlite3_column_double(pstmt, 0);
			if (val != 0)
				*(freq + i) = val;
			else
				break;
#ifdef JC_SQL_DEBUG
			std::cout << val << "/";
#endif
		}
		else 
			break;
	}
#ifdef JC_SQL_DEBUG
	std::cout << "\n";
	std::cout << "Num:" << i << std::endl;
#endif

	//����POI��ȡƵ������
	if (_mode == MODE_POI) {
		double f_start, f_stop;
		char sql[1024] = { 0 };
		memset(freq, 0, sizeof(double)*maxnum);
		if (tx_or_rx == OFFSET_TX)
			sprintf_s(sql, "select [tx_start],[tx_end] from [JC_BAND_INFO] where band = '%s'", band);
		else
			sprintf_s(sql, "select [rx_start],[rx_end] from [JC_BAND_INFO] where band = '%s'", band);
		if (GetSqlVal(sql, f_start, f_stop))
			return JCOFFSET_ERROR;

		int num = ceil((f_stop - f_start) / OFFSET_STEP) + 1;
		num = num < maxnum ? num: maxnum;
		for (int j = 0; j < num; ++j) {
			*(freq + j) = f_start + OFFSET_STEP*j;
		}
		//���һ�㲻�ڲ�������ʱ���������һ��
		if ((f_start + (num -1)  * OFFSET_STEP) > f_stop)
			*(freq + num -1) = f_stop;
		i = num;
	}

	sqlite3_finalize(pstmt);
	return i;
}

//��ȡTxУ׼����
double JcOffsetDB::OffsetTx(const char* band, const char& dut, const char& coup,
							const char& real_or_dsp,
							const double& freq_mhz, const double& tx_dbm) {
	std::stringstream ss;
	std::string sSuffix = dut == 0 ? "_A" : "_B";
	sSuffix += (coup == 0 ? "_TX1" : "_TX2");

	//ѡ��У׼Ƶ�ʱ�TX_EGSM900, ѡ���� EGSM900
	//get(Freq) f1, f2!
	//��ѯfreq_now����������
	std::string stable = "TX_" + std::string(band);
	std::string scolomn(band);
	//����У׼Ƶ������f1,f2��Ƶ��ֵ
	double f1_temp, f2_temp;
	ss.str("");
	ss << "select Max_val, Min_val from (select max(" + scolomn +
		") Max_val from [" + stable + "] where " + scolomn +
		" <= " << freq_mhz << "), (select min(" + scolomn +
		") Min_val from [" + stable + "] where " + scolomn +
		" >= " << freq_mhz << ")";
	if (GetSqlVal(ss.str().c_str(), f1_temp, f2_temp))
		return JCOFFSET_ERROR;
#ifdef JC_SQL_DEBUG
	std::cout << "freq Range: " << f1 << ", " << f2 << std::endl;
#endif
	//����f1,f2��Ӧ��index
	double f1_index, f2_index;
	ss.str("");
	ss << "select A,B from (select [ID] A from [" << stable << "] where " + scolomn +
		"=" << f1_temp << "),(select [ID] B from [" << stable << "] where " + scolomn +
		"=" << f2_temp << ")";
	if (GetSqlVal(ss.str().c_str(), f1_index, f2_index))
		return JCOFFSET_ERROR;
#ifdef JC_SQL_DEBUG
	std::cout << "freq_index Range: " << f1_index << ", " << f2_index << std::endl;
#endif
	//����POI��ȡƵ������
	if (_mode == MODE_POI) {
		double f_start, f_stop;
		char sql[1024] = { 0 };
		sprintf_s(sql, "select [tx_start],[tx_end] from [JC_BAND_INFO] where band = '%s'", band);
		if (GetSqlVal(sql, f_start, f_stop))
			return JCOFFSET_ERROR;

		if (freq_mhz<f_start || freq_mhz>f_stop)
			return JCOFFSET_ERROR;

		f1_index = floor((freq_mhz - f_start) / OFFSET_STEP) + 1;
		f2_index = ceil ((freq_mhz - f_start) / OFFSET_STEP) + 1;
		f1_temp = f_start + OFFSET_STEP * (f1_index - 1);
		f2_temp = f_start + OFFSET_STEP * (f2_index - 1);
	}

	double freq1 = f1_temp;
	double freq2 = f2_temp;
	double f1 = f1_index;
	double f2 = f2_index;
	double tx1 = 0;
	double tx2 = 0;

	//ѡ��У׼���ݱ� EGSM900, ѡ���� Power
	//get(TX) tx1, tx2!
	//��ѯtx_now����������
	stable = "JC_TX_OFFSET_ALL";
	//���ø�������Port
	std::string sport = std::string(band) + sSuffix;
	//���ø�������Dsp
	int idsp = real_or_dsp;
	scolomn = "Power";
	ss.str("");
	ss << "select Max_val, Min_val from (select max(" + scolomn +
		") Max_val from [" + stable + "] where " + scolomn +
		" <= " << tx_dbm << " and Port = '" + sport + "' and Dsp = " << idsp << "), (select min(" + scolomn +
		") Min_val from [" + stable + "] where " + scolomn +
		" >= " << tx_dbm << " and Port = '" + sport + "' and Dsp = " << idsp << ")";
	if (GetSqlVal(ss.str().c_str(), tx1, tx2))
		return JCOFFSET_ERROR;
#ifdef JC_SQL_DEBUG
	std::cout << "tx Range: " << tx1 << ", " << tx2 << std::endl;
#endif

	double y1 = 0;
	double y2 = 0;
	double z1 = 0;
	double z2 = 0;

	//get y1, y2!
	//��ѯ����2�����ֵ(tx1,f1),(tx1,f2)
	ss.str("");
	ss << "select Val1, Val2 from (select [" << f1 << "] Val1 from [" + stable +
		"] where Power = " << tx1 << " and Port = '" + sport + "' and Dsp = " << idsp << "), (select [" << f2 << "] Val2 from [" + stable +
		"] where Power = " << tx1 << " and Port = '" + sport + "' and Dsp = " << idsp << ")";
	if (GetSqlVal(ss.str().c_str(), y1, y2))
		return JCOFFSET_ERROR;
#ifdef JC_SQL_DEBUG
	std::cout << "y1 = " << y1 << ", y2 = " << y2 << std::endl;
#endif

	//get y3, y4!
	//��ѯ����2�����ֵ(tx2,f1),(tx2,f2)
	ss.str("");
	ss << "select Val1, Val2 from (select [" << f1 << "] Val1 from [" + stable +
		"] where Power = " << tx2 << " and Port = '" + sport + "' and Dsp = " << idsp << "), (select [" << f2 << "] Val2 from [" + stable +
		"] where Power = " << tx2 << " and Port = '" + sport + "' and Dsp = " << idsp << ")";
	if (GetSqlVal(ss.str().c_str(), z1, z2))
		return JCOFFSET_ERROR;
#ifdef JC_SQL_DEBUG
	std::cout << "z1 = " << z1 << ", z2 = " << z2 << std::endl;
#endif

	//��ʼ����offset
	double offset1 = SumSlope(freq_mhz, freq1, y1, freq2, y2);
	double offset2 = SumSlope(freq_mhz, freq1, z1, freq2, z2);
#ifdef JC_SQL_DEBUG
	std::cout << "offset1 = " << offset1 << std::endl;
	std::cout << "offset2 = " << offset2 << std::endl;
#endif

	return SumSlope(tx_dbm, tx1, offset1, tx2, offset2);
}

//��ȡRxУ׼����
double JcOffsetDB::OffsetRx(const char* band, const char& dut, const double& freq_now) {
	std::stringstream ss;
	std::string sSuffix = dut == 0 ? "_A" : "_B";

	//ѡ��У׼Ƶ�ʱ�RX_EGSM900, ѡ���� EGSM900
	//get(Freq) f1, f2!
	std::string stable = "RX_" + std::string(band);
	std::string scolomn(band);
	//����У׼Ƶ������f1,f2��Ƶ��ֵ
	double f1_temp, f2_temp;
	ss.str("");
	ss << "select Max_val, Min_val from (select max(" + scolomn +
		") Max_val from [" + stable + "] where " + scolomn +
		" <= " << freq_now << "), (select min(" + scolomn +
		") Min_val from [" + stable + "] where " + scolomn +
		" >= " << freq_now << ")";
	if (GetSqlVal(ss.str().c_str(), f1_temp, f2_temp))
		return JCOFFSET_ERROR;
#ifdef JC_SQL_DEBUG
	std::cout << "Rxfreq Range: " << f1 << ", " << f2 << std::endl;
#endif
	//����f1,f2��Ӧ��index
	double f1_index, f2_index;
	ss.str("");
	ss << "select A,B from (select [ID] A from [" << stable << "] where " + scolomn +
		"=" << f1_temp << "),(select [ID] B from [" << stable << "] where " + scolomn +
		"=" << f2_temp << ")";
	if (GetSqlVal(ss.str().c_str(), f1_index, f2_index))
		return JCOFFSET_ERROR;
#ifdef JC_SQL_DEBUG
	std::cout << "Rxfreq_index Range: " << f1_index << ", " << f2_index << std::endl;
#endif
	//����POI��ȡƵ������
	if (_mode == MODE_POI) {
		double f_start, f_stop;
		char sql[1024] = { 0 };
		sprintf_s(sql, "select [rx_start],[rx_end] from [JC_BAND_INFO] where band = '%s'", band);
		if (GetSqlVal(sql, f_start, f_stop))
			return JCOFFSET_ERROR;

		if (freq_now<f_start || freq_now>f_stop)
			return JCOFFSET_ERROR;

		f1_index = floor((freq_now - f_start) / OFFSET_STEP) + 1;
		f2_index = ceil((freq_now - f_start) / OFFSET_STEP) + 1;
		f1_temp = f_start + OFFSET_STEP * (f1_index - 1);
		f2_temp = f_start + OFFSET_STEP * (f2_index - 1);
	}

	double freq1 = f1_temp;
	double freq2 = f2_temp;
	double f1 = f1_index;
	double f2 = f2_index;

	double y1 = 0;
	double y2 = 0;
	//���ø�������Port
	std::string sport = std::string(band) + sSuffix;
	stable = "JC_RX_OFFSET_ALL";
	//get y1, y2!
	//��ѯ����2�����ֵ(tx1,f1),(tx1,f2)
	ss.str("");
	ss << "select Val1, Val2 from (select [" << f1 << "] Val1 from [" + stable +
		"] where Port = '" + sport + "'), (select [" << f2 << "] Val2 from [" + stable +
		"] where Port = '" + sport + "')";
	if (GetSqlVal(ss.str().c_str(), y1, y2))
		return JCOFFSET_ERROR;
#ifdef JC_SQL_DEBUG
	std::cout << "y1 = " << y1 << ", y2 = " << y2 << std::endl;
#endif
	//��ʼ����offset
	return SumSlope(freq_now, freq1, y1, freq2, y2);
}

//��ȡvco����
double JcOffsetDB::OffsetVco(const char* band, const char& dut) {
	double val = -10000;
	std::string sSuffix = dut == 0 ? "_A" : "_B";

	std::string sTable = "JC_VCO_OFFSET_ALL";
	std::string sColomn(band);
	sColomn += sSuffix;
	std::string sql = "select vco from " + sTable + " where port = '" + sColomn + "'";

	sqlite3_stmt* pStmt;
	int s = sqlite3_prepare(_pConn, sql.c_str(), -1, &pStmt, NULL);
	if (sqlite3_step(pStmt) == SQLITE_ROW) {
		val = sqlite3_column_double(pStmt, 0);
	}
	else
		val = JCOFFSET_ERROR;
	sqlite3_finalize(pStmt);
	return val;
}

//�洢У׼����
int JcOffsetDB::Store_v2(const char& tx_or_rx,
						 const char* band, const char& dut, const char& coup,
						 const char& real_or_dsp,
						 const double tx,
						 const double* val, int num) {

	std::string sSuffix = dut == 0 ? "_A" : "_B";
	if (tx_or_rx == OFFSET_TX)
		sSuffix += (coup == 0 ? "_TX1" : "_TX2");

	std::string stable = tx_or_rx == OFFSET_TX ? "JC_TX_OFFSET_ALL" : "JC_RX_OFFSET_ALL";
	//���ø�������Port
	std::string sport = std::string(band) + sSuffix;
	//���ø�������Dsp
	int idsp = real_or_dsp;

	std::stringstream ss_freq;
	std::stringstream ss_val;

	if (tx_or_rx == OFFSET_TX) {
		//TX��������飨��1��ʼ������
		ss_freq << "(Port,Dsp,Power,'" << 1 << "'";
		for (int i = 2; i <= num; ++i) {
			ss_freq << ",'" << i << "'";
		}
		ss_freq << ")";

		//�洢ֵTX����
		ss_val << "('" << sport << "'," << idsp << "," << tx << "," << val[0];
		for (int i = 1; i < num; ++i) {
			ss_val << "," << val[i];
		}
		ss_val << ")";
	}
	else {
		//RX��������飨��1��ʼ������
		ss_freq << "(Port,'" << 1 << "'";
		for (int i = 2; i <= num; ++i) {
			ss_freq << ",'" << i << "'";
		}
		ss_freq << ")";

		//�洢ֵRX����
		ss_val << "('" << sport << "'," << val[0];
		for (int i = 1; i < num; ++i) {
			ss_val << "," << val[i];
		}
		ss_val << ")";
	}

	//��ʼ�洢��������д�룬ע��δ���������Ĭ��ֵΪ0
	std::string sql = "insert or replace into [" + stable + "] " + std::string(ss_freq.str()) + " values " + std::string(ss_val.str());
	sqlite3_stmt* pstmt;
	sqlite3_prepare(_pConn, sql.c_str(), -1, &pstmt, NULL);
	int resulte = sqlite3_step(pstmt);
	sqlite3_finalize(pstmt);

	if (resulte == SQLITE_DONE)
		return 0;
	else
		return JCOFFSET_ERROR;
}

//�洢vcoУ׼����
int JcOffsetDB::Store_vco_single(const char* band, const char& dut, const double val) {
	std::string sSuffix = dut == 0 ? "_A" : "_B";
	std::string sColomn(band);
	sColomn += sSuffix;

	std::string sql = "insert or replace into JC_VCO_OFFSET_ALL (port,vco) values ('" + sColomn + "'," + std::to_string(val) + ")";
	sqlite3_stmt* pstmt;
	sqlite3_prepare(_pConn, sql.c_str(), -1, &pstmt, NULL);
	int resulte = sqlite3_step(pstmt);
	sqlite3_finalize(pstmt);

	if (resulte == SQLITE_DONE)
		return 0;
	else
		return JCOFFSET_ERROR;
}

//����б��
double JcOffsetDB::SumSlope(double v, double x1, double y1, double x2, double y2) {
	if (x1 == x2)
		return y1;
	else
		return (y2 - y1) / (x2 - x1) * v + (y1 * x2 - y2 * x1) / (x2 - x1);
}

//sqlite���ִ��
int JcOffsetDB::GetSqlVal(const char* strsql, double& a1, double& a2){
	sqlite3_stmt* pStmt;
	int r = 0;
	sqlite3_prepare(_pConn, strsql, -1, &pStmt, NULL);

	if (sqlite3_step(pStmt) == SQLITE_ROW) {
		if (sqlite3_column_count(pStmt) == 2) {
			a1 = sqlite3_column_double(pStmt, 0);
			a2 = sqlite3_column_double(pStmt, 1);
		}
		else
			r = JCOFFSET_ERROR;
	}
	else
		r = JCOFFSET_ERROR;
	sqlite3_finalize(pStmt);
	return r;
}