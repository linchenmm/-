#include "ads1220.h"						//ADS1220
//#include "sys.h"
#include "spi.h"



//ADS1220��ʼ��
void ADS1220Init(void)
{
	SPI_CS_DRDY_Init();//CS\״̬���ų�ʼ��
	MX_SPI1_Init();//Ӳ��SPI��ʼ��
     LL_mDelay(10);
     ADS1220SendResetCommand();//���͸�λ����
     LL_mDelay(10);
     ADS1220Config();//ADS1220���üĴ���
//     ADS1220SetVoltageReference(0x00);//ѡ�� 2.048V �Ȳ���׼��ѹ
     ADS1220SetVoltageReference(0x40);//ѡ�� 5V �ⲿ��׼��ѹ
     
     
     
}

/* ADS1220 ��ʼ������ */
void ADS1220Config(void)
{
	unsigned int Temp;
	ADS1220ReadRegister(ADS1220_0_REGISTER, 0x01, &Temp);
	/* clear prev value; */
   	Temp &= 0x0f;
   	Temp |= ADS1220_MUX_0_G;
   	/* write the register value containing the new value back to the ADS */
   	ADS1220WriteRegister(ADS1220_0_REGISTER, 0x01, &Temp);
	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	/* clear prev DataRate code; */
	Temp &= 0x1f;
	Temp |= (ADS1220_DR_600 + ADS1220_CC);		/* Set default start mode to 600sps and continuous conversions */
	/* write the register value containing the new value back to the ADS */
	ADS1220WriteRegister(ADS1220_1_REGISTER, 0x01, &Temp);
}


//  �ж϶��������ţ��͵�ƽ˵������׼����
int ADS1220WaitForDataReady(int Timeout)
{
    while(Timeout-- >= 0)
    {
         if(!DRDY_Ready)
              return ADS1220_DATA_OK;
    }
    return ADS1220_ERROR;
}


//ADS1220�������ݺ���
void ADS1220SendByte(unsigned char Byte)
{	
     SPI_WriteReadByte(Byte);
}

//ADS1220�������ݺ���
unsigned char ADS1220ReceiveByte(void)
{	
	return SPI_WriteReadByte(0xff);
}

//��ȡ����
uint32_t ADS1220ReadData(void)
{
   uint32_t Data;
     
   SPI_CS_Level(0);

   ADS1220SendByte(ADS1220_CMD_RDATA);

   Data = ADS1220ReceiveByte();
   Data = (Data << 8) | ADS1220ReceiveByte();
   Data = (Data << 8) | ADS1220ReceiveByte();

   if (Data & 0x800000)
      Data |= 0xff000000; 
	 
   SPI_CS_Level(1);
   return Data;
}


/***ADS1220���Ĵ���
StartAddress:�Ĵ�����ַ��0x00-0x03��
NumRegs:д����ֽ���
pData:д��Ĵ���ֵ
***/
void ADS1220ReadRegister(int StartAddress, int NumRegs, unsigned * pData)
{
     int i;
	SPI_CS_Level(0);
 	/* ���η��Ͷ��Ĵ���������Ĵ�����ַ��д��Ĵ����ֽ�������һ�ֽ����*/
	ADS1220SendByte(ADS1220_CMD_RREG | (((StartAddress<<2) & 0x0c) |((NumRegs-1)&0x03)));
   	/* �������� */
	for (i=0; i< NumRegs; i++)
	{
		*pData++ = ADS1220ReceiveByte();
	}
	SPI_CS_Level(1);
	return;
}

/***ADS1220д�Ĵ���
StartAddress:�Ĵ�����ַ��0x00-0x03��
NumRegs:д����ֽ���
pData:д��Ĵ���ֵ
***/
void ADS1220WriteRegister(int StartAddress, int NumRegs, unsigned int* pData)
{
	int i;
	SPI_CS_Level(0);
   	/* ���η���д�Ĵ���������Ĵ�����ַ��д��Ĵ����ֽ�������һ�ֽ����*/
	ADS1220SendByte(ADS1220_CMD_WREG | (((StartAddress<<2) & 0x0c) |((NumRegs-1)&0x03)));
     /* ����д������ */
	for (i=0; i< NumRegs; i++)
	{
		ADS1220SendByte(*pData++);
	}
	SPI_CS_Level(1);
   	return;
}

