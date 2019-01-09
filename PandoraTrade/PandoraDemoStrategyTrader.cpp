// ArbitrageStrategyTrader.cpp : Defines the entry point for the console application.
//

#include <thread>
#include <iostream>

#include <string.h>
#include "cwFtdMdSpi.h"
#include "cwFtdTradeSpi.h"
#include "cwStrategyDemo.h"
#include "tinyxml.h"

#ifdef _MSC_VER
#pragma comment(lib, "cwCTPDLL.lib")
#pragma comment(lib, "tinyxml.lib")
#endif // WIN32

//#define WCS_DISCLAIMER
//#define WCS_USER_AUTHORIZE

//�����򻥳����������ж��Ƿ��иó���������
#ifdef WIN32
HANDLE  m_hAppMutex(NULL);
#endif

#ifndef MAX_PATH
#define MAX_PATH          260
#endif // !MAX_PATH

//price Server
cwFtdMdSpi				m_mdCollector;
cwFtdTradeSpi			m_TradeChannel;
cwStrategyDemo			m_cwTBSignalStategy;

//XML Config Parameter
char		 m_szMdFront[64];
TThostFtdcBrokerIDType	m_szMdBrokerID;
TThostFtdcUserIDType	m_szMdUserID;
TThostFtdcPasswordType	m_szMdPassWord;

char		 m_szTdFront[64];
TThostFtdcBrokerIDType	m_szTdBrokerID;
TThostFtdcUserIDType	m_szTdUserID;
TThostFtdcPasswordType	m_szTdPassWord;

std::vector<std::string> m_SubscribeInstrument;

std::string				m_strStrategyConfigFile;

std::set<std::string>	m_AuthorizeUserIDSet;

