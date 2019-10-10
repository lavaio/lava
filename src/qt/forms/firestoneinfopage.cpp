#include "firestoneinfopage.h"
#include "ui_firestoneinfopage.h"

#include <QList>
#include <QDebug>
#include <qt/walletmodel.h>
#include "lava/slotinfo.h"
#include "guiutil.h"
#include <key_io.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>
#include <univalue.h>
#include <node/transaction.h>
#include <txmempool.h>
#include "txfeemodifier.h"

FirestoneInfoPage::FirestoneInfoPage(const PlatformStyle* style, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FirestoneInfoPage)
{
    ui->setupUi(this);
}

FirestoneInfoPage::~FirestoneInfoPage()
{
    delete ui;
}

void FirestoneInfoPage::setWalletModel(WalletModel *walletModel)
{
    _walletModel = walletModel;
    updateData();
}

void FirestoneInfoPage::updateData()
{
    if (!_walletModel || _walletModel->wallet().isLocked()) {
        return;
    }

    auto slotInfo = SlotInfo::currentSlotInfo();
    ui->ebSlotIdx->setText(QString("%1").arg(slotInfo.index));
    ui->ebFirestoneCount->setText(QString("%1").arg(slotInfo.count));
    ui->ebLockTime->setText(QString("%1").arg(slotInfo.lockTime));
    ui->ebFireStonePrice->setText(GUIUtil::formatPrice(*_walletModel, slotInfo.price));
}

