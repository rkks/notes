How to Compile 2.6 kernel for RedHat 9 and 8.0 and get Fedora Updates
Mike Chirico (mchirico@users.sourceforge.net or mchirico@comcast.net)
Copyright (c) 2004 (GPU Free Documentation License) 
Last Updated: Mon Sep 13 08:58:06 EDT 2004


The latest version of this document can be found at:
http://prdownloads.sourceforge.net/souptonuts/README_26.txt?download

For configs ref:
http://sourceforge.net/project/showfiles.php?group_id=79320&package_id=109944



STEP 1:

    Download the latest version of the kernel and any patches.
    This documentation is done with linux-2.6.3, but look for
    later versions.
         http://www.kernel.org/pub/linux/kernel/v2.6/

    And make sure you have the correct pgp key. From the shell
    prompt run the following to get the key:
         $ gpg --keyserver wwwkeys.pgp.net --recv-keys 0x517D0F0E

    Then download "linux-2.6.x.tar.gz.sign" for the current version
    and run

         $ gpg --verify linux-2.6.7.tar.gz.sign linux-2.6.7.tar.gz

    You just need to get authentication on the top first two lines.
    For more information reference the following:
         http://www.kernel.org/signature.html
         

    Also take a look at the following:
       http://www.codemonkey.org.uk/post-halloween-2.5.txt 
       http://www.kernel.org/pub/linux/kernel/README

    This has some useful hints on some of the changes needed.



STEP 2:

    Download the latest version of module-init-tools
    "module-init-tools-3.0.tar.gz"  and
    "modutils-2.4.21-23.src.rpm"

      http://www.kernel.org/pub/linux/kernel/people/rusty/modules/module-init-tools-3.0.tar.gz
      http://www.kernel.org/pub/linux/kernel/people/rusty/modules/modutils-2.4.21-23.src.rpm



STEP 3:

    Install module-init-tools.  This will replace depmod
    [/sbin/depmod] and other tools.

        tar -zxvf module-init-tools-3.0.tar.gz
        cd module-init-tools-3.0
        ./configure --prefix=/
        make
        make install 
        ./generate-modprobe.conf /etc/modprobe.conf



STEP 4:

    Install modutils-2.4.21-23.src.rpm.  You may get warnings
    about user rusty and group rusty not existing.  Also, yes,
    you'll have to force the install. If you don't do these steps
    for both Redhat 9 and Redhat 8, you'll have problems with the
    make modules_install.

        rpm -i modutils-2.4.21-23.src.rpm
        rpmbuild -bb /usr/src/redhat/SPECS/modutils.spec
        rpm -Fi /usr/src/redhat/RPMS/i386/modutils-2.4.21-23.i386.rpm



STEP 5:

    Install and configure the kernel.  Do NOT use the /usr/src/linux
    area!  Reference the README. I put my files in /home/src/kernel/

        gunzip linux-2.6.3.tar.gz tar -xvf linux-2.6.3.tar cd
        linux-2.6.3

    If you have patches install these now:

        bzip2 -dc ../patch-2.6.xx.bz2 | patch -p1



STEP 6:

    Copy the appropriate /usr/src/linux-2.4/configs
    [kernel-2.4.20-i686.config, kernel-2.4.20-i686-smp.config]
    to .config in whatever directory you are installing.  In my
    case it's /home/src/kernel/linux-2.6.3

       cp /usr/src/linux-2.4/configs/kernel-2.4.20-i686.config  \
                  /home/src/kernel/linux-2.6.3/.config

    If you don't have the source configs, you can download them
    from here:

https://sourceforge.net/project/showfiles.php?group_id=79320&package_id=109944

    I've also included a file config2.6-chirico which was a 2.6
    version for some of my systems. This isn't a bad reference if
    you run into trouble.



STEP 7:

    Assuming you copied the appropriate kernel-2.4 config to
    .config, run the following which will run through necessary
    questions for the 2.6 kernel. Or, you might want to use the
    config2.6-chirico...this has already been run through make
    oldconfig on my system, and I've answered the necessary questions
    for a general system.

               make oldconfig

    See SPECIAL NOTES below if you've done this step multiple
    times, plus already done a "make bzImage" and "make 
    modules" and need to old compiles.

    If you cannot get the config you want, instead,
    do a "make config" -- this will prompt you for all
    the necessary questions.

    Or, there is the  terminal menu "make menuconfig" interface where 
    you can specifically select options.