//ADS1220��λ
void ADS1220SendResetCommand(void)
{
	SPI_CS_Level(0);
	ADS1220SendByte(ADS1220_CMD_RESET);
	SPI_CS_Level(1);
   	return;
}

//ADS1220��ʼ����
void ADS1220SendStartCommand(void)
{
	SPI_CS_Level(0);
	ADS1220SendByte(ADS1220_CMD_SYNC);
	SPI_CS_Level(1);
     return;
}

//ADS1220�ػ�����
void ADS1220SendShutdownCommand(void)
{
	SPI_CS_Level(0);
	ADS1220SendByte(ADS1220_CMD_SHUTDOWN);
	SPI_CS_Level(1);
     return;
}

/***�����·����������
���������·��������
���� AINN = AVSS �����ã� PGA ������� (PGA_BYPASS = 1)�� ���ҽ���ʹ��
���� 1�� 2 �� 4��
0000�� AINP = AIN0�� AINN = AIN1��Ĭ�����ã�
0001�� AINP = AIN0�� AINN = AIN2
0010�� AINP = AIN0�� AINN = AIN3
0011�� AINP = AIN1�� AINN = AIN2
0100�� AINP = AIN1�� AINN = AIN3
0101�� AINP = AIN2�� AINN = AIN3
0110�� AINP = AIN1�� AINN = AIN0
0111�� AINP = AIN3�� AINN = AIN2
1000�� AINP = AIN0�� AINN = AVSS
1001�� AINP = AIN1�� AINN = AVSS
1010�� AINP = AIN2�� AINN = AVSS
1011�� AINP = AIN3�� AINN = AVSS
1100�� (V(REFPx) �C V(REFNx)) / 4 ���ӣ���· PGA��
1101�� (AVDD �C AVSS) / 4 ���ӣ���· PGA��
1110�� AINP �� AINN �̽��� (AVDD + AVSS) / 2
1111�� ����
***/
int ADS1220SetChannel(int Mux)
{
	unsigned int cMux = Mux;	   
   ADS1220WriteRegister(ADS1220_0_REGISTER, 0x01, &cMux);
   return ADS1220_NO_ERROR;
}


/***��������
���������������档
�ڲ�ʹ�� PGA ������£� ��ʹ������ 1�� 2 �� 4�� ����������£� ͨ�����ص��ݽ�
��������档
000�� ���� = 1��Ĭ�����ã�
001�� ���� = 2
010�� ���� = 4
011�� ���� = 8
100�� ���� = 16
101�� ���� = 32
110�� ���� = 64
111�� ���� = 128
***/
int ADS1220SetGain(int Gain)
{
	unsigned int cGain = Gain;   
	ADS1220WriteRegister(ADS1220_0_REGISTER, 0x01, &cGain);
	return ADS1220_NO_ERROR;
}


/***���ú���·�ڲ�������PGA
���� PGA �ή�����幦�ģ� ���ɽ���ģ��ѹ��Χ (VCM) ��չΪ AVSS �C 0.1V ��
AVDD + 0.1V��
ֻ��������� 1�� 2 �� 4 ���� PGA��
���� PGA_BYPASS ������Σ� ��ʼ������������� 8 �� 128 ���� PGA��
0�� PGA �����ã�Ĭ�����ã�
1�� PGA �ѽ��ú���·
***/
int ADS1220SetPGABypass(int Bypass)
{
	unsigned int cBypass = Bypass;
	ADS1220WriteRegister(ADS1220_0_REGISTER, 0x01, &cBypass);
	return ADS1220_NO_ERROR;
}


/***��������
���������������ã� ȡ������ѡ����ģʽ��
***/
int ADS1220SetDataRate(int DataRate)
{
	unsigned int cDataRate = DataRate;  
	ADS1220WriteRegister(ADS1220_1_REGISTER, 0x01, &cDataRate);
	return ADS1220_NO_ERROR;
}

