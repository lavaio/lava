// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/plotinfopage.h>
#include <ui_plotinfopage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>
#include <ui_plotinfopage.h>
#include <QLineEdit>
#include <QMessageBox>
#include <QPair>

#include <interfaces/wallet.h>
#include <key_io.h>
#include <outputtype.h>

#include <optional.h>
#include <validation.h> // cs_main
#include <univalue.h>
#include <actiondb.h>
#include <rpc/protocol.h>
#include <wallet/wallet.h>

Q_DECLARE_METATYPE(interfaces::WalletBalances)

PlotInfoPage::PlotInfoPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlotInfoPage),
    _walletModel(nullptr)
{
    ui->setupUi(this);

    ui->gbNewAddress->hide();

    connect(ui->btnNewAddress, SIGNAL(clicked()), SLOT(onNewPlotIdClicked()));
}

PlotInfoPage::~PlotInfoPage()
{
    delete ui;
}


void PlotInfoPage::setWalletModel(WalletModel *walletModel) {
  _walletModel = walletModel;
  updateData();
}

void PlotInfoPage::updateData() {
  auto& wallet = _walletModel->wallet();
  if (wallet.isLocked()) {
      return;
  }

  auto defaultAddress = getDefaultMinerAddress();
  if(defaultAddress) {
    CTxDestination dest = GetDestinationForKey(defaultAddress.get(), OutputType::LEGACY);
    auto address = QString::fromStdString(EncodeDestination(dest));
    auto plotId = QString("%0").arg(defaultAddress->GetID().GetPlotID());

    ui->ebMinerAddress->setText(address);
    ui->ebPlotId->setText(plotId);
  }
}

Optional<CPubKey> PlotInfoPage::getDefaultMinerAddress()
{
  auto& wallet = _walletModel->wallet();
  std::vector<std::pair<int64_t, CKeyID>> vKeyBirth = getWalletKeys();
  std::vector<std::pair<int64_t, CKeyID>>::const_iterator firstItem = vKeyBirth.begin();
  CKey key;
  std::string label = "miner";
  if (firstItem != vKeyBirth.end()) {
    const CKeyID& keyid = firstItem->second;
    if (wallet.getPrivKey(keyid, key)) {
      auto pubkey = key.GetPubKey();
      wallet.learnRelatedScripts(pubkey, OutputType::P2SH_SEGWIT);

      for (const auto& dest : GetAllDestinationsForKey(pubkey)) {
        if (wallet.hasAddress(dest) == 0) {
          wallet.setAddressBook(dest, label, "receive");
        }
      }

      return boost::make_optional(pubkey);
    }
  }

  return nullopt;
}

void PlotInfoPage::onNewPlotIdClicked()
{
  auto addr = getNewMinerAddress();
  if (addr) {
    CTxDestination dest = GetDestinationForKey(addr.get(), OutputType::LEGACY);
    auto address = QString::fromStdString(EncodeDestination(dest));
    auto plotId = QString("%0").arg(addr->GetID().GetPlotID());

    ui->ebNewAddress->setText(address);
    ui->ebNewId->setText(plotId);
    ui->gbNewAddress->show();
  } else {
    QMessageBox::critical(nullptr, tr("Failed to Generate New Plot Id"),
                          tr("Please check the address pool"));
  }
}

Optional<CPubKey> PlotInfoPage::getNewMinerAddress()
{
  std::string label = "miner";
  auto& wallet = _walletModel->wallet();
  CPubKey newKey;
  if (!wallet.getKeyFromPool(false, newKey)) {
    return nullopt;
  }

  wallet.learnRelatedScripts(newKey, OutputType::P2SH_SEGWIT);

  for (const auto& dest : GetAllDestinationsForKey(newKey)) {
    if (wallet.hasAddress(dest) == 0) {
      wallet.setAddressBook(dest, label, "receive");
    }
  }
  return boost::make_optional(newKey);
}

std::vector<std::pair<int64_t, CKeyID> > PlotInfoPage::getWalletKeys()
{
  auto& wallet = _walletModel->wallet();
  auto keyBirth = wallet.GetKeyBirthTimes();

  // sort time/key pairs
  std::vector<std::pair<int64_t, CKeyID>> vKeyBirth;
  for (const auto& entry : keyBirth) {
      if (const CKeyID* keyID = boost::get<CKeyID>(&entry.first)) { // set and test
          vKeyBirth.push_back(std::make_pair(entry.second, *keyID));
      }
  }
  keyBirth.clear();
  std::sort(vKeyBirth.begin(), vKeyBirth.end());
  return vKeyBirth;
}