STEP 8:

    This is very important.  Make sure you're .config has the
    following in it CONFIG_EXT3_FS=y  You'll run into the following
    error if you leave this =m instead of =y:

          pivotroot: pivot_root(/sysroot,/sysroot/initrd) failed

    This is because Redhat 9.0 and 8.0 use the ext3 filesystem
    for /boot ...



STEP 9:

    Edit the Makefile and add changes to the Extraversion as desired.
    Patches will update these values as well.

    VERSION = 2 
    PATCHLEVEL = 6 
    SUBLEVEL = 3 
    EXTRAVERSION = -skim-ch6



STEP 10:

    make bzImage



STEP 11:

    make modules



STEP 12:

    make modules_install



STEP 13:

    make install

    If you come across errors here, what version of "depmod" is
    being picked up in your path?

    Also, if you get a module not found, say the following:
             No module aic7xxx found for kernel 2.6.x
    Then, in /lib/modules/2.6.x/kernel/drivers/scsi/aic7xxx/
           cp aic7xxx.ko aic7xxx.o

    insmod should look for aic7xxx.ko ;but , it looks for aic7xxx.o

    If you still have trouble, make the following change in the
    .config
           CONFIG_BLK_DEV_SD=y
    and go back to STEP 10.

    You also may want to ref
         kernel-2.6.3-i686-smp-chirico-aic7xxx.config
                in
http://prdownloads.sourceforge.net/souptonuts/configs-0.3.tar.gz?download



STEP 14:

     mkdir /sys



