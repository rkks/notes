#!/bin/ksh
##set -xv
##
## nic_check - http://sysunconfig.net/
## v1.0 - 01/12/2001 - David Cashion - Initial script
## v1.1 - 11/06/2002 - David Cashion - Added ability to look at all interfaces or just the one specified
## v1.2 - 09/30/2004 - Scott Kampen  - Updated to work with ce, ge, bge, dmfe
##                                     and added autoneg link partner check
## v1.3 - 06/12/2006 - David Cashion - Updated to work with ipge
## v1.4 - 07/05/2006 - Matt Cheek    - Ignore Sun Cluster private interface clprivnet
## v1.5 - 10/24/2007 - Thomas Roos   - Adding ngxe interfaces and ignore sppp from M9000
## v1.6 - 10/25/2007 - James Council - Ignore be, lpfc, jnet interfaces as well
##                     Octave Orgeron
## v2.0 - 10/26/2007 - Nick Senedzuk - Support more than 10 interfaces, ignore dman as well
## v2.1 - 10/29/2007 - David Cashion - Now supports e1000g 
## v2.2 - 11/01/2007 - David Barr    - Added iprb
## v2.3 - 11/01/2007 - David Barr    - Updated iprb checks
## v2.4 - 11/20/2007 - Matt Cheek    - Ignore aggr interfaces for now, need to figure out what
##				       interfaces are underneath to support this 
##
## Polls the interfaces specified for speed, link, duplex, autonegotiation of link partner,
## and what the LP is telling us the highest speed/duplex setting is.
## If the interface is down, all bets are off.

export PATH=/usr/sbin:/usr/bin:$PATH

WHOISTHIS=$(id | cut -c1-5)
if [ $WHOISTHIS != "uid=0" ]; then 
        print "Must be root to run $0." 
        exit 1
fi

