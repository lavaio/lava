#ifndef LAVA_ACTION_DB_H
#define LAVA_ACTION_DB_H

#include <dbwrapper.h>
#include <pubkey.h>
#include <key.h>
#include <streams.h>
#include <primitives/block.h>

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

bool SignAction(const uint256 prevTxHash, const CAction &action, const CKey& key, std::vector<unsigned char>& vch);

bool VerifyAction(const uint256 prevTxHash, const CAction& action, std::vector<unsigned char>& vchSig);

CAction DecodeAction(const CTransactionRef tx, std::vector<unsigned char>& vchSig);

typedef std::pair<CKeyID, CKeyID> CRelation;
typedef std::vector<CRelation> CRelationVector;
typedef std::map<uint64_t,uint64_t> RelationMap;
typedef std::map<int,RelationMap> RelationMapIndex;
typedef std::pair<CKeyID, CKeyID> CRelationActive;

class CRelationView : public CDBWrapper
{
public:
    explicit CRelationView(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    ~CRelationView() = default;

    CKeyID To(const CKeyID& from) const;

    CKeyID To(const uint64_t plotid) const;

    bool AcceptAction(const int height, const uint256& txid, const CAction& action, std::vector<std::pair<uint256, CRelationActive>> &relations);

    void ConnectBlock(const int height, const CBlock &blk);

    void DisconnectBlock(const int height, const CBlock &blk);

    bool WriteTicketsToDisk(const int height, const std::vector<std::pair<uint256, CRelationActive>> &relations);

    bool LoadRelationFromDisk(const int height);

    CRelationVector ListRelations() const;
private:
    RelationMap relationTip;
    RelationMapIndex relationMapIndex; 
};

#endif