static inline Optional<QPair<PlotInfoPage::AddressInfo, PlotInfoPage::AddressInfo>> getBinding(const CKeyID& from) {
    Optional<QPair<PlotInfoPage::AddressInfo, PlotInfoPage::AddressInfo>> ret;

    LOCK(cs_main);
    auto to = prelationview->To(from);
    if (to == CKeyID()) {
        return ret;
    }

    PlotInfoPage::AddressInfo fromInfo = { EncodeDestination(CTxDestination(from)), from.GetPlotID() };
    PlotInfoPage::AddressInfo toInfo = { EncodeDestination(CTxDestination(to)), to.GetPlotID() };

    auto data = QPair<PlotInfoPage::AddressInfo, PlotInfoPage::AddressInfo>(fromInfo, toInfo);
    return boost::make_optional(data);
}

QString PlotInfoPage::getBindingInfoStr(const QPair<PlotInfoPage::AddressInfo, PlotInfoPage::AddressInfo>& data) {
   auto message = tr("Address \"%1\" binds to \"%2\"")
           .arg(QString::fromStdString(data.first.address))
           .arg(QString::fromStdString(data.second.address));
   return message;
}

void PlotInfoPage::on_btnQuery_clicked()
{
   QString address = ui->ebAddrToQuery->text().trimmed();
   if(address.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Please enter a valid address"), QMessageBox::Ok, QMessageBox::Ok);
        return;
   }
   CTxDestination dest = DecodeDestination(address.toStdString());
   if (!IsValidDestination(dest) || dest.type() != typeid(CKeyID)) {
     QMessageBox::warning(this, windowTitle(), tr("Please enter a valid address"), QMessageBox::Ok, QMessageBox::Ok);
     return;
   }

   auto from = boost::get<CKeyID>(dest);
   auto results = getBinding(from);
   if(!results) {
     QMessageBox::information(this, windowTitle(), tr("No binding found for address: \"%1\"").arg(address), QMessageBox::Ok, QMessageBox::Ok);
     return;
   }

   auto data = results.get();
   auto message = getBindingInfoStr(data);
   QMessageBox::information(this, windowTitle(), message, QMessageBox::Ok, QMessageBox::Ok);
}

//
// create transaction fails if there is no min-tx fee set
// we use a small tx fee to make sure the binding transaction success
class TxFeeModifer {
    interfaces::Wallet& _wallet;
    CFeeRate _walletFee;

public:
    TxFeeModifer (interfaces::Wallet& wallet): _wallet(wallet) {
        _walletFee = wallet.getPayTxFee();
        if(_walletFee <= CFeeRate()) {
            CAmount nAmount(1);
            wallet.setPayTxFee(CFeeRate(nAmount, 1000));
        }
    }

   ~TxFeeModifer() {
        _wallet.setPayTxFee(_walletFee);
    }
};

