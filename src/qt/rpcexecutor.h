#ifndef RPCEXECUTOR_H
#define RPCEXECUTOR_H

#include <QObject>
#include <interfaces/node.h>
#include <qt/walletmodel.h>


/* Object for executing console RPC commands in a separate thread.
*/
class RPCExecutor : public QObject
{
    Q_OBJECT
public:
    explicit RPCExecutor(interfaces::Node& node) : m_node(node) {}

public Q_SLOTS:
    void request(const QString &command, const WalletModel* wallet_model);

Q_SIGNALS:
    void reply(int category, const QString &command);

private:
    interfaces::Node& m_node;
};

#endif // RPCEXECUTOR_H
