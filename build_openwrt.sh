#scons --config=force -Q \

scons -Q \
	VERBOSE=yes \
	TARGET_OS=openwrt TARGET_ARCH=mipsel TARGET_TRANSPORT=IP \
	TC_PREFIX=mipsel-openwrt-linux- \
	TC_PATH=/home/gomma/projects/openwrt/staging_dir/toolchain-mipsel_24kec+dsp_gcc-5.3.0_musl-1.1.14/bin \
	2>&1 | tee build_openwrt.log
