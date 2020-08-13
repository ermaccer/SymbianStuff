#include <string.h>
#include <in_sock.h>    
#include <CommDbConnPref.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>


extern "C" void GetLocalHostInfo(unsigned int* ip, int* iap_id, char* localname);
extern "C" struct hostent* gethostbyname_iap (const char* name, int iap_id);

void GetLocalHostInfo(unsigned int* ip, int* iap_id, char* localname)
    {
    RSocketServ socketServ;
    RSocket socket;
    TPckgBuf<TSoInetInterfaceInfo> info;
    TPckgBuf<TSoInetIfQuery> query;
    TUint32 local_ip = 0;  
    TInt ap_id = 0;
    RConnection conn;
    TCommDbConnPref pref;
    RHostResolver r;
    
    
    socketServ.Connect();
    
    socket.Open(socketServ,
                         KAfInet,
                         KSockDatagram,
                         KProtocolInetUdp);                    
    socket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);

    while (socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info) == KErrNone)
        {
        if (info().iAddress.Address())
            {
            local_ip = info().iAddress.Address();
            query().iName = info().iName;
            if(socket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, query) == KErrNone)
                {
                ap_id = query().iZone[1];
                }
            }
        
        }
    if (local_ip)
        {
        pref.SetIapId(ap_id);
        pref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt); 
        conn.Open(socketServ, KAfInet);
        conn.Start(pref);
        r.Open(socketServ, KAfInet, KProtocolInetUdp, conn);
        TInetAddr inetaddr;
        inetaddr.SetFamily(KAfInet);
        inetaddr.SetAddress(local_ip);
        TNameEntry name;
        r.GetByAddress(inetaddr, name);
        r.Close();
        if (name().iName.Length())
            {
            TPtr8 resolvedname((unsigned char*)localname, name().iName.Length()+1);
            resolvedname.Copy(name().iName);
            resolvedname.ZeroTerminate();
            }
        else
            {
            strcpy(localname, "noname");
            }
        conn.Close();
        }
    else
        {
        ap_id =0;
        strcpy(localname, "localhost");
        }
    socket.Close();
    socketServ.Close();
    *ip = local_ip;
    *iap_id = ap_id; 
    }
    




class TSockAddrDummy: public TSockAddr
    {
    public:
    // needed to get access to UserPtr() for HostNameResolver
    TUint8* UserPtr()
        {
        return TSockAddr::UserPtr();
        }
    };


    
struct hostent_buf
    {
    struct hostent iHostent;
    struct sockaddr iAddr;
    char* iPtrs[2]; 
    char iName[1];  
    };
    
    //
    // Extract a struct sockaddr from a TSockAddr
    //
void Get( TSockAddrDummy* record, TAny* addr, unsigned long* len )
    {
        if (addr == 0)
            return;
        
        struct sockaddr* sp = (struct sockaddr*)addr;
        
        TUint16 port = (TUint16)record->Port();
        
        if (record->Family() == AF_INET)
        {
            sp->sa_family = AF_INET;
            sp->sa_port = htons(port);
            
            TUint8* from = record->UserPtr();
            
            TUint32 fromaddr = (from[0]<<24) + (from[1]<<16) + (from[2]<<8) + from[3];
            *(TUint32*)sp->sa_data = fromaddr;
            *len = 8;
            return;
        }
        else
        {
            sp->sa_family = (TUint16)record->Family();
            sp->sa_port = port;
        }
        
        TInt ulen = record->GetUserLen();
        
        if (ulen + 4 > (*len))
            ulen = (*len)-4;
        
        if (ulen < 0)
            return;

        *len = ulen + 4;
        memcpy(sp->sa_data, record->UserPtr(), ulen);
    }



struct hostent_buf* __hbp = NULL;

struct hostent* mapNameRecord(TNameRecord& aRecord, int length, int format)
    {
    free(__hbp);
    int nbytes=aRecord.iName.Length()+1;
    __hbp = (struct hostent_buf*)calloc(1,sizeof(struct hostent_buf)+nbytes);
    if (__hbp==NULL)
        {
        return 0;
        }
    __hbp->iHostent.h_name=&__hbp->iName[0];    
    __hbp->iHostent.h_aliases=&__hbp->iPtrs[1];
    __hbp->iHostent.h_addrtype=format;
    __hbp->iHostent.h_length=length;
    __hbp->iHostent.h_addr_list=&__hbp->iPtrs[0];
    __hbp->iPtrs[0]=(char*)&__hbp->iAddr.sa_data[0];
        
    TPtr8 name((TText8*)&__hbp->iName[0],nbytes);
    name.Copy(aRecord.iName);
    name.ZeroTerminate();
        
    unsigned long len=sizeof(__hbp->iAddr);
    aRecord.iAddr.SetFamily(format);
    Get( static_cast<TSockAddrDummy*>(&aRecord.iAddr), &__hbp->iAddr, &len );
    return &__hbp->iHostent;
    }

struct hostent* gethostbyname_iap (const char* name, int iap_id)
    {
    RSocketServ ss;
    TInt err=ss.Connect();
    struct hostent* retval=0;
    RHostResolver r;
    TCommDbConnPref pref;
    RConnection conn;
    
    // TCommDbConnPref stores the conection preference of the user.
    pref.SetIapId(iap_id);
    pref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt); // Set not to prompt the user
    
    // RConnection creates a connection.
   conn.Open(ss, KAfInet);
   conn.Start(pref);
        
    if (err==KErrNone)
        {
        err=r.Open(ss,KAfInet,KProtocolInetUdp,conn);
        if (!err)
            {
            TNameRecord record;
            TNameEntry entry(record);
            TPtrC8 ptr(reinterpret_cast<const TUint8*>(name));
            TBuf<0x40> hostname;
            hostname.Copy(ptr);
            err=r.GetByName(hostname,entry);
            if (err==KErrNone)
                {
                record=entry();
                retval=mapNameRecord(record, sizeof(struct in_addr), AF_INET);
                }
            r.Close();
            }       
        }
    conn.Close();
    ss.Close();
    return retval;
    }

