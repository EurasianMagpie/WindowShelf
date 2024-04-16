// NetworkUtil.h
#ifndef _____X_ADAPTER_UTIL_HEADER_____
#define _____X_ADAPTER_UTIL_HEADER_____

#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iomanip>
#pragma comment(lib, "IPHLPAPI.lib")

#include "LogUtil.h"

#pragma warning(disable: 4996)


namespace NetworkUtil {

static std::string GetLocalTimeString(const time_t* time) {
    std::stringstream ss;
    ss << std::put_time(std::localtime(time), "%c %Z");
    return ss.str();
}

static bool ListAdapterInfo()
{
    LOGI("--- ListAdapterInfo Begin ---");

    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    ULONG ulOutSize = 0;
    DWORD dwResult = 0;
    
    if ((dwResult = GetAdaptersInfo(pAdapterInfo, &ulOutSize)) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutSize);
        if (pAdapterInfo == NULL) {
            LOGI("Alloc Adapter Info Failed");
            return false;
        }
        memset(pAdapterInfo, 0, ulOutSize);

        dwResult = GetAdaptersInfo(pAdapterInfo, &ulOutSize);
    }

    if (dwResult == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            LOGI("\tComboIndex: \t{}", pAdapter->ComboIndex);
            LOGI("\tAdapter Name: \t{}", pAdapter->AdapterName);
            LOGI("\tAdapter Desc: \t{}", pAdapter->Description);
            std::stringstream ssAddr;
            for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                ssAddr << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)pAdapter->Address[i];
                if (i < (pAdapter->AddressLength - 1)) {
                    ssAddr << "-";
                }
            }
            LOGI("\tAdapter Addr: \t{}", ssAddr.str());
            LOGI("\tIndex: \t\t{}", pAdapter->Index);
            switch (pAdapter->Type) {
            case MIB_IF_TYPE_OTHER:
                LOGI("\tType: \t\tOther");
                break;
            case MIB_IF_TYPE_ETHERNET:
                LOGI("\tType: \t\tEthernet");
                break;
            case MIB_IF_TYPE_TOKENRING:
                LOGI("\tType: \t\tToken Ring");
                break;
            case MIB_IF_TYPE_FDDI:
                LOGI("\tType: \t\tFDDI");
                break;
            case MIB_IF_TYPE_PPP:
                LOGI("\tType: \t\tPPP");
                break;
            case MIB_IF_TYPE_LOOPBACK:
                LOGI("\tType: \t\tLookback");
                break;
            case MIB_IF_TYPE_SLIP:
                LOGI("\tType: \t\tSlip");
                break;
            default:
                LOGI("\tType: \t\tUnknown {}", pAdapter->Type);
                break;
            }

            LOGI("\tIP Addr: \t\t{}", pAdapter->IpAddressList.IpAddress.String);
            //LOGI("\tIP Addr: \t\t{}", pAdapter->IpAddressList.IpAddress);
            LOGI("\tIP Mask: \t\t{}", pAdapter->IpAddressList.IpMask.String);

            LOGI("\tGateway: \t\t{}", pAdapter->GatewayList.IpAddress.String);
            LOGI("\t***");

            if (pAdapter->DhcpEnabled) {
                LOGI("\tDHCP Enabled: \tYes");
                LOGI("\t  DHCP Server: \t{}", pAdapter->DhcpServer.IpAddress.String);
                LOGI("\t  Lease Obtained: \t{}", GetLocalTimeString(&pAdapter->LeaseObtained));
                LOGI("\t  Lease Expires:  \t{}", GetLocalTimeString(&pAdapter->LeaseExpires));
            }
            else {
                LOGI("\tDHCP Enabled: \tNo");
            }

            if (pAdapter->HaveWins) {
                LOGI("\tHave Wins: \tYes");
                LOGI("\t  Primary Wins Server:    {}",
                    pAdapter->PrimaryWinsServer.IpAddress.String);
                LOGI("\t  Secondary Wins Server:  {}",
                    pAdapter->SecondaryWinsServer.IpAddress.String);
            }
            else {
                LOGI("\tHave Wins: \tNo");
            }
            pAdapter = pAdapter->Next;
            LOGI(">>> Next >>>");
        }
    }
    else {
        LOGI("GetAdaptersInfo failed, ret: {}", dwResult);
    }

    if (pAdapterInfo) {
        free(pAdapterInfo);
    }

    LOGI("--- ListAdapterInfo End ---");
    return NO_ERROR == dwResult;
}

}

#endif//_____X_ADAPTER_UTIL_HEADER_____
