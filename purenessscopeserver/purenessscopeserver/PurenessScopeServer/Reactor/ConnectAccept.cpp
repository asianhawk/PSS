#include "ConnectAccept.h"

int ConnectConsoleAcceptor::make_svc_handler (CConsoleHandler *&sh)
{
	//如果正在处理的链接超过了服务器设定的数值，则不允许链接继续链接服务器
	CConsoleHandler* pConsoleHandler = new CConsoleHandler();
	if(NULL != pConsoleHandler)
	{
		pConsoleHandler->reactor(this->reactor());
		sh = pConsoleHandler;
		return 0;
	}
	else
	{
		return -1;
	}
}


//==============================================================================

int ConnectAcceptor::make_svc_handler (CConnectHandler *&sh)
{
	//如果正在处理的链接超过了服务器设定的数值，则不允许链接继续链接服务器
	if(App_ConnectHandlerPool::instance()->GetUsedCount() > App_MainConfig::instance()->GetMaxHandlerCount())
	{
		OUR_DEBUG((LM_ERROR, "[ConnectAcceptor::make_svc_handler]Connect is more MaxHandlerCount(%d > %d).\n", App_ConnectHandlerPool::instance()->GetUsedCount(), App_MainConfig::instance()->GetMaxHandlerCount()));
		//不允许链接
		return -1;
	}
	else
	{
		//允许链接
		CConnectHandler* pConnectHandler = App_ConnectHandlerPool::instance()->Create();
		if(NULL != pConnectHandler)
		{
			pConnectHandler->reactor(this->reactor());
			sh = pConnectHandler;
			return 0;
		}
		else
		{
			return -1;
		}
	}
}

int ConnectAcceptor::open2(ACE_INET_Addr& local_addr, ACE_Reactor *reactor, int flags, int backlog)
{
	ACE_TRACE ("ACE_Acceptor<SVC_HANDLER, PEER_ACCEPTOR>::open");
	this->flags_ = flags;
	this->use_select_ = 1;
	this->reuse_addr_ = 1;
	this->peer_acceptor_addr_ = local_addr;

	// Must supply a valid Reactor to Acceptor::open()...

	if (reactor == 0)
	{
		errno = EINVAL;
		return -1;
	}

	// Open the underlying PEER_ACCEPTOR.
	if (this->peer_acceptor_.open (local_addr, 1, 0, backlog) == -1)
		return -1;

	// Set the peer acceptor's handle into non-blocking mode.  This is a
	// safe-guard against the race condition that can otherwise occur
	// between the time when <select> indicates that a passive-mode
	// socket handle is "ready" and when we call <accept>.  During this
	// interval, the client can shutdown the connection, in which case,
	// the <accept> call can hang!
	(void) this->peer_acceptor_.enable (ACE_NONBLOCK);

	int const result = reactor->register_handler (this,
		ACE_Event_Handler::ACCEPT_MASK);
	if (result != -1)
		this->reactor (reactor);
	else
		this->peer_acceptor_.close ();

	return result;
}



//==============================================================================

CConnectAcceptorManager::CConnectAcceptorManager(void)
{
	m_nAcceptorCount = 0;
	m_szError[0]     = '\0';
}

CConnectAcceptorManager::~CConnectAcceptorManager(void)
{
	OUR_DEBUG((LM_INFO, "[CConnectAcceptorManager::~CConnectAcceptorManager].\n"));
	Close();
	OUR_DEBUG((LM_INFO, "[CConnectAcceptorManager::~CConnectAcceptorManager]End.\n"));
}

bool CConnectAcceptorManager::InitConnectAcceptor(int nCount)
{
	try
	{
		Close();

		for(int i = 0; i < nCount; i++)
		{
			ConnectAcceptor* pConnectAcceptor = new ConnectAcceptor();
			if(NULL == pConnectAcceptor)
			{
				throw "[CConnectAcceptorManager::InitConnectAcceptor]pConnectAcceptor new is fail.";
			}

			m_vecConnectAcceptor.push_back(pConnectAcceptor);
		}

		return true;
	}
	catch (const char* szError)
	{
		sprintf_safe(m_szError, MAX_BUFF_500, "%s", szError);
		return false;
	}
}

void CConnectAcceptorManager::Close()
{
	for(int i = 0; i < (int)m_vecConnectAcceptor.size(); i++)
	{
		ConnectAcceptor* pConnectAcceptor = (ConnectAcceptor* )m_vecConnectAcceptor[i];
		if(NULL != pConnectAcceptor)
		{
			pConnectAcceptor->close();
			delete pConnectAcceptor;
			pConnectAcceptor = NULL;
		}
	}

	m_vecConnectAcceptor.clear();
	m_nAcceptorCount = 0;
}

int CConnectAcceptorManager::GetCount()
{
	return (int)m_vecConnectAcceptor.size();
}

ConnectAcceptor* CConnectAcceptorManager::GetConnectAcceptor(int nIndex)
{
	if(nIndex < 0 || nIndex >= (int)m_vecConnectAcceptor.size())
	{
		return NULL;
	}

	return (ConnectAcceptor* )m_vecConnectAcceptor[nIndex];
}

const char* CConnectAcceptorManager::GetError()
{
	return m_szError;
}
