TEMPLATE = subdirs

SUBDIRS += \
#    ZeroMQDemo \
    ZMDCommon \
#    ZMDClient \
    ZMDWorker \
#    ZMDProxy \
    ZMDClientProxy


ZeroMQDemo.subdir = ProductPlatform/ZeroMQDemo

ZMDCommon.subdir = ProductPlatform/ZMDCommon

ZMDClient.subdir = ProductPlatform/ZMDClient
ZMDClient.depends = ZMDCommon

ZMDClientProxy.subdir = ProductPlatform/ZMDClientProxy
ZMDClientProxy.depends = ZMDCommon

ZMDProxy.subdir = ProductPlatform/ZMDProxy
ZMDProxy.depends = ZMDCommon

ZMDWorker.subdir = ProductPlatform/ZMDWorker
ZMDWorker.depends = ZMDCommon