#### Pick the interface given or run through all of them (except lo, ....the list keeps growing)
if [ $# -eq 1 ]; then
   if [ $1 == "-all" ]; then
        LISTOFNICS=$(ifconfig -a | awk -F: '/^[^l\t][^o]/ {print $1}' | sort -u)
   else
        LISTOFNICS="$@"
   fi
else
   print "USAGE:"
   print "\t$0 INTERFACEinstance"
   print "\t$0 -all"
   print "\n\tExample: $0 hme0"
   exit 1
fi         


print "Interface\tLink\tSpeed\tDuplex\tLink Partner Autoneg\tLP Setting"
print "_________\t____\t_____\t______\t____________________\t__________"

for NIC in $LISTOFNICS
   do
   ## Default values of...I dunno
   LPSETTING='N/A'; STATUS='N/A'; SPEED='N/A'; DUPLEX='N/A'; AUTONEG='N/A       '; 

   ## We have lots of "special" cases for each new interface, ick
   case $NIC in
      e1000g*)
      	  ## What were they thinking...more special cases, the whole reason this script exists.
	  INTERFACE=e1000g
	  INT_INST=$(print ${NIC#??????})
	;;
      *)
	  INTERFACE=$(print $NIC | tr -d '[0-9]')
	  INT_INST=$(print $NIC | tr -d '[a-z]')
	;;
   esac

   case $NIC in
      clprivnet*|dman*|sppp*|be*|lpfc*|jnet*|aggr*)
	  ## Ignore all these guys instead of trying to grep -v them all out
	  continue
      	;;

      nxge*)
	#### Sun 10/1 Gigabit Ethernet - Sun Quad GbE UTP x8 PCIe
	if [ $(kstat -p $INTERFACE:$INT_INST:mac:link_up | awk '{print $2}') -eq 0 ]; then
	   STATUS=DOWN; SPEED='N/A'; DUPLEX='N/A'; AUTONEG='N/A  '; LPSETTING='N/A'
	else
	   STATUS=UP
	   DUPLEX=$(kstat -p $INTERFACE:$INT_INST:mac:link_duplex | awk '{print $2}')
	   case $DUPLEX in
		1) DUPLEX=HALF ;;
		2) DUPLEX=FULL ;;
	   esac
	   SPEED=$(kstat -p "$INTERFACE:$INT_INST:Port Stats:link_speed" | awk '{print $3}')
	   [ $(kstat -p $INTERFACE:$INT_INST:mac:lp_cap_autoneg | awk '{print $2}') -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED "
	   [ $(kstat -p $INTERFACE:$INT_INST:mac:lp_cap_10hdx | awk '{print $2}') -eq 1 ] && LPSETTING=10_HALF
	   [ $(kstat -p $INTERFACE:$INT_INST:mac:lp_cap_10fdx | awk '{print $2}') -eq 1 ] && LPSETTING=10_FULL
	   [ $(kstat -p $INTERFACE:$INT_INST:mac:lp_cap_100hdx | awk '{print $2}') -eq 1 ] && LPSETTING=100_HALF
	   [ $(kstat -p $INTERFACE:$INT_INST:mac:lp_cap_100fdx | awk '{print $2}') -eq 1 ] && LPSETTING=100_FULL
	   [ $(kstat -p $INTERFACE:$INT_INST:mac:lp_cap_1000hdx | awk '{print $2}') -eq 1 ] && LPSETTING=1000_HALF
	   [ $(kstat -p $INTERFACE:$INT_INST:mac:lp_cap_1000fdx | awk '{print $2}') -eq 1 ] && LPSETTING=1000_FULL
	fi
        ;;

      iprb*)
        #### Intel 82557, 82558, 82559
	STATUS=$(kstat -m iprb -i $INT_INST -s class | grep class | awk '{print $4}')
	case $STATUS in
	   net) STATUS=UP ;;
	   *) STATUS=DOWN ;;
	esac
	SPEED=$(kstat -m iprb -i $INT_INST -s ifspeed | grep speed | awk '{print $2}')
	case $SPEED in
	   1000000000) SPEED=1000 ;;
	   100000000) SPEED=100 ;;
	   10000000) SPEED=10 ;;
	esac
	typeset -u DUPLEX
	DUPLEX=$(kstat -m iprb -i $INT_INST -s duplex | grep duplex | awk '{print $2}')
        ;;

      e1000g*)
        #### Intel PRO/1000 Gigabit - UltraSPARC T1 servers - T1000, T2000, maybe more?
	set $(dladm show-dev $NIC)
	## e1000g0         lnk: up        speed: 100   Mbps       duplex: full
	typeset -u STATUS
	STATUS=$3
	SPEED=$5
	typeset -u DUPLEX
	DUPLEX=$8
	[ $(kstat -m $INTERFACE -i $INT_INST -s cap_autoneg | grep cap_autoneg | awk '{print $2}') -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED "
	[ $(ndd -get /dev/$NIC adv_10hdx_cap) -eq 1 ] && LPSETTING=10_HALF
	[ $(ndd -get /dev/$NIC adv_10fdx_cap) -eq 1 ] && LPSETTING=10_FULL
	[ $(ndd -get /dev/$NIC adv_100hdx_cap) -eq 1 ] && LPSETTING=100_HALF
	[ $(ndd -get /dev/$NIC adv_100fdx_cap) -eq 1 ] && LPSETTING=100_FULL
	[ $(ndd -get /dev/$NIC adv_1000hdx_cap) -eq 1 ] && LPSETTING=1000_HALF
	## 1000_FULL is not supported
        ;;

      ce*)
        #### Cassini Gigabit-Ethernet - GigaSwift Ethernet
        if [ $(kstat -p $INTERFACE:$INT_INST:$NIC:link_up | awk '{print $2}') -eq 0 ]; then
           STATUS=DOWN; SPEED='N/A'; DUPLEX='N/A'; AUTONEG='N/A       '; LPSETTING='N/A'
        else 
           STATUS=UP
           DUPLEX=$(kstat -p $INTERFACE:$INT_INST:$NIC:link_duplex | awk '{print $2}')
           case $DUPLEX in
                1) DUPLEX=HALF ;;
                2) DUPLEX=FULL ;;
                *) DUPLEX=DOWN ;;
           esac
           SPEED=$(kstat -p $INTERFACE:$INT_INST:$NIC:link_speed | awk '{print $2}')
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_autoneg | awk '{print $2}') -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED "
        
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_10hdx | awk '{print $2}') -eq 1 ] && LPSETTING=10_HALF
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_10fdx | awk '{print $2}') -eq 1 ] && LPSETTING=10_FULL
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_100hdx | awk '{print $2}') -eq 1 ] && LPSETTING=100_HALF
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_100fdx | awk '{print $2}') -eq 1 ] && LPSETTING=100_FULL
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_1000hdx | awk '{print $2}') -eq 1 ] && LPSETTING=1000_HALF
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_1000fdx | awk '{print $2}') -eq 1 ] && LPSETTING=1000_FULL
        fi
        ;;
      
      ge*)
        #### GEM Gigabit-Ethernet - v880, fiber
        ndd -set /dev/$INTERFACE instance $INT_INST
        if [ $? -ne 0 ]; then
           print "ERROR: Problems setting instance number for $NIC"
           break
        fi
        
        if [ $(ndd /dev/$INTERFACE link_status) -eq 0 ]; then
           STATUS=DOWN; SPEED='N/A'; DUPLEX='N/A'; AUTONEG='N/A       '; LPSETTING='N/A'
        else
           STATUS=UP
           [ $(ndd /dev/$INTERFACE link_mode) -eq 0 ] && DUPLEX=HALF || DUPLEX=FULL
           SPEED=$(ndd /dev/$INTERFACE link_speed | sed q)
           [ $(ndd /dev/$INTERFACE lp_1000autoneg_cap) -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED "

           [ $(ndd /dev/$INTERFACE lp_1000hdx_cap) -eq 1 ] && LPSETTING=1000_HALF
           [ $(ndd /dev/$INTERFACE lp_1000fdx_cap) -eq 1 ] && LPSETTING=1000_FULL
        fi
        ;;
        
      bge*) 
        #### Gigabit Ethernet driver for Broadcom BCM57xx - v210, v240, and M?000
        if [ $(ndd /dev/$NIC link_status) -eq 0 ]; then 
           STATUS=DOWN; SPEED='N/A'; DUPLEX='N/A'; AUTONEG='N/A       '; LPSETTING='N/A'
        else
           STATUS=UP
           [ $(ndd /dev/$NIC link_duplex) -eq 0 ] && DUPLEX=HALF || DUPLEX=FULL
           SPEED=$(ndd /dev/$NIC link_speed)
           [ $(ndd /dev/$NIC lp_autoneg_cap) -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED "

           [ $(ndd /dev/$NIC lp_10hdx_cap) -eq 1 ] && LPSETTING=10_HALF
           [ $(ndd /dev/$NIC lp_10fdx_cap) -eq 1 ] && LPSETTING=10_FULL
           [ $(ndd /dev/$NIC lp_100hdx_cap) -eq 1 ] && LPSETTING=100_HALF
           [ $(ndd /dev/$NIC lp_100fdx_cap) -eq 1 ] && LPSETTING=100_FULL
           [ $(ndd /dev/$NIC lp_1000hdx_cap) -eq 1 ] && LPSETTING=1000_HALF
           [ $(ndd /dev/$NIC lp_1000fdx_cap) -eq 1 ] && LPSETTING=1000_FULL
        fi
        ;;
        
      dmfe*)
        #### Davicom Fast Ethernet driver for Davicom DM9102A - v100
        if [ $(ndd /dev/$NIC link_status) -eq 0 ]; then
           STATUS=DOWN; SPEED='N/A'; DUPLEX='N/A'; AUTONEG='N/A       '; LPSETTING='N/A'
        else 
           STATUS=UP
           [ $(ndd /dev/$NIC link_mode) -eq 0 ] && DUPLEX=HALF || DUPLEX=FULL
           SPEED=$(ndd /dev/$NIC link_speed)
           [ $(ndd /dev/$NIC lp_autoneg_cap) -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED "
        
           [ $(ndd /dev/$NIC lp_10hdx_cap) -eq 1 ] && LPSETTING=10_HALF
           [ $(ndd /dev/$NIC lp_10fdx_cap) -eq 1 ] && LPSETTING=10_FULL
           [ $(ndd /dev/$NIC lp_100hdx_cap) -eq 1 ] && LPSETTING=100_HALF
           [ $(ndd /dev/$NIC lp_100fdx_cap) -eq 1 ] && LPSETTING=100_FULL
        fi
        ;;

      ipge*)
        #### PCI-E Gigabit-Ethernet device driver for Intel 82571 - T2000
        if [ $(kstat -p $INTERFACE:$INT_INST:$NIC:link_up | awk '{print $2}') -eq 0 ]; then
           STATUS=DOWN; SPEED='N/A'; DUPLEX='N/A'; AUTONEG='N/A       '; LPSETTING='N/A'
        else 
           STATUS=UP
           DUPLEX=$(kstat -p $INTERFACE:$INT_INST:$NIC:link_duplex | awk '{print $2}')
           case $DUPLEX in
                1) DUPLEX=HALF ;;
                2) DUPLEX=FULL ;;
                *) DUPLEX=DOWN ;;
           esac
           SPEED=$(kstat -p $INTERFACE:$INT_INST:$NIC:link_speed | awk '{print $2}')
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_autoneg | awk '{print $2}') -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED "
        
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_10hdx | awk '{print $2}') -eq 1 ] && LPSETTING=10_HALF
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_10fdx | awk '{print $2}') -eq 1 ] && LPSETTING=10_FULL
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_100hdx | awk '{print $2}') -eq 1 ] && LPSETTING=100_HALF
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_100fdx | awk '{print $2}') -eq 1 ] && LPSETTING=100_FULL
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_1000hdx | awk '{print $2}') -eq 1 ] && LPSETTING=1000_HALF
           [ $(kstat -p $INTERFACE:$INT_INST:$NIC:lp_cap_1000fdx | awk '{print $2}') -eq 1 ] && LPSETTING=1000_FULL
        fi
        ;;
        
      *)
        ## Should catch everything else like hme, qfe, eri that are "normal".
        ndd -set /dev/$INTERFACE instance $INT_INST
        if [ $? -ne 0 ]; then
           print "ERROR: Problems setting instance number for $NIC"
           break
        fi
        
        if [ $(ndd /dev/$INTERFACE link_status) -eq 0 ]; then
           STATUS=DOWN; SPEED='N/A'; DUPLEX='N/A'; AUTONEG='N/A       '; LPSETTING='N/A'
        else
           STATUS=UP
           [ $(ndd /dev/$INTERFACE link_mode) -eq 0 ] && DUPLEX=HALF || DUPLEX=FULL
           [ $(ndd /dev/$INTERFACE link_speed) -eq 0 ] && SPEED=10 || SPEED=100
        
           #### Determine if the link partner (switch / router) has autonegotiation capability enabled or not
           case $NIC in
              hme*) [ $(ndd /dev/hme lp_autoneg_cap) -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED " ;;
              qfe*) [ $(ndd /dev/qfe lp_autoneg_cap) -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED " ;;
              eri*) [ $(ndd /dev/eri lp_autoneg_cap) -eq 0 ] && AUTONEG=DISABLED || AUTONEG="ENABLED " ;;
              *) AUTONEG="UNKNOWN " ;;
           esac
        
           [ $(ndd /dev/$INTERFACE lp_10hdx_cap) -eq 1 ] && LPSETTING=10_HALF
           [ $(ndd /dev/$INTERFACE lp_10fdx_cap) -eq 1 ] && LPSETTING=10_FULL
           [ $(ndd /dev/$INTERFACE lp_100hdx_cap) -eq 1 ] && LPSETTING=100_HALF
           [ $(ndd /dev/$INTERFACE lp_100fdx_cap) -eq 1 ] && LPSETTING=100_FULL
        fi
        ;;
   esac
   print "$NIC\t\t$STATUS\t$SPEED\t$DUPLEX\t$AUTONEG\t\t$LPSETTING"
done
