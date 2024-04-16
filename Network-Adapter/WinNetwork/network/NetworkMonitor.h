// NetworkMonitor.h
#ifndef _____X_NETWORK_MONITOR_HEADER_____
#define _____X_NETWORK_MONITOR_HEADER_____


#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <netlistmgr.h>

#include "LogUtil.h"
#include "NetworkUtil.h"

/*
* INetworkListManagerEvents Impl
*/
class NetworkListManagerEventHandler : public INetworkListManagerEvents
{
public:
	typedef std::function<void(bool)> HandlerFunction;

public:
	NetworkListManagerEventHandler(std::function<void(bool)> fnHandler)
		: m_Ref(1)
		, m_fnHandler(fnHandler)
	{
	}

	~NetworkListManagerEventHandler() {}

public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (!ppvObject) {
			return E_INVALIDARG;
		}

		HRESULT hr(S_OK);
		if (IsEqualIID(riid, IID_IUnknown)) {
			*ppvObject = dynamic_cast<IUnknown*>(this);
			AddRef();
		}
		else if (IsEqualIID(riid, IID_INetworkListManagerEvents)) {
			*ppvObject = dynamic_cast<INetworkListManagerEvents*>(this);
			AddRef();
		}
		else {
			hr = E_NOINTERFACE;
		}
		return hr;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef() override
	{
		return ++m_Ref;
	}

	virtual ULONG STDMETHODCALLTYPE Release() override
	{
		long ref = --m_Ref;
		if (ref == 0) {
			delete this;
		}
		return ref;
	}

	virtual HRESULT STDMETHODCALLTYPE ConnectivityChanged(
		/* [in] */ NLM_CONNECTIVITY newConnectivity) override
	{
		if (newConnectivity == NLM_CONNECTIVITY_DISCONNECTED)
		{
			// DISCONNECTED
			m_fnHandler(false);
		}
		else if ((newConnectivity & NLM_CONNECTIVITY_IPV4_INTERNET)
			|| (newConnectivity & NLM_CONNECTIVITY_IPV6_INTERNET))
		{
			// CONNECTED
			m_fnHandler(true);
		}
		return S_OK;
	}

private:
	std::atomic_long m_Ref;
	std::function<void(bool)> m_fnHandler;
};


/*
* NetworkMonitor
* Not thread safe, one instance this class can only be used in same thread.
*/
class NetworkMonitor
{
public:
    static NetworkMonitor& getInstance() {
        static NetworkMonitor inst;
        return inst;
    }

public:
    NetworkMonitor()
		: m_bIsConnected(false), m_pConnectPoint(NULL), m_dwSinkCookie(0)
	{
		CoInitialize(NULL);
	}

    ~NetworkMonitor()
	{
		stop();
		CoUninitialize();
	}

private:
    NetworkMonitor(const NetworkMonitor&) = delete;
    NetworkMonitor& operator=(const NetworkMonitor&) = delete;

	HRESULT acquireConnectionPoint(IConnectionPoint** ppConnectPoint) {
		IUnknown* pUnknown = NULL;
		HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, IID_IUnknown, (void**)&pUnknown);
		if (SUCCEEDED(hr))
		{
			INetworkListManager* pNetworkListManager = NULL;
			hr = pUnknown->QueryInterface(IID_INetworkListManager, (void**)&pNetworkListManager);
			if (SUCCEEDED(hr))
			{
				VARIANT_BOOL varIsConnect = VARIANT_FALSE;
				hr = pNetworkListManager->get_IsConnectedToInternet(&varIsConnect);
				if (SUCCEEDED(hr))
				{
					m_bIsConnected = varIsConnect == VARIANT_TRUE;
					LOGI("[WinNetwork] acquireConnectionPoint | IsConnect: {}", (varIsConnect ? "true" : "false"));
				}

				IConnectionPointContainer* pCPContainer = NULL;
				hr = pNetworkListManager->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPContainer);
				if (SUCCEEDED(hr))
				{
					IConnectionPoint* pConnectPoint = NULL;
					hr = pCPContainer->FindConnectionPoint(IID_INetworkListManagerEvents, &pConnectPoint);
					if (SUCCEEDED(hr))
					{
						*ppConnectPoint = pConnectPoint;
					}
					pCPContainer->Release();
				}
				pNetworkListManager->Release();
			}
			pUnknown->Release();
		}
		return hr;
	}

	void releaseConnectionPoint() {
		if (m_pConnectPoint) {
			m_pConnectPoint->Release();
			m_pConnectPoint = NULL;
		}
	}

public:
	HRESULT start() {
		HRESULT hr(S_OK);
		if (!m_pConnectPoint) {
			IConnectionPoint* pConnectPoint(NULL);
			hr = acquireConnectionPoint(&pConnectPoint);
			if (SUCCEEDED(hr)) {
				m_pConnectPoint = pConnectPoint;
			}
			else {
				return hr;
			}
		}

		unregisterNetworkListManagerEvents();

		return registerNetworkListManagerEvents();
	}

	void stop() {
		unregisterNetworkListManagerEvents();
		releaseConnectionPoint();
	}

private:
	static void onConnectivityChanged(bool isConnected) {
		getInstance().setIsConnected(isConnected);
		LOGI("[WinNetwork] onConnectivityChanged | IsConnect: {}", (isConnected ? "true" : "false"));
		NetworkUtil::ListAdapterInfo();
	}

	HRESULT registerNetworkListManagerEvents() {
		if (!m_pConnectPoint) {
			return E_FAIL;
		}

		HRESULT hr(S_OK);
		NetworkListManagerEventHandler* pEventHandler =
			new(std::nothrow) NetworkListManagerEventHandler(NetworkMonitor::onConnectivityChanged);
		if (pEventHandler)
		{
			DWORD dwCookie(0);
			hr = m_pConnectPoint->Advise((IUnknown*)pEventHandler, &dwCookie);
			if (SUCCEEDED(hr))
			{
				LOGI("[WinNetwork] registerNetworkListManagerEvents | ConnectPoint Advise success");
				m_dwSinkCookie = dwCookie;
			}
			pEventHandler->Release();
		}

		return hr;
	}

	void unregisterNetworkListManagerEvents() {
		if (m_pConnectPoint && m_dwSinkCookie != 0) {
			m_pConnectPoint->Unadvise(m_dwSinkCookie);
			m_dwSinkCookie = 0;
			LOGI("[WinNetwork] unregisterNetworkListManagerEvents | ConnectPoint Unadvise");
		}
	}

	void setIsConnected(bool isConnected) {
		m_bIsConnected = isConnected;
	}

	bool getIsConnected() const {
		return m_bIsConnected;
	}

private:
    bool m_bIsConnected;
    
	IConnectionPoint* m_pConnectPoint;
	DWORD m_dwSinkCookie;
};

#endif//_____X_NETWORK_MONITOR_HEADER_____