//
int ADS1220SetClockMode(int ClockMode)
{
	unsigned int cClockMode = ClockMode;
	ADS1220WriteRegister(ADS1220_1_REGISTER, 0x01, &cClockMode);
	return ADS1220_NO_ERROR;
}

int ADS1220SetPowerDown(int PowerDown)
{
	unsigned int cPowerDown = PowerDown;
	ADS1220WriteRegister(ADS1220_1_REGISTER, 0x01, &cPowerDown);
	return ADS1220_NO_ERROR;
}

//�����¶ȴ�����ģʽ
int ADS1220SetTemperatureMode(int TempMode)
{
	unsigned int cTempMode = TempMode;
	ADS1220WriteRegister(ADS1220_1_REGISTER, 0x01, &cTempMode);
	return ADS1220_NO_ERROR;
}


int ADS1220SetBurnOutSource(int BurnOut)
{
	unsigned int cBurnOut = BurnOut;
	ADS1220WriteRegister(ADS1220_1_REGISTER, 0x01, &cBurnOut);
	return ADS1220_NO_ERROR;
}

/***��׼��ѹѡ��
����ѡ��ת����ʹ�õĻ�׼��ѹԴ��
00�� ѡ�� 2.048V �ڲ���׼��ѹ��Ĭ�����ã�
01�� ʹ��ר�� REFP0 �� REFN0 ����ѡ����ⲿ��׼��ѹ
10�� ʹ�� AIN0/REFP1 �� AIN3/REFN1 ����ѡ����ⲿ��׼��ѹ
11�� ������׼��ģ���Դ (AVDD �C AVSS)
***/
int ADS1220SetVoltageReference(int VoltageRef)
{
	unsigned int cVoltageRef = VoltageRef;
	ADS1220WriteRegister(ADS1220_2_REGISTER, 0x01, &cVoltageRef);
	return ADS1220_NO_ERROR;
}

/***FIR �˲�������
����Ϊ�ڲ� FIR �˲��������˲���ϵ����
������ģʽ�£� ��Щλ���� 20SPS ���ý��ʹ�ã� ��ռ�ձ�ģʽ�£� ��Щλ����
5SPS ���ý��ʹ�á� �������������������ʣ� ��Щλ������Ϊ 00��
00�� �� 50Hz �� 60Hz ���ƣ�Ĭ�����ã�
01�� ͬʱ���� 50Hz �� 60Hz
10�� ֻ���� 50Hz
11�� ֻ���� 60Hz
***/
int ADS1220Set50_60Rejection(int Rejection)
{
	unsigned int cRejection = Rejection;
	ADS1220WriteRegister(ADS1220_2_REGISTER, 0x01, &cRejection);
	return ADS1220_NO_ERROR;
}

/***�Ͳ��Դ��������
�������� AIN3/REFN1 �� AVSS ֮�����ӵĵͲ࿪�ص���Ϊ��
0�� ����ʼ�մ��ڶϿ�״̬��Ĭ�����ã�
1�� ���ػ��ڷ��� START/SYNC ����ʱ�Զ��պϣ� ���ڷ��� POWERDOWN ����
ʱ�Զ��Ͽ�
***/
int ADS1220SetLowSidePowerSwitch(int PowerSwitch)
{
	unsigned int cPowerSwitch = PowerSwitch;
	ADS1220WriteRegister(ADS1220_2_REGISTER, 0x01, &cPowerSwitch);
	return ADS1220_NO_ERROR;
}


int ADS1220SetCurrentDACOutput(int CurrentOutput)
{
	unsigned int cCurrentOutput = CurrentOutput;
	ADS1220WriteRegister(ADS1220_2_REGISTER, 0x01, &cCurrentOutput);
	return ADS1220_NO_ERROR;
}


int ADS1220SetIDACRouting(int IDACRoute)
{
	unsigned int cIDACRoute = IDACRoute;
	ADS1220WriteRegister(ADS1220_3_REGISTER, 0x01, &cIDACRoute);
	return ADS1220_NO_ERROR;
}


int ADS1220SetDRDYMode(int DRDYMode)
{
	unsigned int cDRDYMode = DRDYMode;
	ADS1220WriteRegister(ADS1220_3_REGISTER, 0x01, &cDRDYMode);
	return ADS1220_NO_ERROR;
}


