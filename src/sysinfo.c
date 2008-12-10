/* 
   Copyright (C) 2008 - Cfengine AS

   This file is part of Cfengine 3 - written and maintained by Cfengine AS.
 
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version. 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/

/*****************************************************************************/
/*                                                                           */
/* File: sysinfo.c                                                           */
/*                                                                           */
/* Created: Sun Sep 30 14:14:47 2007                                         */
/*                                                                           */
/*****************************************************************************/

#include "cf3.defs.h"
#include "cf3.extern.h"

#ifdef IRIX
#include <sys/syssgi.h>
#endif

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
# ifdef _SIZEOF_ADDR_IFREQ
#  define SIZEOF_IFREQ(x) _SIZEOF_ADDR_IFREQ(x)
# else
#  define SIZEOF_IFREQ(x) \
          ((x).ifr_addr.sa_len > sizeof(struct sockaddr) ? \
           (sizeof(struct ifreq) - sizeof(struct sockaddr) + \
            (x).ifr_addr.sa_len) : sizeof(struct ifreq))
# endif
#else
# define SIZEOF_IFREQ(x) sizeof(struct ifreq)
#endif

/*******************************************************************/

void GetNameInfo3()

{ int i,found = false;
  char *sp,*sp2,workbuf[CF_BUFSIZE];
  time_t tloc;
  struct hostent *hp;
  struct sockaddr_in cin;
#ifdef AIX
  char real_version[_SYS_NMLN];
#endif
#ifdef IRIX
  char real_version[256]; /* see <sys/syssgi.h> */
#endif
#ifdef HAVE_SYSINFO
  long sz;
#endif

Debug("GetNameInfo()\n");
  
VFQNAME[0] = VUQNAME[0] = '\0';
  
if (uname(&VSYSNAME) == -1)
   {
   perror("uname ");
   FatalError("Uname couldn't get kernel name info!!\n");
   }

#ifdef AIX
snprintf(real_version,_SYS_NMLN,"%.80s.%.80s", VSYSNAME.version, VSYSNAME.release);
strncpy(VSYSNAME.release, real_version, _SYS_NMLN);
#elif defined IRIX
/* This gets us something like `6.5.19m' rather than just `6.5'.  */ 
 syssgi (SGI_RELEASE_NAME, 256, real_version);
#endif 

for (sp = VSYSNAME.sysname; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

for (sp = VSYSNAME.machine; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

for (i = 0; CLASSATTRIBUTES[i][0] != '\0'; i++)
   {
   if (WildMatch(CLASSATTRIBUTES[i][0],ToLowerStr(VSYSNAME.sysname)))
      {
      if (WildMatch(CLASSATTRIBUTES[i][1],VSYSNAME.machine))
         {
         if (WildMatch(CLASSATTRIBUTES[i][2],VSYSNAME.release))
            {
            if (UNDERSCORE_CLASSES)
               {
               snprintf(workbuf,CF_BUFSIZE,"_%s",CLASSTEXT[i]);
               NewClass(workbuf);
               }
            else
               {
               NewClass(CLASSTEXT[i]);
               }
            found = true;

            VSYSTEMHARDCLASS = (enum classes) i;
            NewScalar("sys","class",CLASSTEXT[i],cf_str);

            break;
            }
         }
      else
         {
         Debug2("Cfengine: I recognize %s but not %s\n",VSYSNAME.sysname,VSYSNAME.machine);
         continue;
         }
      }
   }

FindDomainName(VSYSNAME.nodename);

if (!StrStr(VSYSNAME.nodename,VDOMAIN))
   {
   snprintf(VFQNAME,CF_BUFSIZE,"%s.%s",VSYSNAME.nodename,ToLowerStr(VDOMAIN));
   NewClass(CanonifyName(VFQNAME));
   strcpy(VUQNAME,VSYSNAME.nodename);
   NewClass(CanonifyName(VUQNAME));
   }
else
   {
   int n = 0;
   strcpy(VFQNAME,VSYSNAME.nodename);
   NewClass(CanonifyName(VFQNAME));
   
   while(VSYSNAME.nodename[n++] != '.')
      {
      }
   
   strncpy(VUQNAME,VSYSNAME.nodename,n-1);
   NewClass(CanonifyName(VUQNAME));
   }
  
if ((tloc = time((time_t *)NULL)) == -1)
   {
   printf("Couldn't read system clock\n");
   }

if (UNDERSCORE_CLASSES)
   {
   snprintf(workbuf,CF_BUFSIZE,"_%s",CLASSTEXT[i]);
   }
else
   {
   snprintf(workbuf,CF_BUFSIZE,"%s",CLASSTEXT[i]);
   }

CfOut(cf_verbose,"","Cfengine - %s\n%s\n\n",VERSION,CF3COPYRIGHT);
CfOut(cf_verbose,"","------------------------------------------------------------------------\n\n");
CfOut(cf_verbose,"","Host name is: %s\n",VSYSNAME.nodename);
CfOut(cf_verbose,"","Operating System Type is %s\n",VSYSNAME.sysname);
CfOut(cf_verbose,"","Operating System Release is %s\n",VSYSNAME.release);
CfOut(cf_verbose,"","Architecture = %s\n\n\n",VSYSNAME.machine);
CfOut(cf_verbose,"","Using internal soft-class %s for host %s\n\n",workbuf,VSYSNAME.nodename);
CfOut(cf_verbose,"","The time is now %s\n\n",ctime(&tloc));
CfOut(cf_verbose,"","------------------------------------------------------------------------\n\n");

snprintf(workbuf,CF_MAXVARSIZE,"%s",ctime(&tloc));
NewScalar("sys","date",workbuf,cf_str);
NewScalar("sys","cdate",CanonifyName(workbuf),cf_str);
NewScalar("sys","host",VSYSNAME.nodename,cf_str);
NewScalar("sys","uqhost",VUQNAME,cf_str);
NewScalar("sys","fqhost",VFQNAME,cf_str);
NewScalar("sys","os",VSYSNAME.sysname,cf_str);
NewScalar("sys","release",VSYSNAME.release,cf_str);
NewScalar("sys","arch",VSYSNAME.machine,cf_str);
NewScalar("sys","workdir",CFWORKDIR,cf_str);
NewScalar("sys","fstab",VFSTAB[VSYSTEMHARDCLASS],cf_str);
NewScalar("sys","resolv",VRESOLVCONF[VSYSTEMHARDCLASS],cf_str);
NewScalar("sys","maildir",VMAILDIR[VSYSTEMHARDCLASS],cf_str);

if (strlen(VDOMAIN) > 0)
   {
   NewScalar("sys","domain",VDOMAIN,cf_str);
   }
else
   {
   NewScalar("sys","domain","undefined_domain",cf_str);
   NewClass("undefined_domain");
   }

sprintf(workbuf,"%d_bit",sizeof(long)*8);
NewClass(workbuf);
Verbose("Additional hard class defined as: %s\n",CanonifyName(workbuf));

snprintf(workbuf,CF_BUFSIZE,"%s_%s",VSYSNAME.sysname,VSYSNAME.release);
NewClass(CanonifyName(workbuf));

#ifdef IRIX
/* Get something like `irix64_6_5_19m' defined as well as
   `irix64_6_5'.  Just copying the latter into VSYSNAME.release
   wouldn't be backwards-compatible.  */
snprintf(workbuf,CF_BUFSIZE,"%s_%s",VSYSNAME.sysname,real_version);
NewClass(CanonifyName(workbuf));
#endif

NewClass(CanonifyName(VSYSNAME.machine));
CfOut(cf_verbose,"","Additional hard class defined as: %s\n",CanonifyName(workbuf));

snprintf(workbuf,CF_BUFSIZE,"%s_%s",VSYSNAME.sysname,VSYSNAME.machine);
NewClass(CanonifyName(workbuf));
CfOut(cf_verbose,"","Additional hard class defined as: %s\n",CanonifyName(workbuf));

snprintf(workbuf,CF_BUFSIZE,"%s_%s_%s",VSYSNAME.sysname,VSYSNAME.machine,VSYSNAME.release);
NewClass(CanonifyName(workbuf));
CfOut(cf_verbose,"","Additional hard class defined as: %s\n",CanonifyName(workbuf));

#ifdef HAVE_SYSINFO
#ifdef SI_ARCHITECTURE
sz = sysinfo(SI_ARCHITECTURE,workbuf,CF_BUFSIZE);
if (sz == -1)
  {
  CfOut(cf_verbose,"","cfengine internal: sysinfo returned -1\n");
  }
else
  {
  NewClass(CanonifyName(workbuf));
  CfOut(cf_verbose,"","Additional hard class defined as: %s\n",workbuf);
  }
#endif
#ifdef SI_PLATFORM
sz = sysinfo(SI_PLATFORM,workbuf,CF_BUFSIZE);
if (sz == -1)
  {
  CfOut(cf_verbose,"","cfengine internal: sysinfo returned -1\n");
  }
else
  {
  NewClass(CanonifyName(workbuf));
  CfOut(cf_verbose,"","Additional hard class defined as: %s\n",workbuf);
  }
#endif
#endif

snprintf(workbuf,CF_BUFSIZE,"%s_%s_%s_%s",VSYSNAME.sysname,VSYSNAME.machine,VSYSNAME.release,VSYSNAME.version);

if (strlen(workbuf) > CF_MAXVARSIZE-2)
   {
   CfOut(cf_verbose,"","cfengine internal: $(arch) overflows CF_MAXVARSIZE! Truncating\n");
   }

sp = strdup(CanonifyName(workbuf));
NewScalar("sys","long_arch",sp,cf_str);
NewClass(sp);
free(sp);

snprintf(workbuf,CF_BUFSIZE,"%s_%s",VSYSNAME.sysname,VSYSNAME.machine);
sp = strdup(CanonifyName(workbuf));
NewScalar("sys","ostype",sp,cf_str);
NewClass(sp);

if (! found)
   {
   CfOut(cf_error,"","Cfengine: I don't understand what architecture this is!");
   }

strcpy(workbuf,"compiled_on_"); 
strcat(workbuf,CanonifyName(AUTOCONF_SYSNAME));
NewClass(CanonifyName(workbuf));
CfOut(cf_verbose,"","GNU autoconf class from compile time: %s",workbuf);

/* Get IP address from nameserver */

if ((hp = gethostbyname(VFQNAME)) == NULL)
   {
   Verbose("Hostname lookup failed on node name \"%s\"\n",VSYSNAME.nodename);
   return;
   }
else
   {
   memset(&cin,0,sizeof(cin));
   cin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   CfOut(cf_verbose,"","Address given by nameserver: %s\n",inet_ntoa(cin.sin_addr));
   strcpy(VIPADDRESS,inet_ntoa(cin.sin_addr));
   
   for (i=0; hp->h_aliases[i]!= NULL; i++)
      {
      Debug("Adding alias %s..\n",hp->h_aliases[i]);
      NewClass(CanonifyName(hp->h_aliases[i])); 
      }
   }
}

/*********************************************************************/

void GetInterfaceInfo3(void)

{ int fd,len,i,j,first_address;
  struct ifreq ifbuf[CF_IFREQ],ifr, *ifp;
  struct ifconf list;
  struct sockaddr_in *sin;
  struct hostent *hp;
  char *sp, workbuf[CF_BUFSIZE];
  char ip[CF_MAXVARSIZE];
  char name[CF_MAXVARSIZE];
  char last_name[CF_BUFSIZE];    

Debug("GetInterfaceInfo3()\n");

NewScalar("sys","interface",VIFDEV[VSYSTEMHARDCLASS],cf_str);

last_name[0] = '\0';

if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
   {
   CfOut(cf_error,"socket","Couldn't open socket");
   exit(1);
   }

list.ifc_len = sizeof(ifbuf);
list.ifc_req = ifbuf;

#ifdef SIOCGIFCONF
if (ioctl(fd, SIOCGIFCONF, &list) == -1 || (list.ifc_len < (sizeof(struct ifreq))))
#else
if (ioctl(fd, OSIOCGIFCONF, &list) == -1 || (list.ifc_len < (sizeof(struct ifreq))))
#endif
   {
   CfOut(cf_error,"ioctl","Couldn't get interfaces - old kernel? Try setting CF_IFREQ to 1024");
   exit(1);
   }

last_name[0] = '\0';

for (j = 0,len = 0,ifp = list.ifc_req; len < list.ifc_len; len+=SIZEOF_IFREQ(*ifp),j++,ifp=(struct ifreq *)((char *)ifp+SIZEOF_IFREQ(*ifp)))
   {
   if (ifp->ifr_addr.sa_family == 0)
      {
      continue;
      }

   if (strlen(ifp->ifr_name) == 0)
      {
      continue;
      }
   
   CfOut(cf_verbose,"","Interface %d: %s\n",j+1,ifp->ifr_name);
   
   /* Chun Tian (binghe) <binghe.lisp@gmail.com>:
      use a last_name to detect whether current address is a interface's first address:
      if current ifr_name = last_name, it's not the first address of current interface. */

   if (strncmp(last_name,ifp->ifr_name,sizeof(ifp->ifr_name)) == 0)
      {
      first_address = false;
      }
   else
      {
      first_address = true;
      }
   
   strncpy(last_name,ifp->ifr_name,sizeof(ifp->ifr_name));

   if (UNDERSCORE_CLASSES)
      {
      snprintf(workbuf, CF_BUFSIZE, "_net_iface_%s", CanonifyName(ifp->ifr_name));
      }
   else
      {
      snprintf(workbuf, CF_BUFSIZE, "net_iface_%s", CanonifyName(ifp->ifr_name));
      }

   NewClass(workbuf);
   
   if (ifp->ifr_addr.sa_family == AF_INET)
      {
      strncpy(ifr.ifr_name,ifp->ifr_name,sizeof(ifp->ifr_name));
      
      if (ioctl(fd,SIOCGIFFLAGS,&ifr) == -1)
         {
         CfOut(cf_error,"ioctl","No such network device");
         close(fd);
         return;
         }

      if ((ifr.ifr_flags & IFF_BROADCAST) && !(ifr.ifr_flags & IFF_LOOPBACK))
         {
         sin=(struct sockaddr_in *)&ifp->ifr_addr;
   
         if ((hp = gethostbyaddr((char *)&(sin->sin_addr.s_addr),sizeof(sin->sin_addr.s_addr),AF_INET)) == NULL)
            {
            Debug("No hostinformation for %s not found\n", inet_ntoa(sin->sin_addr));
            }
         else
            {
            if (hp->h_name != NULL)
               {
               Debug("Adding hostip %s..\n",inet_ntoa(sin->sin_addr));
               NewClass(CanonifyName(inet_ntoa(sin->sin_addr)));
               Debug("Adding hostname %s..\n",hp->h_name);
               NewClass(CanonifyName(hp->h_name));

               if (hp->h_aliases != NULL)
                  {
                  for (i=0; hp->h_aliases[i] != NULL; i++)
                     {
                     Verbose("Adding alias %s..\n",hp->h_aliases[i]);
                     NewClass(CanonifyName(hp->h_aliases[i]));
                     }
                  }
               }
            }
         
         /* Old style compat */
         strcpy(ip,inet_ntoa(sin->sin_addr));
         AppendItem(&IPADDRESSES,ip,"");
         
         for (sp = ip+strlen(ip)-1; *sp != '.'; sp--)
            {
            }
         *sp = '\0';
         NewClass(CanonifyName(ip));
            
         /* New style classes */
         strcpy(ip,"ipv4_");
         strcat(ip,inet_ntoa(sin->sin_addr));
         NewClass(CanonifyName(ip));
         NewScalar("sys","ipv4",inet_ntoa(sin->sin_addr),cf_str);

         for (sp = ip+strlen(ip)-1; (sp > ip); sp--)
            {
            if (*sp == '.')
               {
               *sp = '\0';
               NewClass(CanonifyName(ip));
               }
            }

         /* Matching variables */

         if (first_address)
            {
            strcpy(ip,inet_ntoa(sin->sin_addr));
            snprintf(name,CF_MAXVARSIZE-1,"ipv4[%s]",CanonifyName(ifp->ifr_name));
            NewScalar("sys",name,ip,cf_str);
            
            i = 3;
         
            for (sp = ip+strlen(ip)-1; (sp > ip); sp--)
               {
               if (*sp == '.')
                  {
                  *sp = '\0';
                  snprintf(name,CF_MAXVARSIZE-1,"ipv4_%d[%s]",i--,CanonifyName(ifp->ifr_name));
                  NewScalar("sys",name,ip,cf_str);
                  }
               }
            }
         }
      }
   }
 
close(fd);
}

/*******************************************************************/

void Get3Environment()

{ char env[CF_BUFSIZE],class[CF_BUFSIZE],name[CF_MAXVARSIZE],value[CF_MAXVARSIZE];
  FILE *fp;
  struct stat statbuf;
  time_t now = time(NULL);
  
CfOut(cf_verbose,"","Looking for environment from cfenvd...\n");
snprintf(env,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_ENV_FILE);

if (stat(env,&statbuf) == -1)
   {
   CfOut(cf_verbose,"","Unable to detect environment from cfMonitord\n\n");
   return;
   }

if (statbuf.st_mtime < (now - 60*60))
   {
   CfOut(cf_verbose,"","Environment data are too old - discarding\n");
   unlink(env);
   return;
   }

snprintf(value,CF_MAXVARSIZE-1,"%s",ctime(&statbuf.st_mtime));
Chop(value);

DeleteVariable("sys","env_time");
NewScalar("sys","env_time",value,cf_str);

CfOut(cf_verbose,"","Loading environment...\n");
 
if ((fp = fopen(env,"r")) == NULL)
   {
   CfOut(cf_verbose,"","\nUnable to detect environment from cfenvd\n\n");
   return;
   }

while (!feof(fp))
   {
   class[0] = '\0';
   name[0] = '\0';
   value[0] = '\0';

   fgets(class,CF_BUFSIZE-1,fp);

   if (feof(fp))
      {
      break;
      }

   if (strstr(class,"="))
      {
      sscanf(class,"%255[^=]=%255[^\n]",name,value);

      DeleteVariable("sys",name);
      NewScalar("sys",name,value,cf_str);
      }
   else
      {
      NewClass(class);
      }
   }
 
fclose(fp);
CfOut(cf_verbose,"","Environment data loaded\n\n"); 
}

/*********************************************************************/

void FindV6InterfaceInfo(void)

{ FILE *pp;
  char buffer[CF_BUFSIZE]; 
 
/* Whatever the manuals might say, you cannot get IPV6
   interface configuration from the ioctls. This seems
   to be implemented in a non standard way across OSes
   BSDi has done getifaddrs(), solaris 8 has a new ioctl, Stevens
   book shows the suggestion which has not been implemented...
*/
 
 Verbose("Trying to locate my IPv6 address\n");

 switch (VSYSTEMHARDCLASS)
    {
    case cfnt:
        /* NT cannot do this */
        return;

    case irix:
    case irix4:
    case irix64:
        
        if ((pp = cf_popen("/usr/etc/ifconfig -a","r")) == NULL)
           {
           Verbose("Could not find interface info\n");
           return;
           }
        
        break;

    case hp:
        
        if ((pp = cf_popen("/usr/sbin/ifconfig -a","r")) == NULL)
           {
           Verbose("Could not find interface info\n");
           return;
           }

        break;

    case aix:
        
        if ((pp = cf_popen("/etc/ifconfig -a","r")) == NULL)
           {
           Verbose("Could not find interface info\n");
           return;
           }

        break;
        
    default:
        
        if ((pp = cf_popen("/sbin/ifconfig -a","r")) == NULL)
           {
           Verbose("Could not find interface info\n");
           return;
           }

    }

/* Don't know the output format of ifconfig on all these .. hope for the best*/
 
while (!feof(pp))
   {    
   fgets(buffer,CF_BUFSIZE-1,pp);

   if (ferror(pp))  /* abortable */
      {
      break;
      }
   
   if (StrStr(buffer,"inet6"))
      {
      struct Item *ip,*list = NULL;
      char *sp;
      
      list = SplitStringAsItemList(buffer,' ');
      
      for (ip = list; ip != NULL; ip=ip->next)
         {
         for (sp = ip->name; *sp != '\0'; sp++)
            {
            if (*sp == '/')  /* Remove CIDR mask */
               {
               *sp = '\0';
               }
            }

         if (IsIPV6Address(ip->name) && (strcmp(ip->name,"::1") != 0))
            {
            Verbose("Found IPv6 address %s\n",ip->name);
            AppendItem(&IPADDRESSES,ip->name,"");
            NewClass(CanonifyName(ip->name));
            }
         }
      
      DeleteItemList(list);
      }
   }

cf_pclose(pp);
}


/*********************************************************************/

void FindDomainName(char *hostname)

{ char fqn[CF_MAXVARSIZE];
  char *ptr;
  char buffer[CF_BUFSIZE];
 
strcpy(VFQNAME,hostname); /* By default VFQNAME = hostname (nodename) */

if (strstr(VFQNAME,".") == 0)
   {
   /* The nodename is not full qualified - try to find the FQDN hostname */

   if (gethostname(fqn, sizeof(fqn)) != -1)
      {
      struct hostent *hp;

      if (hp = gethostbyname(fqn))
         {
         if (strstr(hp->h_name,"."))
            {
            /* We find a FQDN hostname So we change the VFQNAME variable */
            strncpy(VFQNAME,hp->h_name,CF_MAXVARSIZE);
            VFQNAME[CF_MAXVARSIZE-1]= '\0'; 
            }
         }
      }
   }

strcpy(buffer,VFQNAME);
NewClass(CanonifyName(buffer));
NewClass(CanonifyName(ToLowerStr(buffer)));

if (strstr(VFQNAME,"."))
   {
   /* If VFQNAME is full qualified we can create VDOMAIN variable */
   ptr = strchr(VFQNAME, '.');
   strcpy(VDOMAIN, ++ptr);
   DeleteClass("undefined_domain");
   }

if (strstr(VFQNAME,".") == 0 && (strcmp(VDOMAIN,CF_START_DOMAIN) != 0))
   {
   strcat(VFQNAME,".");
   strcat(VFQNAME,VDOMAIN);
   }

if (strstr(VFQNAME,"."))
   {
   /* Add some domain hierarchy classes */
   for (ptr=VFQNAME; *ptr != '\0'; ptr++)
      {
      if (*ptr == '.')
         {
         if (*(ptr+1) != '\0')
            {
            Debug("Defining domain #%s#\n",(ptr+1));
            NewClass(CanonifyName(ptr+1));
            }
         else
            {
            Debug("Domain rejected\n");
            }      
         }
      }
   }

NewClass(CanonifyName(VDOMAIN));
}
