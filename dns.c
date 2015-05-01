#include "main.h"
#include "log.h"
#include "syslog.h"

#include <inaddr.h>
#include <winsock2.h> //winsock
#include <windns.h>   //DNS api's
#include <stdio.h>    //standard i/o
#include <Winerror.h>

#define BUFFER 255


static PIP4_ARRAY get_dns_server() 
{
	PIP4_ARRAY pSrvList = NULL;
	char DnsServIp[BUFFER];

	pSrvList = (PIP4_ARRAY) LocalAlloc(LPTR, sizeof(IP4_ARRAY));
    if(!pSrvList) {
		printf("Memory allocation failed \n");
        exit(1);
    }

    strcpy_s(DnsServIp, BUFFER, "8.8.8.8"); // testing
    pSrvList->AddrCount = 1;
    pSrvList->AddrArray[0] = inet_addr(DnsServIp); //DNS server IP address

	return pSrvList;
}

static int dns_ptr(char *pOwnerName) 
{ 
    DNS_STATUS status;               //Return value of  DnsQuery_A() function.
    PDNS_RECORD pDnsRecord;          //Pointer to DNS_RECORD structure.
    PIP4_ARRAY pSrvList = NULL;      //Pointer to IP4_ARRAY structure.
    DNS_FREE_TYPE freetype ;

	//pSrvList = get_dns_server();

	freetype =  DnsFreeRecordListDeep;

    // Calling function DnsQuery to query PTR records  

    status = DnsQuery_A((PCSTR)pOwnerName,         //Pointer to OwnerName. 
                        DNS_TYPE_PTR,              //Type of the record to be queried.
                        DNS_QUERY_BYPASS_CACHE | DNS_QUERY_NO_LOCAL_NAME | DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_NO_NETBT,     // Bypasses windows nonsense. 
                        0, //pSrvList,             //Contains DNS server IP address.
                        &pDnsRecord,               //Resource record that contains the response.
                        NULL);                     //Reserved for future use.

    if (status){
		return 0; // just leave pOwnerName as-is
		if (status != 9003) 
			printf("Failed to query the PTR record and the error is %d \n", status);
    } else {
        //printf("The host name is -> %s  \n",(pDnsRecord->Data.PTR.pNameHost));
		strcpy_s(pOwnerName, BUFFER, pDnsRecord->Data.PTR.pNameHost);
        // Free memory allocated for DNS records. 
        DnsRecordListFree(pDnsRecord, freetype);
    }
    LocalFree(pSrvList);

	return 1;
}

// obviously no ipv6 
static void ReverseIP(char* pIP)
{
    char seps[]   = ".";
    char *token, *context;
    char pIPSec[4][4];
    int i=0;
    token = strtok_s( pIP, seps, &context);

    while( token != NULL )
    {
        /* While there are "." characters in "string" */
        sprintf_s(pIPSec[i], 4, "%s", token);
        /* Get next "." character: */
        token = strtok_s( NULL, seps, &context);
        i++;
    }

    sprintf_s(pIP, BUFFER, "%s.%s.%s.%s.%s", pIPSec[3], pIPSec[2], pIPSec[1], pIPSec[0], "IN-ADDR.ARPA");
}

int get_hostname (char *buffer, int len) 
{
	int rv, i;
	struct hostent *pHost = 0;
	char aszIPAddresses[10][16];    //maximum of ten IP addresses
	struct sockaddr_in SocketAddress;
	char pReversedIP[BUFFER];		//Reversed IP address.

	if (!buffer) return 0;

	rv = gethostname(buffer, len);
	if (rv == 0) {
		pHost = gethostbyname(buffer);
		
		if (pHost) {
			for(i = 0; ((pHost->h_addr_list[i]) && (i < 1)); ++i)
			{
				memcpy(&SocketAddress.sin_addr, pHost->h_addr_list[i], pHost->h_length);
				strcpy_s(aszIPAddresses[i], 16, inet_ntoa(SocketAddress.sin_addr));
				sprintf_s(pReversedIP, BUFFER, "%s", aszIPAddresses[i]);
				ReverseIP(pReversedIP);

				if (dns_ptr(pReversedIP)) 
					strcpy_s(buffer, len, pReversedIP);
				else
					sprintf_s(buffer, BUFFER, "%s", aszIPAddresses[i]);
			}
		} else {
			printf("gethostname rv: %d (pHost == null). cant resolve localhost name/ip address. dont use the -a flag. sorry.\n", rv);
			exit(-1);
		}
	}

	return 1;
}