#ifndef TXFEEMODIFIER_H
#define TXFEEMODIFIER_H
#include <interfaces/wallet.h>
#include <policy/feerate.h>

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

#endif // TXFEEMODIFIER_H