STEP 15:

     /etc/rc.sysinit needs to be modified.  Look for the following
     line:

  action $"Mounting proc filesystem: " mount -n -t proc /proc  /proc

    and after this line enter the following:

  action $"Mounting sysfs filesystem: " mount  -t sysfs none /sys

    Here's my  /etc/rc.sysinit for reference:

  http://prdownloads.sourceforge.net/souptonuts/rc.sysinit.txt?download


    Be very careful at this step. Backup the /etc/rc.sysinit file.

    Also make changes to "/etc/fstab" adding the following line:

         none  /sys sysfs  defaults 0 0

    A copy of my "/etc/fstab" can be found at the following location
     
      http://prdownloads.sourceforge.net/souptonuts/fstab.txt?download


     You may also want to reference Thomer
           [http://thomer.com/linux/migrate-to-2.6.html ] 



STEP 16:

    Add the following to /etc/fstab for usb support.

    /proc/bus/usb           /proc/bus/usb           usbdevfs defaults       0 0



STEP 17 (CHECKING EVERYTHING):

    Check the following:

    a. The new image file should be installed on boot and there
    should be sym link to it.  My latest kernel is 2.6.3-skim-ch6,
    and I got the "-skim-ch6" from the values I put in the Makefile,
    so I see the following:

           /boot
               vmlinuz -> vmlinuz-2.6.3-skim-ch6
                System.map -> System.map-2.6.3-skim-ch6

           /boot/grub/grub.conf   Should have been automatically
           updated from make.

    In /boot/grub/grub.conf change "default=0" to boot
    with the new kernel.  Here's an example of my
    grub.conf:


  # grub.conf generated by anaconda
  #
  # Note that you do not have to rerun grub after making 
  # NOTICE:  You have a /boot partition.  This means that
  #          all kernel and initrd paths are relative to 
  #          root (hd0,2)
  #          kernel /vmlinuz-version ro root=/dev/hda6
  #          initrd /initrd-version.img
  #boot=/dev/hda
  default=0
  timeout=10
  splashimage=(hd0,2)/grub/splash.xpm.gz
  title Red Hat Linux (2.6.3-skim-ch6)
      root (hd0,2)
      kernel /vmlinuz-2.6.3-skim-ch6 ro root=LABEL=/
      initrd /initrd-2.6.3-skim-ch6.img


    b. The directory /sys exists. Also check you /etc/fstab
       against mine, noting the "sys entry".
         http://prdownloads.sourceforge.net/souptonuts/fstab.txt?download

    c. You added the mount command for sys in /etc/rc.sysinit

    d. CONFIG_EXT3_FS=y  was used in the .config

    e. Run /sbin/lsmod or cat /proc/modules to make
       sure a 2.4 kernel module wasn't forgotten. Also
       look at "$cat /proc/iomem"

       

STEP 18 (GETTING FEDORIA UPDATES: YUM):

    Yum works with RPM based systems to update packages
    automatically.  Yum is an officially supported update
    mechanism for Fedora, and Fedora mirrors are set up 
    as Yum repositories.

    Download:

     http://linux.duke.edu/projects/yum/download.ptml

    Before installing Yum, or any RPM package, you'll need
    to do the following:

      export LD_ASSUME_KERNEL=2.4.19

    Next, install Yum

      rpm -ivh yum-2.0.7-1.noarch.rpm

    The "/etc/yum.conf" may need to be updated.  Here is an
    example file with the values for redhat 9 entered in
    directly

        
        [main]                                                                  
        cachedir=/var/cache/yum                                                 
        debuglevel=2                                                            
        logfile=/var/log/yum.log                                                
        pkgpolicy=newest                                                        
        distroverpkg=redhat-release                                             
        tolerant=1                                                              
        exactarch=1                                                             
                                                                                
        [fedora-stable-9]                                                       
        name=Fedora Project Stable RPMS for RHL 9                               
        baseurl=http://download.fedora.us/fedora/redhat/9/i386/yum/stable/   
        gpgcheck=1                                                              
                                                                                
        [fedora-updates-9]                                                      
        name=Fedora Project update RPMS for RHL 9                               
        baseurl=http://download.fedora.us/fedora/redhat/9/i386/yum/updates/  
        gpgcheck=1                                                              
                                                                                

   NOTE: If you're using Redhat 8, replace 9 with "8.0" 

   Next update your GPG-KEY

     rpm --import http://www.fedora.us/FEDORA-GPG-KEY
     rpm --import http://www.fedoralegacy.org/FEDORA-LEGACY-GPG-KEY

   Note, you may need more keys.  I've copied all the Fedora keys that came on
   the DVD.iso. You can download them from here:
   
     http://prdownloads.sourceforge.net/souptonuts/fedora_gpg_keys.tar.gz?download

   The following will download a lot of "HEADER" files, but WILL NOT update 
   anything.  Yes, I too thought my system was being updated with old files,
   but  IT IS NOT.

       yum check-update  

   After this runs, which takes awhile. You can then update selected packages.
   Here is an example of the "libpng10" package being updated.

       yum -y update libpng10

   Also, subsequent calls to "yum check-update" shows what needs to be updated.


   To update every currently installed package

       yum -y update

   or to exclude packages like kernel and httpd:

       yum  --exclude kernel*  --exclude httpd*  -y    update
    


STEP 19 (DEVELOP YOUR OWN 2.6 MODULES):

     You're done with the 2.6 build.  So learn how to develop
     2.6 kernel modules.  First, checkout the following article

      http://lwn.net/Articles/driver-porting/

     Then, take a look at the following sample code, which shows how
     to create /proc entries for communicating with the kernel and writing
     out to any available tty device.

      http://prdownloads.sourceforge.net/souptonuts/procreadwrite.0.0.3.tar.gz?download




UPGRADING TO FEDORA (DISKLESS INSTALL)

     It's possible to completely install Fedora by downloading the *.iso image, mount
     it to a loopback device,  copy the Linux kernel images to /boot, modify grub, reboot, then
     have it find the *.iso image for the rest of the install. 

     STEP A:


         Download the complete DVD package. It's 4 GB: 

               FC2-i386-DVD.iso 

         from: http://download.fedora.redhat.com/pub/fedora/linux/core/2/i386/iso/


    STEP B:

         Once downloaded check the md5sum

              md5sum FC2-i386-DVD.iso


    STEP C:

         Create a directory off of root for mounting the iso image.

              mkdir /iso0


    STEP D:

         The DVD image file can be mounted with the following command:

              mount -o loop -t iso9660 /FC2-i386-DVD.iso  /iso0

         
    STEP E:

         Copy /iso0/images/pxeboot to the /boot directory

              cp -r /iso0/images/pxeboot /boot/.

    STEP F:

         Modify Grub to include the following:


             title Fedora (INIT)                  
	             root (hd0,2)                 
	             kernel /pxeboot/vmlinuz      
	             initrd /pxeboot/initrd.img   
	                                          

        Special note: Look at the other grub entries "root (hd0,1)", perhaps
        it's listed as "root (hd3,0)" or (hd2,0).  This must be the same, so
        change hd0 above to match the other entries.


   STEP G:

        Make note of where FC2-i386-DVD.iso resides by issuing the "df" command.

             $ df .

        You'll need this location after reboot.  It's something like /dev/hda1 or /dev/hdb2           


   STEP H:

        Reboot.  When asked for the location of the "iso" file select FILE on the Fedora menu.
        put in the /dev/hda1 location above.  Note if it's in a subdirectory off of this 
        filesystem there is a text box to enter that as well.

        The install should proceed on its own from here.
        


SPECIAL NOTES:

  Sometimes you may need a  "make mrproper" and "make distclean" if
  you have previously done a  "make modules"  and made new changes
  to oldconfig.

        make mrproper
        make distclean
        make oldconfig
        ...



REFERENCES:

http://www.kernel.org/pub/linux/kernel/README
http://www.codemonkey.org.uk/post-halloween-2.5.txt
http://kerneltrap.org/node/view/799
http://thomer.com/linux/migrate-to-2.6.html 
http://www.kernel.org/

http://bugzilla.kernel.org/
http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&oe=UTF-8&group=linux.kernel
http://linuxdevices.com/articles/AT3855888078.html

http://prdownloads.sourceforge.net/souptonuts/README_26.txt?download
http://prdownloads.sourceforge.net/souptonuts/rc.sysinit.txt?download
http://prdownloads.sourceforge.net/souptonuts/fstab.txt?download
http://prdownloads.sourceforge.net/souptonuts/configs-0.4.tar.gz?download
https://sourceforge.net/forum/forum.php?forum_id=353715

http://www.redhat.com/software/rhel/kernel26/
http://www.tldp.org/HOWTO/KernelAnalysis-HOWTO.html
http://www-124.ibm.com/linux/projects/?topic_id=2

(OpenPGP Signature with Kernel)
http://www.kernel.org/signature.html

Describing the boot process (Good but dated for 2.4 kernel. Still, take a look)
 http://www.tldp.org/LDP/lki/lki.pdf


FEDORA:

http://fedora.artoo.net/faq/

FEDORA GPG KEYS:

http://prdownloads.sourceforge.net/souptonuts/fedora_gpg_keys.tar.gz?download

YUM:

http://linux.duke.edu/projects/yum/download.ptml
http://www.linuxjournal.com/article.php?sid=7448


KERNEL DRIVER DEVELOPMENT IN 2.6:

 Excellent (series of articles):
  http://lwn.net/Articles/driver-porting/
   and
  http://tldp.org/LDP/lkmpg/2.6/lkmpg.pdf

 Here's my sample program:
  http://prdownloads.sourceforge.net/souptonuts/procreadwrite.0.0.3.tar.gz?download
    (Note in 2.6.6+ "current->tty" is being replaced with "current->signal->tty")

 Here's one on how to pass command line arguments to a module.
  http://www.tldp.org/LDP/lkmpg/2.6/html/x333.html

 Good but dated for 2.4 kernel:
  http://www.oreilly.com/catalog/linuxdrive2/
   or for the free version
  http://www.xml.com/ldd/chapter/book/

  http://linuxdevices.com/articles/AT4389927951.html
  http://linuxdevices.com/articles/AT5793467888.html

 Further index of on-line links:
  http://jungla.dit.upm.es/~jmseyas/linux/kernel/hackers-docs.html

 The OS used to develop Linux:
  http://www.minix.org/
  http://www.cs.vu.nl/~ast/minix.html

CONTRIBUTORS:
 Juan Antonio Martinez <jantonio@dit.upm.es>
 jVirdi <jvirdi@users.sourceforge.nt>