/*��ȡ�Ĵ�������*/

int ADS1220GetChannel(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_0_REGISTER, 0x01, &Temp);
	return (Temp >>4);
}


int ADS1220GetGain(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_0_REGISTER, 0x01, &Temp);
	return ( (Temp & 0x0e) >>1);
}


int ADS1220GetPGABypass(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_0_REGISTER, 0x01, &Temp);
	return (Temp & 0x01);
}


int ADS1220GetDataRate(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	return ( Temp >>5 );
}


int ADS1220GetClockMode(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	return ( (Temp & 0x18) >>3 );
}


int ADS1220GetPowerDown(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	return ( (Temp & 0x04) >>2 );
}


int ADS1220GetTemperatureMode(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	return ( (Temp & 0x02) >>1 );
}


int ADS1220GetBurnOutSource(void)
{
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);

	return ( Temp & 0x01 );
}


int ADS1220GetVoltageReference(void)
{
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_2_REGISTER, 0x01, &Temp);

	return ( Temp >>6 );
}


int ADS1220Get50_60Rejection(void)
{
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_2_REGISTER, 0x01, &Temp);

	return ( (Temp & 0x30) >>4 );
}


int ADS1220GetLowSidePowerSwitch(void)
{
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_2_REGISTER, 0x01, &Temp);

	return ( (Temp & 0x08) >>3);
}


int ADS1220GetCurrentDACOutput(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_2_REGISTER, 0x01, &Temp);
	return ( Temp & 0x07 );
}


int ADS1220GetIDACRouting(int WhichOne)
{
     unsigned int Temp;
     
	if (WhichOne >1) return ADS1220_ERROR;
	ADS1220ReadRegister(ADS1220_3_REGISTER, 0x01, &Temp);
	if (WhichOne) return ( (Temp & 0x1c) >>2);
	else return ( Temp >>5 );
}


int ADS1220GetDRDYMode(void)
{
	unsigned Temp;
	ADS1220ReadRegister(ADS1220_3_REGISTER, 0x01, &Temp);
	return ( (Temp & 0x02) >>1 );
}



void set_MUX(char c)
{	
	int mux = (int) c - 48;
	int dERROR;
	unsigned Temp;
	if (mux>=49 && mux<=54) mux -= 39;
	/* The MUX value is only part of the register, so we have to read it back
	   and massage the new value into it */
	ADS1220ReadRegister(ADS1220_0_REGISTER, 0x01, &Temp);
	Temp &= 0x0f;									/* strip out old settings */
	/* Change Data rate */
	switch(mux) {
		case 0:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_0_1);
			break;
		case 1:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_0_2);
			break;
		case 2:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_0_3);
			break;
		case 3:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_1_2);
			break;
		case 4:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_1_3);
			break;
		case 5:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_2_3);
			break;
		case 6:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_1_0);
			break;
		case 7:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_3_2);
			break;
		case 8:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_0_G);
			break;
		case 9:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_1_G);
			break;
		case 10:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_2_G);
			break;
		case 11:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_3_G);
			break;
		case 12:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_EX_VREF);
			break;
		case 13:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_AVDD);
			break;
		case 14:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_DIV2);
			break;
		case 15:
			dERROR = ADS1220SetChannel(Temp + ADS1220_MUX_DIV2);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;												
	}
	if (dERROR==ADS1220_ERROR)
		set_ERROR();
}


