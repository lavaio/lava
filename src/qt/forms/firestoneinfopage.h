#ifndef FIRESTONEINFOPAGE_H
#define FIRESTONEINFOPAGE_H

#include <QWidget>

namespace Ui {
class FirestoneInfoPage;
}


class PlatformStyle;
class WalletModel;
class CKeyID;

class FirestoneInfoPage : public QWidget
{
    Q_OBJECT

public:
    explicit FirestoneInfoPage(const PlatformStyle* style, QWidget *parent = nullptr);
    ~FirestoneInfoPage();

    void setWalletModel(WalletModel *walletModel);

    struct FirestoneInfo {
        size_t totalCount;
        size_t curSlotCount;
        size_t nextSlotCount;
        size_t overdueCount;
    };

private slots:
    void on_btnQuery_clicked();
    FirestoneInfo getFirestoneInfo(const CKeyID& key);

    void on_btnBuy_clicked();

    void on_btnRelease_clicked();

private:
    void updateData();

private:
    Ui::FirestoneInfoPage *ui;
    WalletModel *_walletModel;
};

#endif // FIRESTONEINFOPAGE_H
