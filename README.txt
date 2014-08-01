This is our research prototype code used in the evaluation of our secure
device discovery and recognition (SDDR) protocol, as presented in our paper:

SDDR: Light-Weight, Secure Mobile Encounters
Matthew Lentz, Viktor Erdelyi, Paarijaat Aditya,
Elaine Shi, Peter Druschel, Bobby Bhattacharjee
USENIX Security Symposium 2014


###########################################################################
# Android Pre-Requisites
###########################################################################

A Galaxy Nexus smartphone. (Note: While this software should run on other
devices, we have not attempted to build or run it for anything other than
the Galaxy Nexus.)

Download/Install: Android SDK
(We require android-16 support)

Download/Install: Android NDK
(We used version r9)

Download/Build: Android Source for android-4.1.2 on Galaxy Nexus ("maguro")
(See https://source.android.com/source/building.html)

Download/Install: Galaxy Nexus ("maguro") Factory Image for 4.1.2
(See https://developers.google.com/android/nexus/images)

Software to root the device. The commands provided below assume that they
are executed on a root shell on the device.


###########################################################################
# AWake Application
###########################################################################

In order to let the device suspend between SDDR protocol actions (e.g.,
discovery), the AWake java application must be built and installed on the
phone. Use the following commands:

cd awake
ant debug
adb install -r bin/AWake-debug.apk


###########################################################################
# Third-party Libraries w/ Custom Makefiles
###########################################################################

Use the following commands to build the third-party libraries for Android.
This must only be done for GMP and Google Protocol Buffers, since other
Makefiles were easy enough to port to the Android.mk format.

NOTE: This assumes that $NDKROOT points to the root directory of the NDK.

$NDKROOT/build/tools/make-standalone-toolchain.sh \
--toolchain=arm-linux-androideabi-4.8 \
--platform=android-14 \
--install-dir=/tmp/ndkchain

export PATH=/tmp/ndkchain/bin:$PATH

cd gmp
./configure --host=arm-linux-androideabi
make -j4

cd google
./configure --host=arm-linux-androideabi
make -j4


###########################################################################
# SDDR Application
###########################################################################

Use the following commands to build and install the application:

ndk-build -j4

adb root
adb remount
adb push libs/armeabi-v7a/sddr /system/xbin
adb push libs/armeabi-v7a/libgnustl_shared.so /system/lib


###########################################################################
# jl10psicompare Application
###########################################################################

This application builds with the above SDDR application. Use the following
commands to install the application:

adb root
adb remount
adb push libs/armeabi-v7a/jl10psicompare /system/xbin
adb push libs/armeabi-v7a/libgnustl_shared.so /system/lib
adb push third-party/jl10psi/dsa.1024.priv /data/local
adb push third-party/jl10psi/dsa.1024.pub /data/local


###########################################################################
# Replicating Experiments in USENIX Security 2014 Paper
###########################################################################

[[Figure 2: Protocol execution times of PSI vs SDDR...]]

Modify jni/Android.mk to remove the "-DLOG_W_ENABLED -DLOG_D_ENABLED" flags
to prevent log messages from influencing the results.

Run the following command for advertised set sizes of 2**N, for N in [0,8],
and <# Discoveries> in [0,1]:

> sddr -r BT2 -c None -b <Size of Advertised Set> --psicmp <# Discoveries>

Run the following command for the entire set of PSI times (across all
advertised set sizes):

> jl10psicompare


[[Table 2: Average power and energy consumed by various components...]]
[[Table 3: Energy and battery life consumption for different states...]]
[[Figure 3: Power traces from running the SDDR and DH+PSI protocols...]]

For SDDR over BT2.1 use the following command:

> sddr -r BT2 -c None -b 256 --churn

For DH+PSI over BT2.1 use the following command:

> sddr -r BT2PSI -c None -b 256 --churn

For ResolveAddr in BT4.0 use the following command:

> sddr -r BT4AR -c None -b 256 --churn


[[Figure 4: Estimated daily battery life consumption while running...]]

None. This is based on results from Table 2 and 3, but also looking at 1,3
devices instead of just 5 nearby devices for use in extrapolating the energy
costs associated with the SDDR protocol according to Equation 1.


[[Smokescreen Conservative Energy Estimates]]

Currently this requires modifying the EbNRadioBT2NR.cpp code to ignore
processing the result from the name request. We will update this with an
option in the main manu shortly.
