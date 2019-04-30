######################################################################
# 变量说明
# ====================================================================
# BLD_LDIR-y: 厍路径
# BLD_DLIB-y: 连接的动态库
# BLD_SLIB-y：连接的静态库
# BLD_LDFLAG-y: 链接选项
# BLD_CFLAG-y: C参数定义
# BLD_CDEF-y: C宏定义
# BLD_CINC-y: 头文件目录
# ROOT_DIR: 由cfg配置文件传下来，当前编译的目录
######################################################################



# set the app pub dir
APP_RGOBJ := ${ROOT_DIR}/../app-rgobj

# set the inc dir
BLD_CINC-y += ${APP_RGOBJ}
BLD_CINC-y += ${APP_RGOBJ}/poe_proto

# add the depend mod src
BLD_CSRC-y += $(wildcard ${APP_RGOBJ}/*.c)

BLD_DLIB-y += ddm
BLD_LDFLAG-y += -lddm
BLD_DLIB-y += rg_dm_cache

BLD_DLIB-y += rg_cap
BLD_DLIB-y += cap

BLD_DLIB-y += rg_mom zlog rg_at zct hiredis callfun zformat s_cap