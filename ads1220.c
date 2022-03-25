#include "ads1220.h"						//ADS1220
//#include "sys.h"
#include "spi.h"



//ADS1220初始化
void ADS1220Init(void)
{
	SPI_CS_DRDY_Init();//CS\状态引脚初始化
	MX_SPI1_Init();//硬件SPI初始化
     LL_mDelay(10);
     ADS1220SendResetCommand();//发送复位命令
     LL_mDelay(10);
     ADS1220Config();//ADS1220配置寄存器
//     ADS1220SetVoltageReference(0x00);//选择 2.048V 內部基准电压
     ADS1220SetVoltageReference(0x40);//选择 5V 外部基准电压
     
     
     
}

/* ADS1220 初始化配置 */
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


//  判断读数据引脚，低电平说明数据准备好
int ADS1220WaitForDataReady(int Timeout)
{
    while(Timeout-- >= 0)
    {
         if(!DRDY_Ready)
              return ADS1220_DATA_OK;
    }
    return ADS1220_ERROR;
}


//ADS1220发送数据函数
void ADS1220SendByte(unsigned char Byte)
{	
     SPI_WriteReadByte(Byte);
}

//ADS1220接收数据函数
unsigned char ADS1220ReceiveByte(void)
{	
	return SPI_WriteReadByte(0xff);
}

//读取数据
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


/***ADS1220读寄存器
StartAddress:寄存器地址（0x00-0x03）
NumRegs:写入的字节数
pData:写入寄存器值
***/
void ADS1220ReadRegister(int StartAddress, int NumRegs, unsigned * pData)
{
     int i;
	SPI_CS_Level(0);
 	/* 依次发送读寄存器的命令、寄存器地址、写入寄存器字节数进行一字节组包*/
	ADS1220SendByte(ADS1220_CMD_RREG | (((StartAddress<<2) & 0x0c) |((NumRegs-1)&0x03)));
   	/* 接收数据 */
	for (i=0; i< NumRegs; i++)
	{
		*pData++ = ADS1220ReceiveByte();
	}
	SPI_CS_Level(1);
	return;
}

/***ADS1220写寄存器
StartAddress:寄存器地址（0x00-0x03）
NumRegs:写入的字节数
pData:写入寄存器值
***/
void ADS1220WriteRegister(int StartAddress, int NumRegs, unsigned int* pData)
{
	int i;
	SPI_CS_Level(0);
   	/* 依次发送写寄存器的命令、寄存器地址、写入寄存器字节数进行一字节组包*/
	ADS1220SendByte(ADS1220_CMD_WREG | (((StartAddress<<2) & 0x0c) |((NumRegs-1)&0x03)));
     /* 发送写入数据 */
	for (i=0; i< NumRegs; i++)
	{
		ADS1220SendByte(*pData++);
	}
	SPI_CS_Level(1);
   	return;
}

//ADS1220复位
void ADS1220SendResetCommand(void)
{
	SPI_CS_Level(0);
	ADS1220SendByte(ADS1220_CMD_RESET);
	SPI_CS_Level(1);
   	return;
}

//ADS1220开始命令
void ADS1220SendStartCommand(void)
{
	SPI_CS_Level(0);
	ADS1220SendByte(ADS1220_CMD_SYNC);
	SPI_CS_Level(1);
     return;
}

//ADS1220关机命令
void ADS1220SendShutdownCommand(void)
{
	SPI_CS_Level(0);
	ADS1220SendByte(ADS1220_CMD_SHUTDOWN);
	SPI_CS_Level(1);
     return;
}

