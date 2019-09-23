QT       += core gui network

QT += widgets

CONFIG += c++11 c++14

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += ZMQ_STATIC NOMINMAX _CONSOLE SCL_SECURE_NO_WARNINGS WIN32 QT_DEPRECATED_WARNINGS ENABLE_WALLET HAVE_CONFIG_H _AMD64_ "HAVE_DECL_BSWAP_16=1" "HAVE_DECL_BSWAP_64=1"

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RC_ICONS = res/icons/bitcoin.ico

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
    freesacechecker.cpp \
    guiutil.cpp \
    intro.cpp \
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
    txviewdelegate.cpp \
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
    freesacechecker.h \
    guiconstants.h \
    guiutil.h \
    intro.h \
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
    txviewdelegate.h \
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
  LIBS +=  -lboost_thread-vc140-mt -lboost_filesystem-vc140-mt -lboost_chrono-vc140-mt

  LIBS += -L$$PWD/../../build_msvc/x64/Release -llibleveldb -llibbitcoin_wallet -llibbitcoin_common -llibbitcoin_wallet -llibbitcoin_util -llibleveldb
  LIBS += -llibunivalue -llibbitcoin_cli -llibbitcoin_consensus -llibbitcoin_crypto -llibshabal -llibsecp256k1 -llibbitcoin_server
  LIBS += -llibprotobuf -levent -llibdb48 -llibzmq-mt-s-4_3_3 -llibbitcoin_zmq
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