void PlotInfoPage::on_btnBind_clicked()
{
     if(!_walletModel || _walletModel->wallet().isLocked()) {
        QMessageBox::warning(this, windowTitle(), tr("Please unlock wallet to continue"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    auto fromAddr = ui->ebAddressFrom->text().trimmed();
    auto toAddr = ui->ebAddressTo->text().trimmed();
    CTxDestination fromDest = DecodeDestination(fromAddr.toStdString());
    CTxDestination toDest = DecodeDestination(toAddr.toStdString());
    if (!IsValidDestination(fromDest) || fromDest.type() != typeid(CKeyID)) {
      QMessageBox::warning(this, windowTitle(), tr("Invalid from address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }
    if (!IsValidDestination(toDest) || toDest.type() != typeid(CKeyID)) {
      QMessageBox::warning(this, windowTitle(), tr("Invalid to address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    if (fromAddr == toAddr) {
      QMessageBox::warning(this, windowTitle(), tr("From address should not be the same as to address!"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    auto results = getBinding(boost::get<CKeyID>(fromDest));
    if(results) {
      int ret = QMessageBox::warning(this, windowTitle(), tr("This address already binds to an address, are you sure to continue?"), QMessageBox::Yes, QMessageBox::Cancel);
      if (ret != QMessageBox::Yes) {
        return;
      }
    }

    auto lock = _walletModel->requestUnlock();
    if(!lock.isValid()) {
      QMessageBox::warning(this, windowTitle(), tr("Failed to unlock wallet"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    int ret = QMessageBox::warning(this, windowTitle(), tr("Make binding takes 16 lv, are you sure to continue?"), QMessageBox::Yes, QMessageBox::Cancel);
    if (ret != QMessageBox::Yes) {
        return;
    }

    auto fromKey = _walletModel->wallet().getKeyForDestination(fromDest);
    if (fromKey.IsNull()) {
      QMessageBox::warning(this, windowTitle(), tr("Invalid from address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }
    auto toKey = boost::get<CKeyID>(toDest);

    CKey key;
    if (!_walletModel->wallet().getPrivKey(fromKey, key)) {
        QMessageBox::warning(this, windowTitle(), tr("Wallet doesn't have private key for address: \"%1\""), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    try {
        auto action = MakeBindAction(fromKey, toKey);
        QString message;
        {
          TxFeeModifer feeUpdater(_walletModel->wallet());
          auto txid = _walletModel->wallet().sendAction(action, key, CTxDestination(fromKey));
          message = tr("Transaction \"%1\" was created for plot id binding").arg(QString::fromStdString(txid.GetHex()));
        }

        QMessageBox::information(this, windowTitle(), message, QMessageBox::Ok, QMessageBox::Ok);
    } catch(const UniValue& ex) {
        std::map<std::string, UniValue> valMap;
        ex.getObjMap(valMap);
        auto code = valMap["code"].get_int();
        auto msg = valMap["message"].get_str();
        auto message = QString("got error code: %1, message: %2").arg(code).arg(QString::fromStdString(msg));
        QMessageBox::critical(this, windowTitle(), message, QMessageBox::Ok, QMessageBox::Ok);
    }catch(...) {
        QMessageBox::critical(this, windowTitle(), tr("Failed to create plot id binding transaction, please make sure there is enough balance in your wallet"), QMessageBox::Ok, QMessageBox::Ok);
    }
}

void PlotInfoPage::on_btnUnbind_clicked()
{
    if(!_walletModel || _walletModel->wallet().isLocked()) {
        QMessageBox::warning(this, windowTitle(), tr("Please unlock wallet to continue"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    auto fromAddr = ui->ebAddressFrom->text().trimmed();
    CTxDestination fromDest = DecodeDestination(fromAddr.toStdString());
    if (!IsValidDestination(fromDest) || fromDest.type() != typeid(CKeyID)) {
      QMessageBox::warning(this, windowTitle(), tr("Invalid from address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }


    auto lock = _walletModel->requestUnlock();
    if(!lock.isValid()) {
      QMessageBox::warning(this, windowTitle(), tr("Failed to unlock wallet"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    auto results = getBinding(boost::get<CKeyID>(fromDest));
    if(!results) {
      QMessageBox::information(this, windowTitle(), tr("No binding for this address, please don't waste your money!"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    int ret = QMessageBox::warning(this, windowTitle(), tr("Unbinding takes 16 lv, are you sure to continue?"), QMessageBox::Yes, QMessageBox::Cancel);
    if (ret != QMessageBox::Yes) {
        return;
    }

    auto fromKey = _walletModel->wallet().getKeyForDestination(fromDest);
    if (fromKey.IsNull()) {
      QMessageBox::warning(this, windowTitle(), tr("Invalid from address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }
    CKey key;
    if (!_walletModel->wallet().getPrivKey(fromKey, key)) {
        QMessageBox::warning(this, windowTitle(), tr("Wallet doesn't have private key for address: \"%1\""), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    try {
        auto action = CAction(CUnbindAction(fromKey));
        QString message;
        {
          TxFeeModifer feeUpdater(_walletModel->wallet());
          auto txid = _walletModel->wallet().sendAction(action, key, CTxDestination(fromKey));
          message = tr("Transaction \"%1\" was created for plot id unbinding").arg(QString::fromStdString(txid.GetHex()));
        }

        QMessageBox::information(this, windowTitle(), message, QMessageBox::Ok, QMessageBox::Ok);
    } catch(const UniValue& ex) {
        std::map<std::string, UniValue> valMap;
        ex.getObjMap(valMap);
        auto code = valMap["code"].get_int();
        auto msg = valMap["message"].get_str();
        auto message = QString("got error code: %1, message: %2").arg(code).arg(QString::fromStdString(msg));
        QMessageBox::critical(this, windowTitle(), message, QMessageBox::Ok, QMessageBox::Ok);
    }catch(...) {
        QMessageBox::critical(this, windowTitle(), tr("Failed to create unbind transaction, please make sure there is enough balance in your wallet"), QMessageBox::Ok, QMessageBox::Ok);
    }
}