#ifdef WIN32
#define GetCharElement(Type, Name) const char * psz##Name = Element->Attribute(#Name);\
if (psz##Name != NULL)\
{\
	strcpy_s(m_sz##Type##Name, psz##Name);\
}
#else
#define GetCharElement(Type, Name) const char * psz##Name = Element->Attribute(#Name);\
if (psz##Name != NULL)\
{\
	strcpy(m_sz##Type##Name, psz##Name);\
}
#endif // WIN32


bool ReadXmlConfigFile()
{
	char exeFullPath[MAX_PATH];
	memset(exeFullPath, 0, MAX_PATH);
	std::string strFullPath;
#ifdef WIN32
	WCHAR TexeFullPath[MAX_PATH] = { 0 };

	GetModuleFileName(NULL, TexeFullPath, MAX_PATH);
	int iLength;
	//��ȡ�ֽڳ���   
	iLength = WideCharToMultiByte(CP_ACP, 0, TexeFullPath, -1, NULL, 0, NULL, NULL);
	//��tcharֵ����_char    
	WideCharToMultiByte(CP_ACP, 0, TexeFullPath, -1, exeFullPath, iLength, NULL, NULL);
#else
	size_t cnt = readlink("/proc/self/exe", exeFullPath, MAX_PATH);
	if (cnt < 0 || cnt >= MAX_PATH)
	{
		printf("***Error***\n");
		exit(-1);
	}
#endif // WIN32

	strFullPath = exeFullPath;
	strFullPath = strFullPath.substr(0, strFullPath.find_last_of("/\\"));

#ifdef WIN32
	strFullPath.append("\\PegasusTraderConfig.xml");
#else
	strFullPath.append("/PegasusTraderConfig.xml");
#endif // WIN32


	std::cout << "Get Account Config File: " << strFullPath.c_str() << std::endl;

	TiXmlDocument doc(strFullPath.c_str());
	bool loadOkay = doc.LoadFile(TIXML_ENCODING_LEGACY);

	if (!loadOkay)
	{
		std::cout << "Load Config File Failed ! " << std::endl;
		return false;
		//WriteXmlConfigFile();
	}

	TiXmlNode* RootNode = doc.RootElement();
	if (RootNode != NULL)
	{
		//Read General
		TiXmlNode* ChildNode = RootNode->FirstChild("User");
		if (ChildNode != NULL)
		{
			TiXmlNode* SubChildNode = ChildNode->FirstChild("MarketDataServer");
			if (SubChildNode != NULL)
			{
				TiXmlElement * Element = SubChildNode->ToElement();
				GetCharElement(Md, Front);
				GetCharElement(Md, BrokerID);
				GetCharElement(Md, UserID);
				GetCharElement(Md, PassWord);
			}

			SubChildNode = ChildNode->FirstChild("TradeServer");
			if (SubChildNode != NULL)
			{
				TiXmlElement * Element = SubChildNode->ToElement();
				GetCharElement(Td, Front);
				GetCharElement(Td, BrokerID);
				GetCharElement(Td, UserID);
				GetCharElement(Td, PassWord);
			}
		}

		ChildNode = RootNode->FirstChild("Subscription");
		if (ChildNode != NULL)
		{
			TiXmlNode* SubChildNode = ChildNode->FirstChild("Instrument");
			while (SubChildNode != NULL)
			{
				TiXmlElement * Element = SubChildNode->ToElement();
				const char * pszTemp = Element->Attribute("ID");
				if (pszTemp != NULL)
				{
					m_SubscribeInstrument.push_back(pszTemp);
				}
				SubChildNode = SubChildNode->NextSibling("Instrument");
			}
		}

		m_strStrategyConfigFile.clear();
		ChildNode = RootNode->FirstChild("StrategyConfigFile");
		if (ChildNode != NULL)
		{
			TiXmlElement * Element = ChildNode->ToElement();
			const char * pszTemp = Element->GetText();
			if (pszTemp != NULL)
			{
				m_strStrategyConfigFile = pszTemp;
			}
		}
	}

	return true;
}


unsigned int PriceServerThread()
{
	
	m_mdCollector.SetUserLoginField(m_szMdBrokerID, m_szMdUserID, m_szMdPassWord);
	m_mdCollector.SubscribeMarketData(m_SubscribeInstrument);

	m_mdCollector.Connect(m_szMdFront);
	m_mdCollector.WaitForFinish();
	return 0;
}

unsigned int TradeServerThread()
{
	m_TradeChannel.SetUserLoginField(m_szTdBrokerID, m_szTdUserID, m_szTdPassWord);
	m_TradeChannel.Connect(m_szTdFront);
	m_TradeChannel.WaitForFinish();
	return 0;
}

#ifdef WIN32
bool CtrlHandler(DWORD fdwCtrlType)
{
	m_mdCollector.DisConnect();
	m_TradeChannel.DisConnect();

#ifdef WIN32
	if (m_hAppMutex != NULL)
	{
		ReleaseMutex(m_hAppMutex);
		CloseHandle(m_hAppMutex);
		m_hAppMutex = NULL;
	}
#endif

	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal.   
	case CTRL_C_EVENT:
		printf("Ctrl-C event\n\n");
		//Beep(750, 300);
		return(TRUE);

		// CTRL-CLOSE: confirm that the user wants to exit.   
	case CTRL_CLOSE_EVENT:
		//Beep(600, 200);
		printf("Ctrl-Close event\n\n");
		return(TRUE);

		// Pass other signals to the next handler.   
	case CTRL_BREAK_EVENT:
		//Beep(900, 200);
		printf("Ctrl-Break event\n\n");
		return FALSE;

	case CTRL_LOGOFF_EVENT:
		//Beep(1000, 200);
		printf("Ctrl-Logoff event\n\n");
		return FALSE;

	case CTRL_SHUTDOWN_EVENT:
		//Beep(750, 500);
		printf("Ctrl-Shutdown event\n\n");
		return FALSE;

	default:
		return FALSE;
	}
}
#endif // WIN32

int main()
{

#ifdef WIN32
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		printf("\nThe Control Handler is uninstalled.\n");
		return 0;
	}
#endif // WIN32

	std::cout << "Welcome To Pandora Trader !!" << std::endl;

	std::cout << "Init Config From File!" << std::endl;
	if (!ReadXmlConfigFile())
	{
		std::cout << "Init Config Failed!!" << std::endl;
		std::cout << "The Program will shut down in 5 seconds��" << std::endl;
		
		int nCnt = 0;
		while (nCnt < 6)
		{
			cwSleep(1000);
			std::cout << nCnt << " . " << std::endl;
			nCnt++;
		}

		return -1;
	}
	std::cout << "User: " << m_szTdUserID << std::endl;

	if (m_strStrategyConfigFile.size() == 0)
	{
		m_cwTBSignalStategy.InitialStrategy(NULL);
	}
	else
	{
		m_cwTBSignalStategy.InitialStrategy(m_strStrategyConfigFile.c_str());
	}

	//����mutex ��ֹһ�����򿪶��
	std::string strAppMutexName;
	strAppMutexName = m_szTdUserID;
	strAppMutexName.append("_");
	strAppMutexName += m_cwTBSignalStategy.GetStrategyName().c_str();

#ifdef WIN32
	int  unicodeLen = ::MultiByteToWideChar(CP_ACP,	0, strAppMutexName.c_str(),	-1,	NULL, 0);
	wchar_t  * TAppMutexName = new wchar_t[unicodeLen + 1];
	memset(TAppMutexName, 0, (unicodeLen + 1)*sizeof(wchar_t));
	::MultiByteToWideChar(CP_ACP, 0, strAppMutexName.c_str(), -1,(LPWSTR)TAppMutexName,	unicodeLen);

	//���������壬ͬһ������ֻ������һ�Σ�����������Σ�������ERROR_ALREADY_EXISTS����
	m_hAppMutex = ::CreateMutex(NULL, TRUE, TAppMutexName);
	if (m_hAppMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		std::cout << "�Ѿ���⵽һ���Ĳ��Գ��������У��벻Ҫ�ظ��򿪲��Գ���" << std::endl;
		std::cout << "������5����Զ��˳���" << std::endl;
		CloseHandle(m_hAppMutex);
		m_hAppMutex = NULL;
		delete [] TAppMutexName;

		int nCnt = 0;
		while (nCnt < 6)
		{
			cwSleep(1000);
			std::cout << nCnt << " . " << std::endl;	
			nCnt++;
		}
		
		return -1;
	}

	delete [] TAppMutexName;
#endif

	std::string strStrategyName = m_cwTBSignalStategy.GetStrategyName();

	m_TradeChannel.RegisterBasicStrategy(&m_cwTBSignalStategy);

	m_mdCollector.RegisterTradeSPI(&m_TradeChannel);
	m_mdCollector.RegisterStrategy(&m_cwTBSignalStategy);

	std::thread m_PriceServerThread = std::thread(PriceServerThread);

	std::thread m_TradeServerThread = std::thread(TradeServerThread);

	
	int iCnt = 0;
	while (1)
	{
		iCnt++;
		if (iCnt % 20 == 0)
		{
			if (iCnt % 80 == 0)
			{
				std::cout << m_szTdUserID << " " << strStrategyName.c_str() << std::endl;
			}
			cwAccountPtr pAccount = m_TradeChannel.GetAccount();
			if (pAccount.get() != NULL)
			{
				std::cout << m_cwTBSignalStategy.m_strCurrentUpdateTime.c_str()
					<< " Total��" << pAccount->Balance 
					<< " Available��" << pAccount->Available
					<< " PL��"	<< pAccount->CloseProfit + pAccount->PositionProfit - pAccount->Commission 
					<< " Fee��" << pAccount->Commission << std::endl;
			}

			std::map<std::string, cwPositionPtr> PosMap = m_TradeChannel.GetPosition();
			for (auto it = PosMap.begin(); it != PosMap.end(); it++)
			{
				if (it->second.get() != NULL
					&& (it->second->GetLongTotalPosition() > 0
						|| it->second->GetShortTotalPosition() > 0)) 
				{
					std::cout << it->first.c_str()
						<< " Long:" << it->second->GetLongTotalPosition() << " "
						<< it->second->GetLongTodayPosition() << " "
						<< it->second->GetLongYdPosition() << " "
						<< it->second->GetLongAveragePosPrice()
						<< " Short:" << it->second->GetShortTotalPosition() << " "
						<< it->second->GetShortTodayPosition() << " "
						<< it->second->GetShortYdPosition() << " "
						<< it->second->GetShortAveragePosPrice() << std::endl;
				}
			}
		}
		cwSleep(1000);
	}
    return 0;
}