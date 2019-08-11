#ifndef LAVA_ACTION_DB_H
#define LAVA_ACTION_DB_H

#include <dbwrapper.h>
#include <pubkey.h>
#include <key.h>
#include <streams.h>
#include <primitives/block.h>
#include <primitives/transaction.h>

#include <boost/variant.hpp>

typedef std::pair<CKeyID, CKeyID> CBindAction;
typedef CKeyID CUnbindAction;
class CNilAction {
public:
    friend bool operator==(const CNilAction &a, const CNilAction &b) { return true; }
    friend bool operator<(const CNilAction &a, const CNilAction &b) { return true; }
};

typedef boost::variant<CNilAction, CBindAction, CUnbindAction> CAction;

CAction MakeBindAction(const CKeyID& from, const CKeyID& to);

class CActionVisitor : public boost::static_visitor<bool>
{
private:
    CDataStream* ss;
public:
    explicit CActionVisitor(CDataStream* s) { ss = s; }
public:
    bool operator()(const CNilAction& action) const
    {
        return false;
    }

    bool operator()(const CBindAction& action) const
    {
        *ss << action;
        return true;
    }

    bool operator()(const CUnbindAction& action) const
    {
        *ss << action;
        return true;
    }
};

std::vector<unsigned char> SerializeAction(const CAction& action);

CAction UnserializeAction(const std::vector<unsigned char>& vch);

bool SignAction(const CAction &action, const CKey& key, std::vector<unsigned char>& vch);

bool VerifyAction(const CAction& action, std::vector<unsigned char>& vchSig);

CAction DecodeAction(const CTransactionRef tx, std::vector<unsigned char>& vchSig);

typedef std::pair<CKeyID, CKeyID> CRelation;
typedef std::vector<CRelation> CRelationVector;
class CRelationDB : public CDBWrapper
{
public:
    explicit CRelationDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    ~CRelationDB() = default;

    CKeyID To(const CKeyID& from) const;

    CKeyID To(const uint64_t plotid) const;

    bool AcceptAction(const uint256& txid, const CAction& action);

    bool RollbackAction(const uint256& txid);

    bool SetSynced();

    bool ResetSynced();

    int IsSynced();

    bool EraseDB();

    CRelationVector ListRelations() const;
private:
    bool InsertRelation(const CKeyID& from, const CKeyID& to);
};

/// The global transaction index, used in GetTransaction. May be null.
extern std::unique_ptr<CRelationDB> g_relationdb;
#endif