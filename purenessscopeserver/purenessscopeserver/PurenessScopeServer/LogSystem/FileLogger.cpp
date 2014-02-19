#include "FileLogger.h"



//********************************************************

CFileLogger::CFileLogger()
{
	m_nCount       = 0;
	m_u4BlockSize  = 0;
	m_u4PoolCount  = 0;
	m_szLogRoot[0] = '\0';
}

CFileLogger::~CFileLogger()
{
	OUR_DEBUG((LM_INFO, "[CFileLogger::~CFileLogger].\n"));
	for(mapLogFile::iterator b = m_mapLogFile.begin(); b!= m_mapLogFile.end(); b++)
	{
		CLogFile* pLogFile = (CLogFile* )b->second;
		SAFE_DELETE(pLogFile);
	}

	m_mapLogFile.clear();
	m_vecLogType.clear();
	OUR_DEBUG((LM_INFO, "[CFileLogger::~CFileLogger]End.\n"));
}

int CFileLogger::DoLog(int nLogType, _LogBlockInfo* pLogBlockInfo)
{
	mapLogFile::iterator f = m_mapLogFile.find(nLogType);
	if(f == m_mapLogFile.end())
	{
		return -1;
	}
	else
	{
		CLogFile* pLogFile = (CLogFile* )f->second;
		pLogFile->doLog(pLogBlockInfo);
	}		

	return 0;
}

int CFileLogger::GetLogTypeCount()
{
	return (int)m_vecLogType.size();
}

int CFileLogger::GetLogType(int nIndex)
{
	if(nIndex >= (int)m_vecLogType.size())
	{
		return 0;
	}

	return (int)m_vecLogType[nIndex];
}

bool CFileLogger::Init()
{
	CXmlOpeation objXmlOpeation;
	uint16 u2LogID                  = 0;
	uint8 u1FileClass               = 0;
	uint8 u1DisPlay                 = 0;
	char szFile[MAX_BUFF_1024]      = {'\0'};
	char szFileName[MAX_BUFF_100]   = {'\0'};
	char szServerName[MAX_BUFF_100] = {'\0'};
	char* pData = NULL;

	m_vecLogType.clear();

	sprintf_safe(szFile, MAX_BUFF_1024, "%s%s", App_MainConfig::instance()->GetModulePath(), FILELOG_CONFIG);
	if(false == objXmlOpeation.Init(szFile))
	{
		OUR_DEBUG((LM_ERROR,"[CFileLogger::Init] Read Configfile[%s] failed\n", szFile));
		return false; 
	}

	//�õ�����������
	//conf.GetValue("ServferName",strServerName,"\\ServerInfo");
	pData = objXmlOpeation.GetData("ServerLogHead", "Text");
	if(pData != NULL)
	{
		sprintf_safe(szServerName, MAX_BUFF_100, "%s", pData);
	}
	OUR_DEBUG((LM_ERROR, "[CFileLogger::readConfig]strServerName=%s\n", szServerName));	

	//�õ�����·��
	pData = objXmlOpeation.GetData("LogPath", "Path");
	if(pData != NULL)
	{
		sprintf_safe(m_szLogRoot, MAX_BUFF_100, "%s", pData);
	}
	OUR_DEBUG((LM_ERROR, "[CFileLogger::readConfig]m_strRoot=%s\n", m_szLogRoot));

	//�õ���־��������Ϣ����־��Ĵ�С
	pData = objXmlOpeation.GetData("LogPool", "BlockSize");
	if(pData != NULL)
	{
		m_u4BlockSize = (uint32)ACE_OS::atoi(pData);
	}

	//�õ���־��������Ϣ�����������־��ĸ���
	pData = objXmlOpeation.GetData("LogPool", "PoolCount");
	if(pData != NULL)
	{
		m_u4PoolCount = (uint32)ACE_OS::atoi(pData);
	}

	//��������ĸ���
	TiXmlElement* pNextTiXmlElement        = NULL;
	TiXmlElement* pNextTiXmlElementPos     = NULL;
	TiXmlElement* pNextTiXmlElementIdx     = NULL;
	TiXmlElement* pNextTiXmlElementDisplay = NULL;

	while(true)
	{
		//�õ���־id
		pData = objXmlOpeation.GetData("LogInfo", "logid", pNextTiXmlElementIdx);  
		if(pData != NULL)
		{
			u2LogID = (uint16)atoi(pData);                                                      
			OUR_DEBUG((LM_ERROR, "[CFileLogger::readConfig]u2LogID=%d\n", u2LogID));
		}
		else
		{
			break;
		}

		//�õ���־����
		pData = objXmlOpeation.GetData("LogInfo", "logname", pNextTiXmlElement);
		if(pData != NULL)
		{
			sprintf_safe(szFileName, MAX_BUFF_100, "%s", pData);
			OUR_DEBUG((LM_ERROR, "[CFileLogger::readConfig]strFileValue=%s\n", szFileName));
		}
		else
		{
			break;
		}

		//�õ���־����
		pData = objXmlOpeation.GetData("LogInfo", "logtype", pNextTiXmlElementPos);  
		if(pData != NULL)
		{
			u1FileClass = (uint8)atoi(pData);                                                      
			OUR_DEBUG((LM_ERROR, "[CFileLogger::readConfig]u1FileClass=%d\n", u1FileClass));
		}
		else
		{
			break;
		}

		//�õ���־�����Դ��0Ϊ������ļ���1Ϊ�������Ļ
		pData = objXmlOpeation.GetData("LogInfo", "Display", pNextTiXmlElementDisplay);  
		if(pData != NULL)
		{
			u1DisPlay = (uint8)atoi(pData);                                                      
			OUR_DEBUG((LM_ERROR, "[CFileLogger::readConfig]u1DisPlay=%d\n", u1DisPlay));
		}
		else
		{
			break;
		}

		//���ӵ�������־�ļ�����map��
		mapLogFile::iterator f = m_mapLogFile.find(u2LogID);

		if(f != m_mapLogFile.end())
		{
			continue;
		}

		CLogFile* pLogFile = new CLogFile(m_szLogRoot, m_u4BlockSize);

		pLogFile->SetLoggerName(szFileName);
		pLogFile->SetLoggerType((int)u2LogID);
		pLogFile->SetLoggerClass((int)u1FileClass);
		pLogFile->SetServerName(szServerName);
		pLogFile->SetDisplay(u1DisPlay);
		pLogFile->Run();

		m_mapLogFile.insert(mapLogFile::value_type(pLogFile->GetLoggerType(), pLogFile));
		m_vecLogType.push_back(u2LogID);

	}

	return true;
}

bool CFileLogger::Close()
{
	for(mapLogFile::iterator b = m_mapLogFile.begin(); b != m_mapLogFile.end(); b++)
	{
		CLogFile* pLogFile = (CLogFile* )b->second;
		delete pLogFile;
	}

	m_mapLogFile.clear();
	m_vecLogType.clear();
	m_nCount = 0;

	return true;
}

uint32 CFileLogger::GetBlockSize()
{
	return m_u4BlockSize;
}

uint32 CFileLogger::GetPoolCount()
{
	return m_u4PoolCount;
}
