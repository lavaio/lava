// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/minerviewpage.h>
#include <qt/forms/ui_minerviewpage.h>
#include <qt/platformstyle.h>

MinerviewPage::MinerviewPage(const PlatformStyle *platformStyle, QWidget *parent)
    :ui(new Ui::MinerviewPage)
{
    ui->setupUi(this);
}

MinerviewPage::~MinerviewPage()
{

}
