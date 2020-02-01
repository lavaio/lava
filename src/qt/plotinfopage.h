// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PLOTINFOPAGE_H
#define BITCOIN_QT_PLOTINFOPAGE_H

#include <interfaces/wallet.h>

#include <QWidget>
#include <memory>
#include <optional.h>

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class PlatformStyle;
class WalletModel;

namespace Ui {
    class PlotInfoPage;
}

/** PlotInfo(miner) page widget */
class PlotInfoPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlotInfoPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~PlotInfoPage();

    // void setClientModel(ClientModel *clientModel);
    // void setWalletModel(WalletModel *walletModel);
    // void showOutOfSyncWarning(bool fShow);

    void setWalletModel(WalletModel *walletModel);

    struct AddressInfo {
      std::string address;
      std::string plotId;
    };

    void updateData();

private:
    Optional<CPubKey> getDefaultMinerAddress();
    Optional<CPubKey> getNewMinerAddress();

    std::vector<std::pair<int64_t, CKeyID>> getWalletKeys();

    QString getBindingInfoStr(const QPair<AddressInfo, AddressInfo>& data);

public Q_SLOTS:
    void onNewPlotIdClicked();

Q_SIGNALS:

private:
    Ui::PlotInfoPage *ui;

private Q_SLOTS:

    void on_btnQuery_clicked();

    void on_btnBind_clicked();

    void on_btnUnbind_clicked();

private:
    WalletModel* _walletModel;
};

#endif // BITCOIN_QT_PLOTINFOPAGE_H
