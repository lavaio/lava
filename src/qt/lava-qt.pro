QT       += core gui network

QT += widgets

CONFIG += c++11 c++14 static

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += NOMINMAX _CONSOLE SCL_SECURE_NO_WARNINGS QT_DEPRECATED_WARNINGS ENABLE_WALLET HAVE_CONFIG_H _AMD64_

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RC_ICONS = res/icons/bitcoin.ico
ICON = res/icons/bitcoin.icns

SOURCES += \
    addressbookpage.cpp \
    addresstablemodel.cpp \
    amountspinbox.cpp \
    askpassphrasedialog.cpp \
    bantablemodel.cpp \
    bitcoin.cpp \
    bitcoinaddressvalidator.cpp \
    bitcoinamountfield.cpp \
    bitcoingui.cpp \
    bitcoinstrings.cpp \
    bitcoinunits.cpp \
    clientmodel.cpp \
    coincontroldialog.cpp \
    coincontroltreewidget.cpp \
    csvmodelwriter.cpp \
    editaddressdialog.cpp \
    forms/firestoneinfopage.cpp \
    freesacechecker.cpp \
    guiutil.cpp \
    intro.cpp \
    lava/slotinfo.cpp \
    main.cpp \
    modaloverlay.cpp \
    networkstyle.cpp \
    notificator.cpp \
    openuridialog.cpp \
    optionsdialog.cpp \
    optionsmodel.cpp \
    overviewpage.cpp \
    paymentrequest.pb.cc \
    paymentrequestplus.cpp \
    paymentserver.cpp \
    peertablemodel.cpp \
    platformstyle.cpp \
    plotinfopage.cpp \
    qtrpctimerbase.cpp \
    qvalidatedlineedit.cpp \
    qvaluecombobox.cpp \
    receivecoinsdialog.cpp \
    receiverequestdialog.cpp \
    recentrequeststablemodel.cpp \
    rpcconsole.cpp \
    rpcexecutor.cpp \
    sendcoinsdialog.cpp \
    sendcoinsentry.cpp \
    signverifymessagedialog.cpp \
    splashscreen.cpp \
    trafficgraphwidget.cpp \
    transactiondesc.cpp \
    transactiondescdialog.cpp \
    transactionfilterproxy.cpp \
    transactionrecord.cpp \
    transactiontablemodel.cpp \
    transactionview.cpp \
    txfeemodifier.cpp \
    txviewdelegate.cpp \
    uiexception.cpp \
    utilitydialog.cpp \
    walletcontroller.cpp \
    walletframe.cpp \
    walletmodel.cpp \
    walletmodeltransaction.cpp \
    walletview.cpp \
    winshutdownmonitor.cpp

HEADERS += \ \
    addressbookpage.h \
    addresstablemodel.h \
    amountspinbox.h \
    askpassphrasedialog.h \
    bantablemodel.h \
    bitcoin.h \
    bitcoinaddressvalidator.h \
    bitcoinamountfield.h \
    bitcoingui.h \
    bitcoinunits.h \
    clientmodel.h \
    coincontroldialog.h \
    coincontroltreewidget.h \
    csvmodelwriter.h \
    editaddressdialog.h \
    forms/firestoneinfopage.h \
    freesacechecker.h \
    guiconstants.h \
    guiutil.h \
    intro.h \
    lava/slotinfo.h \
    modaloverlay.h \
    networkstyle.h \
    notificator.h \
    openuridialog.h \
    optionsdialog.h \
    optionsmodel.h \
    overviewpage.h \
    paymentrequest.pb.h \
    paymentrequestplus.h \
    paymentserver.h \
    peertablemodel.h \
    platformstyle.h \
    plotinfopage.h \
    qtrpctimerbase.h \
    qvalidatedlineedit.h \
    qvaluecombobox.h \
    receivecoinsdialog.h \
    receiverequestdialog.h \
    recentrequeststablemodel.h \
    rpcconsole.h \
    rpcexecutor.h \
    sendcoinsdialog.h \
    sendcoinsentry.h \
    signverifymessagedialog.h \
    splashscreen.h \
    trafficgraphwidget.h \
    transactiondesc.h \
    transactiondescdialog.h \
    transactionfilterproxy.h \
    transactionrecord.h \
    transactiontablemodel.h \
    transactionview.h \
    txfeemodifier.h \
    txviewdelegate.h \
    uiexception.h \
    utilitydialog.h \
    walletcontroller.h \
    walletframe.h \
    walletmodel.h \
    walletmodeltransaction.h \
    walletview.h \
    winshutdownmonitor.h

