// for address
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
void FindOSInterfaces(int type = 0) {
	/* Declare and initialize variables */

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
			DebugLog("\tLength of the IP_ADAPTER_ADDRESS struct: %ld\n",
				pCurrAddresses->Length);
			DebugLog("\tIfIndex (IPv4 interface): %u\n", pCurrAddresses->IfIndex);
			DebugLog("\tAdapter name: %s\n", pCurrAddresses->AdapterName);

			pUnicast = pCurrAddresses->FirstUnicastAddress;
			if (pUnicast != NULL) {
				for (i = 0; pUnicast != NULL; i++)
					pUnicast = pUnicast->Next;
				DebugLog("\tNumber of Unicast Addresses: %d\n", i);
			}
			else
				DebugLog("\tNo Unicast Addresses\n");


			// iterate through the unicast addresses
			pUnicast = pCurrAddresses->FirstUnicastAddress;
			while (pUnicast != NULL) {
				SOCKADDR* pAddr = pUnicast->Address.lpSockaddr;
				if (pAddr->sa_family == AF_INET) {
					sockaddr_in* ipv4 = (sockaddr_in*) pAddr;
					DebugLog("%d.%d.%d.%d\n",
						ipv4->sin_addr.S_un.S_un_b.s_b1,
						ipv4->sin_addr.S_un.S_un_b.s_b2,
						ipv4->sin_addr.S_un.S_un_b.s_b3,
						ipv4->sin_addr.S_un.S_un_b.s_b4);
				}
				pUnicast = pUnicast->Next;
			}

			pAnycast = pCurrAddresses->FirstAnycastAddress;
			if (pAnycast) {
				for (i = 0; pAnycast != NULL; i++)
					pAnycast = pAnycast->Next;
				DebugLog("\tNumber of Anycast Addresses: %d\n", i);
			}
			else
				DebugLog("\tNo Anycast Addresses\n");

			pMulticast = pCurrAddresses->FirstMulticastAddress;
			if (pMulticast) {
				for (i = 0; pMulticast != NULL; i++)
					pMulticast = pMulticast->Next;
				DebugLog("\tNumber of Multicast Addresses: %d\n", i);
			}
			else
				DebugLog("\tNo Multicast Addresses\n");

			pDnServer = pCurrAddresses->FirstDnsServerAddress;
			if (pDnServer) {
				for (i = 0; pDnServer != NULL; i++)
					pDnServer = pDnServer->Next;
				DebugLog("\tNumber of DNS Server Addresses: %d\n", i);
			}
			else
				DebugLog("\tNo DNS Server Addresses\n");

			DebugLog("\tDNS Suffix: %wS\n", pCurrAddresses->DnsSuffix);
			DebugLog("\tDescription: %wS\n", pCurrAddresses->Description);
			DebugLog("\tFriendly name: %wS\n", pCurrAddresses->FriendlyName);

			if (pCurrAddresses->PhysicalAddressLength != 0) {
				DebugLog("\tPhysical address: ");
				for (i = 0; i < (int)pCurrAddresses->PhysicalAddressLength;
					i++) {
					if (i == (pCurrAddresses->PhysicalAddressLength - 1))
						DebugLog("%.2X\n",
						(int)pCurrAddresses->PhysicalAddress[i]);
					else
						DebugLog("%.2X-",
						(int)pCurrAddresses->PhysicalAddress[i]);
				}
			}
			DebugLog("\tFlags: %ld\n", pCurrAddresses->Flags);
			DebugLog("\tMtu: %lu\n", pCurrAddresses->Mtu);
			DebugLog("\tIfType: %ld\n", pCurrAddresses->IfType);
			DebugLog("\tOperStatus: %ld\n", pCurrAddresses->OperStatus);
			DebugLog("\tIpv6IfIndex (IPv6 interface): %u\n",
				pCurrAddresses->Ipv6IfIndex);
			DebugLog("\tZoneIndices (hex): ");
			for (i = 0; i < 16; i++)
				DebugLog("%lx ", pCurrAddresses->ZoneIndices[i]);
			DebugLog("\n");

			DebugLog("\tTransmit link speed: %I64u\n", pCurrAddresses->TransmitLinkSpeed);
			DebugLog("\tReceive link speed: %I64u\n", pCurrAddresses->ReceiveLinkSpeed);

			pPrefix = pCurrAddresses->FirstPrefix;
			if (pPrefix) {
				for (i = 0; pPrefix != NULL; i++)
					pPrefix = pPrefix->Next;
				DebugLog("\tNumber of IP Adapter Prefix entries: %d\n", i);
			}
			else
				DebugLog("\tNumber of IP Adapter Prefix entries: 0\n");

			DebugLog("\n");

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
}