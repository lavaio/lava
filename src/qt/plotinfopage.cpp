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

#include <interfaces/wallet.h>
#include <key_io.h>
#include <outputtype.h>

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
