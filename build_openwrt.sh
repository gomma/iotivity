#scons --config=force -Q \

scons -Q \
	VERBOSE=yes \
	TARGET_OS=linux TARGET_ARCH=mipsel TARGET_TRANSPORT=IP \
	TC_PREFIX=mipsel-openwrt-linux- \
	TC_PATH=/home/gomma/projects/openwrt/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_musl-1.1.11/bin \
	2>&1 | tee build_openwrt.log