/***输入多路复用器配置
配置输入多路复用器。
对于 AINN = AVSS 的设置， PGA 必须禁用 (PGA_BYPASS = 1)， 并且仅可使用
增益 1、 2 和 4。
0000： AINP = AIN0， AINN = AIN1（默认设置）
0001： AINP = AIN0， AINN = AIN2
0010： AINP = AIN0， AINN = AIN3
0011： AINP = AIN1， AINN = AIN2
0100： AINP = AIN1， AINN = AIN3
0101： AINP = AIN2， AINN = AIN3
0110： AINP = AIN1， AINN = AIN0
0111： AINP = AIN3， AINN = AIN2
1000： AINP = AIN0， AINN = AVSS
1001： AINP = AIN1， AINN = AVSS
1010： AINP = AIN2， AINN = AVSS
1011： AINP = AIN3， AINN = AVSS
1100： (V(REFPx) – V(REFNx)) / 4 监视（旁路 PGA）
1101： (AVDD – AVSS) / 4 监视（旁路 PGA）
1110： AINP 和 AINN 短接至 (AVDD + AVSS) / 2
1111： 保留
***/
int ADS1220SetChannel(int Mux)
{
	unsigned int cMux = Mux;	   
   ADS1220WriteRegister(ADS1220_0_REGISTER, 0x01, &cMux);
   return ADS1220_NO_ERROR;
}


/***增益配置
用于配置器件增益。
在不使用 PGA 的情况下， 可使用增益 1、 2 和 4。 在这种情况下， 通过开关电容结
构获得增益。
000： 增益 = 1（默认设置）
001： 增益 = 2
010： 增益 = 4
011： 增益 = 8
100： 增益 = 16
101： 增益 = 32
110： 增益 = 64
111： 增益 = 128
***/
int ADS1220SetGain(int Gain)
{
	unsigned int cGain = Gain;   
	ADS1220WriteRegister(ADS1220_0_REGISTER, 0x01, &cGain);
	return ADS1220_NO_ERROR;
}


/***禁用和旁路内部低噪声PGA
禁用 PGA 会降低整体功耗， 并可将共模电压范围 (VCM) 扩展为 AVSS – 0.1V 至
AVDD + 0.1V。
只能针对增益 1、 2 和 4 禁用 PGA。
无论 PGA_BYPASS 设置如何， 都始终针对增益设置 8 至 128 启用 PGA。
0： PGA 已启用（默认设置）
1： PGA 已禁用和旁路
***/
int ADS1220SetPGABypass(int Bypass)
{
	unsigned int cBypass = Bypass;
	ADS1220WriteRegister(ADS1220_0_REGISTER, 0x01, &cBypass);
	return ADS1220_NO_ERROR;
}


/***数据速率
控制数据速率设置， 取决于所选工作模式。
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

//设置温度传感器模式
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

/***基准电压选择
用于选择转换所使用的基准电压源。
00： 选择 2.048V 内部基准电压（默认设置）
01： 使用专用 REFP0 和 REFN0 输入选择的外部基准电压
10： 使用 AIN0/REFP1 和 AIN3/REFN1 输入选择的外部基准电压
11： 用作基准的模拟电源 (AVDD – AVSS)
***/
int ADS1220SetVoltageReference(int VoltageRef)
{
	unsigned int cVoltageRef = VoltageRef;
	ADS1220WriteRegister(ADS1220_2_REGISTER, 0x01, &cVoltageRef);
	return ADS1220_NO_ERROR;
}

/***FIR 滤波器配置
用于为内部 FIR 滤波器配置滤波器系数。
在正常模式下， 这些位仅与 20SPS 设置结合使用； 在占空比模式下， 这些位仅与
5SPS 设置结合使用。 对于所有其他数据速率， 这些位均设置为 00。
00： 无 50Hz 或 60Hz 抑制（默认设置）
01： 同时抑制 50Hz 和 60Hz
10： 只抑制 50Hz
11： 只抑制 60Hz
***/
int ADS1220Set50_60Rejection(int Rejection)
{
	unsigned int cRejection = Rejection;
	ADS1220WriteRegister(ADS1220_2_REGISTER, 0x01, &cRejection);
	return ADS1220_NO_ERROR;
}

/***低侧电源开关配置
用于配置 AIN3/REFN1 和 AVSS 之间连接的低侧开关的行为。
0： 开关始终处于断开状态（默认设置）
1： 开关会在发送 START/SYNC 命令时自动闭合， 并在发出 POWERDOWN 命令
时自动断开
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


/*获取寄存器参数*/

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

//设置工作模式
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