void FirestoneInfoPage::on_btnQuery_clicked()
{
    auto fromAddr = ui->ebQueryAddress->text().trimmed();
    CTxDestination fromDest = DecodeDestination(fromAddr.toStdString());
    if (!IsValidDestination(fromDest) || fromDest.type() != typeid(CKeyID)) {
      QMessageBox::warning(this, windowTitle(), tr("Invalid address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    qInfo() << "here1" << endl;
    auto firestoneInfo = getFirestoneInfo(boost::get<CKeyID>(fromDest));

    auto strTotal = tr("Address \"%1\" has %2 firestones").arg(fromAddr).arg(firestoneInfo.totalCount);
    auto strCurSlot = tr("%1 firestone(s) in current slot").arg(firestoneInfo.curSlotCount);
    auto strNextSlot = tr("%1 immature firestone(s)").arg(firestoneInfo.nextSlotCount);
    auto strOverdue = tr("%1 overdue firestone(s)").arg(firestoneInfo.overdueCount);

    auto parts = QList<QString>() << strTotal << strCurSlot << strNextSlot << strOverdue;
    auto message = parts.join("\n");

    QMessageBox::information(this, tr("Firestone Information"), message, QMessageBox::Ok, QMessageBox::Ok);
}

FirestoneInfoPage::FirestoneInfo FirestoneInfoPage::getFirestoneInfo(const CKeyID &key)
{
    std::vector<CTicketRef> alltickets;
    {
      //
      // scope to limit the lock scope...
      LOCK(cs_main);
      alltickets = pticketview->FindeTickets(key);
      auto end = std::remove_if(alltickets.begin(), alltickets.end(), [](const CTicketRef& ticket) {
        return pcoinsTip->AccessCoin(COutPoint(ticket->out->hash, ticket->out->n)).IsSpent();
      });
      alltickets.erase(end, alltickets.end());
    }

    size_t useableCount = 0;
    size_t overdueCount = 0;
    size_t immatureCount = 0;

    std::for_each(alltickets.begin(), alltickets.end(), [&useableCount, &overdueCount, &immatureCount](const CTicketRef& ticket) {
        int height = ticket->LockTime();
        auto keyid = ticket->KeyID();
        if (keyid.size() == 0 || height == 0) {
            return;
        }
        switch (ticket->State(chainActive.Tip()->nHeight)){
        case CTicket::CTicketState::IMMATURATE:
            immatureCount++;
          break;
        case CTicket::CTicketState::USEABLE:
            useableCount++;
          break;
        case CTicket::CTicketState::OVERDUE:
            overdueCount++;
          break;
        case CTicket::CTicketState::UNKNOW:
          break;
        }
    });
    FirestoneInfo info = {
      alltickets.size(), useableCount, immatureCount, overdueCount
    };
    return info;
}

extern void ImportScript(CWallet* const pwallet, const CScript& script, const std::string& strLabel, bool isRedeemScript) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet);

void FirestoneInfoPage::on_btnBuy_clicked()
{
  if(!_walletModel || _walletModel->wallet().isLocked()) {
    QMessageBox::warning(this, windowTitle(), tr("Please unlock wallet to continue"), QMessageBox::Ok, QMessageBox::Ok);
    return;
  }

  auto buyAddr = ui->ebBuyAddress->text().trimmed();
  CTxDestination buyDest = DecodeDestination(buyAddr.toStdString());
  if (!IsValidDestination(buyDest) || buyDest.type() != typeid(CKeyID)) {
    QMessageBox::warning(this, windowTitle(), tr("Invalid target address"), QMessageBox::Ok, QMessageBox::Ok);
    return;
  }

  auto changeAddr = ui->ebChangeAddress->text().trimmed();
  CTxDestination changeDest = DecodeDestination(changeAddr.toStdString());
  if (!IsValidDestination(changeDest) || changeDest.type() != typeid(CKeyID)) {
    QMessageBox::warning(this, windowTitle(), tr("Invalid change address"), QMessageBox::Ok, QMessageBox::Ok);
    return;
  }

  auto buyID = boost::get<CKeyID>(buyDest);
  auto changeID = boost::get<CKeyID>(changeDest);

  CKey privKeyTemp;
  if(!_walletModel->wallet().getPrivKey(buyID, privKeyTemp)) {
    auto ret = QMessageBox::warning(this, windowTitle(), tr("Target address isn't an address in your wallet, are you sure?"), QMessageBox::Yes, QMessageBox::No);
    if (ret != QMessageBox::Yes) {
      return;
    }
  }

  if(!_walletModel->wallet().getPrivKey(changeID, privKeyTemp)) {
    auto ret = QMessageBox::warning(this, windowTitle(), tr("Change address isn't an address in your wallet, are you sure?"), QMessageBox::Yes, QMessageBox::No);
    if (ret != QMessageBox::Yes) {
      return;
    }
  }

  try {
      _walletModel->wallet().doWithChainAndWalletLock([&](auto& lockedChain, auto& wallet) {
        LOCK(cs_main);

        TxFeeModifer feeUpdater(_walletModel->wallet());
        auto locktime = pticketview->LockTime();
        if (locktime == chainActive.Height()) {
          QMessageBox::warning(this, windowTitle(), tr("Can't buy firestone on slot's last block"), QMessageBox::Ok, QMessageBox::Ok);
          return;
        }

        auto nAmount = pticketview->CurrentTicketPrice();
        auto redeemScript = GenerateTicketScript(buyID, locktime);
        buyDest = CTxDestination(CScriptID(redeemScript));
        auto scriptPubkey = GetScriptForDestination(buyDest);
        auto opRetScript = CScript() << OP_RETURN << CTicket::VERSION << ToByteVector(redeemScript);

        //set change dest
        CCoinControl coin_control;
        coin_control.destChange = CTxDestination(changeID);

        mapValue_t mapValue;
        CTransactionRef tx = SendMoneyWithOpRet(*lockedChain, &wallet, buyDest, nAmount, false, opRetScript, coin_control, std::move(mapValue));
        ImportScript(&wallet, redeemScript, "tickets", true);
      });

    QMessageBox::information(this, windowTitle(), tr("Buy firestone success!"), QMessageBox::Ok, QMessageBox::Ok);
  } catch(...) {
    QMessageBox::critical(this, windowTitle(), tr("Failed to buy firestone, please make sure there is enough balance in your wallet"), QMessageBox::Ok, QMessageBox::Ok);
  }
}

void FirestoneInfoPage::on_btnRelease_clicked()
{
    if(!_walletModel || _walletModel->wallet().isLocked()) {
      QMessageBox::warning(this, windowTitle(), tr("Please unlock wallet to continue"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }
    auto relAddr = ui->ebReleaseAddress->text().trimmed();
    CTxDestination relDest = DecodeDestination(relAddr.toStdString());
    if (!IsValidDestination(relDest) || relDest.type() != typeid(CKeyID)) {
      QMessageBox::warning(this, windowTitle(), tr("Invalid address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    auto refundAddr = ui->ebRefundAddress->text().trimmed();
    CTxDestination refundDest = DecodeDestination(refundAddr.toStdString());
    if (!IsValidDestination(refundDest) || refundDest.type() != typeid(CKeyID)) {
      QMessageBox::warning(this, windowTitle(), tr("Invalid refund address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    CKey key;
    auto relID = boost::get<CKeyID>(relDest);
    if (!_walletModel->wallet().getPrivKey(relID, key)){
      QMessageBox::warning(this, windowTitle(), tr("This address isn't your address, you can't release firestone for it"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    std::vector<CTicketRef> alltickets;
    {
      LOCK(cs_main);
      alltickets = pticketview->FindeTickets(boost::get<CKeyID>(relDest));
    }

    std::vector<CTicketRef> tickets;
    for(size_t i=0;i < alltickets.size(); i++){
        auto ticket = alltickets[i];
        auto out = *(ticket->out);
        if (!pcoinsTip->AccessCoin(out).IsSpent() && !mempool.isSpent(out)){
            tickets.push_back(ticket);
            if (tickets.size() > 4)
                break;
        }
    }
    if(tickets.empty()) {
      QMessageBox::information(this, windowTitle(), tr("No firestones in this address"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    TxFeeModifer feeUpdater(_walletModel->wallet());

    UniValue results(UniValue::VOBJ);
    std::map<uint256,std::pair<int,CScript>> txScriptInputs;
    std::vector<CTxOut> outs;
    UniValue ticketids(UniValue::VARR);
    for(auto iter = tickets.begin(); iter!=tickets.end(); iter++){
      auto state = (*iter)->State(chainActive.Height());
      if (state == CTicket::CTicketState::OVERDUE){
        auto ticket = (*iter);
        uint256 txid = ticket->out->hash;
        uint32_t n = ticket->out->n;
        CScript redeemScript = ticket->redeemScript;
        ticketids.push_back(txid.ToString() + ":" + itostr(n));

        // construct the freefirestone tx inputs.
        auto prevTx = MakeTransactionRef();
        uint256 hashBlock;
        if (!GetTransaction(txid, prevTx, Params().GetConsensus(), hashBlock)) {
            QMessageBox::critical(this, windowTitle(), tr("Failed to free firestone, something unexpected happened..."), QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
        txScriptInputs.insert(std::make_pair(txid,std::make_pair(n,redeemScript)));
        outs.push_back(prevTx->vout[n]);
      }
    }
    results.pushKV("OutPoint", ticketids);

    auto tx = _walletModel->wallet().createTicketAllSpendTx(txScriptInputs, outs, refundDest, key);

    std::string errStr;
    uint256 spendTxID;
    const CAmount highfee{ ::maxTxFee };
    if (TransactionError::OK != BroadcastTransaction(tx, spendTxID, errStr, 50*highfee)) {
      QMessageBox::warning(this, windowTitle(), tr("Failed to broadcast transation!"), QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    QMessageBox::information(this, windowTitle(), tr("Released %1 firestone(s)").arg(tickets.size()),
                             QMessageBox::Ok, QMessageBox::Ok);
}
