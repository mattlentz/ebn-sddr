Use the following commands to build the third-party libraries for Android. This
must only be done for GMP and Google Protocol Buffers, since other Makefiles
were basic enough to easily port to the Android.mk format.

This assumes that $NDKROOT points to the root directory of the NDK.

> $NDKROOT/build/tools/make-standalone-toolchain.sh --toolchain=arm-linux-androideabi-4.8 --platform=android-14 --install-dir=/tmp/ndkchain
> export PATH=/tmp/ndkchain/bin:$PATH

> cd gmp
> ./configure --host=arm-linux-androideabi
> make -j6

> cd google
> ./configure --host=arm-linux-androideabi
> make -j6

