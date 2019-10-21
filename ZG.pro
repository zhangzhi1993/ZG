TEMPLATE = subdirs

SUBDIRS += \
    ZeroMQDemo \
    ZMDCommon \
    ZMDClient \
    ZMDWorker \
    ZMDProxy


ZeroMQDemo.subdir = ProductPlatform/ZeroMQDemo

ZMDCommon.subdir = ProductPlatform/ZMDCommon

ZMDClient.subdir = ProductPlatform/ZMDClient
ZMDClient.depends = ZMDCommon

ZMDProxy.subdir = ProductPlatform/ZMDProxy
ZMDClient.depends = ZMDCommon

ZMDWorker.subdir = ProductPlatform/ZMDWorker
ZMDClient.depends = ZMDCommon