INCLUDEPATH += $$PWD/../ \
             $$PWD/../leveldb/include \
             $$PWD/../univalue/include

windows {
  DEFINES += WIN32 "HAVE_DECL_BSWAP_16=1" "HAVE_DECL_BSWAP_64=1" ZMQ_STATIC
  LIBS +=  -lboost_thread-vc140-mt -lboost_filesystem-vc140-mt -lboost_chrono-vc140-mt

  LIBS += -L$$PWD/../../build_msvc/x64/Release -llibleveldb -llibbitcoin_wallet -llibbitcoin_common -llibbitcoin_util -llibleveldb
  LIBS += -llibunivalue -llibbitcoin_cli -llibbitcoin_consensus -llibbitcoin_crypto -llibshabal -llibsecp256k1 -llibbitcoin_server
  LIBS += -llibprotobuf -levent -llibdb48 -llibzmq-mt-s-4_3_3 -llibbitcoin_zmq
}

darwin {
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

  INCLUDEPATH += /usr/local/include /usr/local/opt/berkeley-db@4/include

  LIBS += -L$$PWD/..
  LIBS += -L/usr/local/lib -L/usr/local/Cellar/openssl/1.0.2t/lib -lcrypto
  LIBS += -lbitcoin_wallet -lbitcoin_common -lbitcoin_wallet -lbitcoin_util -lleveldb
  LIBS += -L$$PWD/../univalue/.libs/ -lunivalue
  LIBS += -L$$PWD/../secp256k1/.libs/ -lsecp256k1
  LIBS += -L$$PWD/../crypto -lbitcoin_crypto_shani -lbitcoin_crypto_avx2 -lbitcoin_crypto_base -lbitcoin_crypto_sse41
  LIBS += -lbitcoin_cli -lbitcoin_consensus -lbitcoin_server
  LIBS += -L/usr/local/Cellar/miniupnpc/2.1/lib -lminiupnpc -lqrencode

  LIBS += -lboost_thread-mt -lboost_filesystem-mt -lboost_chrono-mt

  LIBS += -lprotobuf -levent -levent_pthreads
  LIBS += -L$$PWD/../../db4/lib -ldb_cxx-4.8
  LIBS += -framework Cocoa -framework Foundation

  SOURCES += macos_appnap.mm \
         macdockiconhandler.mm \
         macnotificationhandler.mm

  HEADERS +=  macos_appnap.h \
         macdockiconhandler.h \
         macnotificationhandler.h

  TR_EXCLUDE += /usr/local/include/boost/*
}


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    forms/addressbookpage.ui \
    forms/askpassphrasedialog.ui \
    forms/coincontroldialog.ui \
    forms/debugwindow.ui \
    forms/editaddressdialog.ui \
    forms/firestoneinfopage.ui \
    forms/helpmessagedialog.ui \
    forms/intro.ui \
    forms/minerviewpage.ui \
    forms/modaloverlay.ui \
    forms/openuridialog.ui \
    forms/optionsdialog.ui \
    forms/overviewpage.ui \
    forms/plotinfopage.ui \
    forms/receivecoinsdialog.ui \
    forms/receiverequestdialog.ui \
    forms/sendcoinsdialog.ui \
    forms/sendcoinsentry.ui \
    forms/signverifymessagedialog.ui \
    forms/transactiondescdialog.ui

TRANSLATIONS = locale/bitcoin_zh_CN.ts

RESOURCES += \
    bitcoin.qrc \
    bitcoin_locale.qrc

DISTFILES +=
