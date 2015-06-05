/*********************Copy-Right****************************
 *														   *
 *														   *
 *														   *
 *														   *
 ***********************************************************/
#ifndef __JC_MATRIX_SWITCH_H__
#define __JC_MATRIX_SWITCH_H__

//#define EXPORT_OUT_DLL

//�������ش������
#define MATRIX_SWITCH_OK						(0 )	//�ɹ�
#define MATRIX_SWITCH_ERROR_INIT_EXCEPTION		(-1)    //��ʼ���쳣
#define MATRIX_SWITCH_ERROR_INI_FILE_NOEXIST	(-2)    //�����ļ�������
#define MATRIX_SWITCH_ERROR_OBJECT_NULL			(-3)    //���󲻴���
#define MATRIX_SWITCH_ERROR_CHAN_NO_EXIST		(-4)    //ͨ����Ų�����
#define MATRIX_SWITCH_EXCUTE_FAILED				(-1)    //���ز���ִ��ʧ��
//ͨ�����ӷ�ʽ����
#define COMM_TYPE_TCP		(2)
#define COMM_TYPE_UDP		(3)
#define COMM_TYPE_COM		(4)
//���󿪹�����
#define ID_HUAWEI			(1)
#define ID_POI				(2)
/**********************************************************/
/*POI DEFINE*/
#define ID_SW1_SDT3			(1)   //��������1 ��������Χ<1~3>
#define ID_SW2_SDT3         (2)	  //��������2 ��������Χ<1~3>
#define ID_SW3_SDT4         (3)   //��������3 ��������Χ<1~4>
#define ID_SW4_SDT4         (4)   //��������4 ��������Χ<1~4>
#define ID_SW5_SDT7         (5)   //��������5 ��������Χ<1~7>
#define ID_SW6_SDT7         (6)   //��������6 ��������Χ<1~7>
#define ID_SW7_SDT2         (7)   //��������7 ��������Χ<1~2>
#define ID_SW8_SDT2         (8)   //��������8 ��������Χ<1~2>
#define ID_SW9_SDT2         (9)   //��������9 ��������Χ<1~2>
#define ID_SW10_SDT2        (10)  //��������10��������Χ<1~2>
#define ID_SW11_SDT2        (11)  //��������11��������Χ<1~2>
#define ID_SW12_SDT2        (12)  //��������12��������Χ<1~2>
//POIģ���ַ
#define IP_Signalswich  	(1)
#define IP_Paspecumpwmt     (2)
#define IP_Testmdcdmagsm    (3)
#define IP_Testmdfdd18      (4)
#define IP_Testmdfdd21      (5)
#define IP_Testmdtdftda     (6)
#define IP_Testmdtdd23      (7)
//POI-TX1 ͨ��
#define ID_1Cmgsmtx1        (0)
#define ID_2Cucdmatx1       (1)
#define ID_3Ctfd18tx1       (2)
#define ID_4Cufd18tx1       (3)
#define ID_5Ctfd21tx1       (4)
#define ID_6Cuw21tx1        (5)
#define ID_7Cmdcstx1        (6)
#define ID_8Cmtdftx1        (7)
#define ID_10Cmtdetx1       (9)
#define ID_11Cttdetx1       (10)
//POI-TX2 ͨ��                     
#define ID_1Cmgsmtx2        (0 )
#define ID_2Cucdmatx2       (1 )
#define ID_3Ctfd18tx2       (2 )
#define ID_4Cufd18tx2       (3 )
#define ID_5Ctfd21tx2       (4 )
#define ID_6Cuw21tx2        (5 )
#define ID_7Cmdcstx2        (6 )
#define ID_8Cmtdftx2        (7 )
#define ID_9Cmtdatx2        (8 )
#define ID_10Cmtdetx2       (9 )
#define ID_12Cutdetx2       (11)
//POI-RX ͨ��                      
#define ID_1gsmpim 	        (0)
#define ID_2cdmapim         (1)
#define ID_3ctfd18pim       (2)
#define ID_4cufd18pim       (3)
#define ID_5ctfd21pim       (4)
#define ID_6cuw21pim        (5)
#define ID_7cmdcspim        (6)
#define ID_8cmtdfpim        (7)
#define ID_10cmtde23pim     (9)
#define ID_11cttde23pim     (10)
//POI-Coup ͨ��
#define ID_1Cdmagsmcp1      (0 )
#define ID_2Cdmagsmcp2      (1 )
#define ID_3fdd18cp1        (2 )
#define ID_4fdd18cp2        (3 )
#define ID_5fdd21cp1        (4 )
#define ID_6fdd21cp2        (5 )
#define ID_7tdftdacp1       (6 )
#define ID_8tdftdacp2       (7 )
#define ID_9tdftdacp3       (8 )
#define ID_10tde23cp1	    (9 )
#define ID_11tde23cp2       (10)
/**********************************************************/

//����ָ������
typedef int (*pMartrixSwitchInit)(int,char*,int,int);
typedef int(*pMartrixSwitchExcute)(int, int, int);
typedef int (*pMartrixSwitchBoxExcute)(int, int, int, int);
typedef int (*pMartrixSwitchDispose)();

#ifdef EXPORT_OUT_DLL
//��̬������
extern "C" __declspec(dllexport) int MartrixSwitchInit(int handle, char*dllName, int swType, int comType);
extern "C" __declspec(dllexport) int MartrixSwitchBoxExcute(int tx1, int tx2, int pim, int det);
extern "C" __declspec(dllexport) int MartrixSwitchExcute(int addr, int swId, int swIdx);
extern "C" __declspec(dllexport) int MartrixSwitchDispose();

#else
//��̬������
/*******************************************************************************
 *�������ƣ�MartrixSwitchInit
 *�������ܣ����󿪹س�ʼ��
 *����˵����int handle	��������������������NULL
 *			char*dllName����������̬����������Ѱ�������ļ���·��
 *			int swType	�������������ͣ���Ϊ��POI
 *			int comType	����ͨ�ŷ�ʽ
 *�������ͣ����մ��󷵻ش���
 */
extern int MartrixSwitchInit(int handle, char*dllName, int swType, int comType);
/*******************************************************************************
 *�������ƣ�MartrixSwitchBoxExcute
 *�������ܣ��������ͨ������
 *����˵����int tx1	���� TX1ͨ�����
 *			int tx2 ���� TX2ͨ�����
 *			int pim	���� PIMͨ�����
 *			int det	���� DETͨ�����
 *�������ͣ����մ��󷵻ش���
 */
extern int MartrixSwitchBoxExcute(int tx1, int tx2, int pim, int det);
/*******************************************************************************
 *�������ƣ�MartrixSwitchExcute
 *�������ܣ����󿪹ص����ز���
 *����˵����int addr	����ģ���ַ
 *			int swId	���������������
 *			int swIdx	���������������
 *�������ͣ����մ��󷵻ش���
 */
extern int MartrixSwitchExcute(int addr, int swId, int swIdx);
/*******************************************************************************
 *�������ƣ�MartrixSwitchDispose
 *�������ܣ����󿪹��ͷ���Դ
 *�������ͣ����մ��󷵻ش���
 */
extern int MartrixSwitchDispose();

#endif

#endif