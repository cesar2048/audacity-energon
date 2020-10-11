// for address
#include "osinterface.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

// declare its existence
void DebugLog(const char *format ...);

/// finds ethernet interfaces from the OS
/// type: 4 for IPv4, 6 for IPv6, 0 for both
/// reference: https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
/// reference: https://stackoverflow.com/questions/54314901/where-is-the-ip-address-in-ip-adapter-addresses-lh
vector<string> FindOSInterfaces(int type) {
	/* Declare and initialize variables */
	vector<string> result;

	DWORD dwSize = 0;
	DWORD dwRetVal = 0;

	unsigned int i = 0;

	// Set the flags to pass to GetAdaptersAddresses
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

	// default to unspecified address family (both)
	ULONG family = AF_UNSPEC;
	LPVOID lpMsgBuf = NULL;

	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = 0;
	ULONG Iterations = 0;

	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
	PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
	PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
	IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
	IP_ADAPTER_PREFIX *pPrefix = NULL;

	char msgBuffer[4 * 4 + 1]; // e.g: "192.168.100.128"

	if (type == 4) family = AF_INET;
	if (type == 6) family == AF_INET6;

	DebugLog("Calling GetAdaptersAddresses function with family = ");
	if (family == AF_INET)
		DebugLog("AF_INET\n");
	if (family == AF_INET6)
		DebugLog("AF_INET6\n");
	if (family == AF_UNSPEC)
		DebugLog("AF_UNSPEC\n\n");

	// Allocate a 15 KB buffer to start with.
	outBufLen = WORKING_BUFFER_SIZE;

	do {

		pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(outBufLen);
		if (pAddresses == NULL) {
			DebugLog
			("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
			exit(1);
		}

		dwRetVal =
			GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

		if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
			FREE(pAddresses);
			pAddresses = NULL;
		}
		else {
			break;
		}

		Iterations++;

	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

	if (dwRetVal == NO_ERROR) {
		// If successful, output some information from the data we received
		pCurrAddresses = pAddresses;
		while (pCurrAddresses) {

			// iterate through the unicast addresses
			pUnicast = pCurrAddresses->FirstUnicastAddress;
			while (pUnicast != NULL) {
				SOCKADDR* pAddr = pUnicast->Address.lpSockaddr;
				if (pAddr->sa_family == AF_INET) {
					sockaddr_in* ipv4 = (sockaddr_in*) pAddr;
					sprintf(msgBuffer, "%d.%d.%d.%d",
						ipv4->sin_addr.S_un.S_un_b.s_b1,
						ipv4->sin_addr.S_un.S_un_b.s_b2,
						ipv4->sin_addr.S_un.S_un_b.s_b3,
						ipv4->sin_addr.S_un.S_un_b.s_b4);
					DebugLog("IP: %s\n", msgBuffer);

					result.push_back(string(msgBuffer));
				}
				pUnicast = pUnicast->Next;
			}

			pCurrAddresses = pCurrAddresses->Next;
		}
	}
	else {
		DebugLog("Call to GetAdaptersAddresses failed with error: %d\n",
			dwRetVal);
		if (dwRetVal == ERROR_NO_DATA)
			DebugLog("\tNo addresses were found for the requested parameters\n");
		else {
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				// Default language
				(LPTSTR)& lpMsgBuf, 0, NULL)) {
				DebugLog("\tError: %s", lpMsgBuf);
				LocalFree(lpMsgBuf);
				if (pAddresses)
					FREE(pAddresses);
				exit(1);
			}
		}
	}

	if (pAddresses) {
		FREE(pAddresses);
	}

	return result;
}