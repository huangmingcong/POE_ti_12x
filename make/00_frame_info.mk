########################################################################################
# 变量说明
# ======================================================================================
# BLD_OUT_TOP: 输出目标存放在文件系统中的哪个目录，含USR_ROOTFS_LIB、USR_ROOTFS_SBIN两种
# BLD_OUT_NAME: 编译目标名称
# BLD_MK_TGT: 目标文件的类型，含mk_bin、mk_slib、mk_dlib、mk_cust四种
# ROOT_DIR: 由cfg配置文件传下来，当前编译的目录
########################################################################################

REAL_DIR=$(shell readlink  -f ${ROOT_DIR})

BLD_OUT_TOP := $(USR_ROOTFS_SBIN)
BLD_OUT_NAME := $(shell basename ${REAL_DIR})
BLD_MK_TGT := mk_bin