void set_GAIN(char c)
{
	int pga = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_0_REGISTER, 0x01, &Temp);
		Temp &= 0xf1;									/* strip out old settings */

	switch(pga) {
		case 0:
			dERROR = ADS1220SetGain(Temp + ADS1220_GAIN_1);
			break;
		case 1:
			dERROR = ADS1220SetGain(Temp + ADS1220_GAIN_2);
			break;
		case 2:
			dERROR = ADS1220SetGain(Temp + ADS1220_GAIN_4);
			break;
		case 3:
			dERROR = ADS1220SetGain(Temp + ADS1220_GAIN_8);
			break;
		case 4:
			dERROR = ADS1220SetGain(Temp + ADS1220_GAIN_16);
			break;
		case 5:
			dERROR = ADS1220SetGain(Temp + ADS1220_GAIN_32);
			break;
		case 6:
			dERROR = ADS1220SetGain(Temp + ADS1220_GAIN_64);
			break;
		case 7:
			dERROR = ADS1220SetGain(Temp + ADS1220_GAIN_128);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;	
		}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_PGA_BYPASS(char c)
{
	int buff = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_0_REGISTER, 0x01, &Temp);
	Temp &= 0xfe;									/* strip out old settings */

	switch(buff) {
		case 0:
			dERROR = ADS1220SetPGABypass(Temp);
			break;
		case 1:
			dERROR = ADS1220SetPGABypass(Temp + ADS1220_PGA_BYPASS);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_DR(char c)
{
	int spd = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	Temp &= 0x1f;									

	switch(spd) {
		case 0:
			dERROR = ADS1220SetDataRate(Temp + ADS1220_DR_20);
			break;
		case 1:
			dERROR = ADS1220SetDataRate(Temp + ADS1220_DR_45);
			break;
		case 2:
			dERROR = ADS1220SetDataRate(Temp + ADS1220_DR_90);
			break;
		case 3:
			dERROR = ADS1220SetDataRate(Temp + ADS1220_DR_175);
			break;
		case 4:
			dERROR = ADS1220SetDataRate(Temp + ADS1220_DR_330);
			break;
		case 5:
			dERROR = ADS1220SetDataRate(Temp + ADS1220_DR_600);
			break;
		case 6:
			dERROR = ADS1220SetDataRate(Temp + ADS1220_DR_1000);
			break;
		case 7:
			dERROR = ADS1220SetDataRate(Temp + ADS1220_DR_1000);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}

//���ù���ģʽ
void set_MODE(char c)
{
	int spd = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	Temp &= 0xe7;									
     
	switch(spd) {
		case 0:
			dERROR = ADS1220SetClockMode(Temp + ADS1220_MODE_NORMAL);
			break;
		case 1:
			dERROR = ADS1220SetClockMode(Temp + ADS1220_MODE_DUTY);
			break;
		case 2:
			dERROR = ADS1220SetClockMode(Temp + ADS1220_MODE_TURBO);
			break;
		case 3:
			dERROR = ADS1220SetClockMode(Temp + ADS1220_MODE_DCT);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_CM(char c)
{
	int pwrdn = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	Temp &= 0xfb;				

	switch(pwrdn) {
		case 0:
			dERROR = ADS1220SetPowerDown(Temp);
			break;
		case 1:
			dERROR = ADS1220SetPowerDown(Temp + ADS1220_CC);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_TS(char c)
{
	int tmp = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	Temp &= 0xfd;								

	switch(tmp) {
		case 0:
			dERROR = ADS1220SetTemperatureMode(Temp);
			break;
		case 1:
			dERROR = ADS1220SetTemperatureMode(Temp + ADS1220_TEMP_SENSOR);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_BCS(char c)
{
	int bo = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_1_REGISTER, 0x01, &Temp);
	Temp &= 0xfe;								

	switch(bo) {
		case 0:
			dERROR = ADS1220SetBurnOutSource(Temp);
			break;
		case 1:
			dERROR = ADS1220SetBurnOutSource(Temp + ADS1220_BCS);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_VREF(char c)
{
	int ref = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_2_REGISTER, 0x01, &Temp);
	Temp &= 0x3f;								

	switch(ref) {
		case 0:
			dERROR = ADS1220SetVoltageReference(Temp + ADS1220_VREF_INT);
			break;
		case 1:
			dERROR = ADS1220SetVoltageReference(Temp + ADS1220_VREF_EX_DED);
			break;
		case 2:
			dERROR = ADS1220SetVoltageReference(Temp + ADS1220_VREF_EX_AIN);
			break;
		case 3:
			dERROR = ADS1220SetVoltageReference(Temp + ADS1220_VREF_SUPPLY);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_50_60(char c)
{
	int flt = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_2_REGISTER, 0x01, &Temp);
	Temp &= 0xcf;									

	switch(flt) {
		case 0:
			dERROR = ADS1220Set50_60Rejection(Temp + ADS1220_REJECT_OFF);
			break;
		case 1:
			dERROR = ADS1220Set50_60Rejection(Temp + ADS1220_REJECT_BOTH);
			break;
		case 2:
			dERROR = ADS1220Set50_60Rejection(Temp + ADS1220_REJECT_50);
			break;
		case 3:
			dERROR = ADS1220Set50_60Rejection(Temp + ADS1220_REJECT_60);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_PSW(char c)
{
	int sw = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_2_REGISTER, 0x01, &Temp);
	Temp &= 0xf7;								

	switch(sw) {
		case 0:
			dERROR = ADS1220SetLowSidePowerSwitch(Temp);
			break;
		case 1:
			dERROR = ADS1220SetLowSidePowerSwitch(Temp + ADS1220_PSW_SW);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_IDAC(char c)
{
	int current = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_2_REGISTER, 0x01, &Temp);
	Temp &= 0xf8;		
     
	switch(current) {
		case 0:
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_OFF);
			break;
		case 1:
#ifdef ADS1120
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_OFF);
#else
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_10);
#endif
			break;
		case 2:
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_50);
			break;
		case 3:
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_100);
			break;
		case 4:
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_250);
			break;
		case 5:
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_500);
			break;
		case 6:
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_1000);
			break;
		case 7:
			dERROR = ADS1220SetCurrentDACOutput(Temp + ADS1220_IDAC_2000);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
		}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}


void set_IMUX(char c, int i)
{
	int mux = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_3_REGISTER, 0x01, &Temp);
	if (i==1) {
		Temp &= 0xe3;								

		switch(mux) {
			case 0:
				Temp |= ADS1220_IDAC2_OFF;
				break;
			case 1:
				Temp |= ADS1220_IDAC2_AIN0;
				break;
			case 2:
				Temp |= ADS1220_IDAC2_AIN1;
				break;
			case 3:
				Temp |= ADS1220_IDAC2_AIN2;
				break;
			case 4:
				Temp |= ADS1220_IDAC2_AIN3;
				break;
			case 5:
				Temp |= ADS1220_IDAC2_REFP0;
				break;
			case 6:
				Temp |= ADS1220_IDAC2_REFN0;
				break;
			case 7:
				Temp |= ADS1220_IDAC2_REFN0;
				break;
			default:
				dERROR = ADS1220_ERROR;
				break;
		}
	}
	else {
		Temp &= 0x1f;

		switch(mux) {
			case 0:
				Temp |= ADS1220_IDAC1_OFF;
				break;
			case 1:
				Temp |= ADS1220_IDAC1_AIN0;
				break;
			case 2:
				Temp |= ADS1220_IDAC1_AIN1;
				break;
			case 3:
				Temp |= ADS1220_IDAC1_AIN2;
				break;
			case 4:
				Temp |= ADS1220_IDAC1_AIN3;
				break;
			case 5:
				Temp |= ADS1220_IDAC1_REFP0;
				break;
			case 6:
				Temp |= ADS1220_IDAC1_REFN0;
				break;
			case 7:
				Temp |= ADS1220_IDAC1_REFN0;
				break;
			default:
				dERROR = ADS1220_ERROR;
				break;
		}
	}
	if (dERROR==ADS1220_NO_ERROR) 
		dERROR = ADS1220SetIDACRouting(Temp); 
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}

void set_DRDYM(char c)
{
	int drdy = (int) c - 48;
	int dERROR;
	unsigned Temp;

	ADS1220ReadRegister(ADS1220_3_REGISTER, 0x01, &Temp);
	Temp &= 0xfd;									

	switch(drdy) {
		case 0:
			dERROR = ADS1220SetDRDYMode(Temp);
			break;
		case 1:
			dERROR = ADS1220SetDRDYMode(Temp + ADS1220_DRDY_MODE);
			break;
		default:
			dERROR = ADS1220_ERROR;
			break;
	}
	if (dERROR==ADS1220_ERROR) 
		set_ERROR();
}

void set_ERROR(void)
{
	
}



