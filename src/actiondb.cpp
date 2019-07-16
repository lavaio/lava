#include <actiondb.h>
#include <validation.h>
#include <chainparams.h>

CAction MakeBindAction(const CKeyID& from, const CKeyID& to)
{
    CBindAction ba(std::make_pair(from, to));
    return std::move(CAction(ba));
}

bool SignAction(const CAction &action, const CKey& key, std::vector<unsigned char>& vch)
{
    vch.clear();
    auto actionVch = SerializeAction(action);
    vch.insert(vch.end(), actionVch.begin(), actionVch.end());
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << actionVch;
    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig)) {
        return false;
    }
    vch.insert(vch.end(), vchSig.begin(), vchSig.end());
    return true;
}

bool VerifyAction(const CAction& action, std::vector<unsigned char>& vchSig)
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << SerializeAction(action);
    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;
    auto result{ false };
    if (action.type() == typeid(CBindAction)) {
        auto from = boost::get<CBindAction>(action).first;
        result = from == pubkey.GetID();
    } else if (action.type() == typeid(CUnbindAction)) {
        auto from = boost::get<CUnbindAction>(action);
        result = from == pubkey.GetID();
    }
    return result;
}

std::vector<unsigned char> SerializeAction(const CAction& action) {
    CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << action.which();
    boost::apply_visitor(CActionVisitor(&ss), action);
    return std::vector<unsigned char>(ss.begin(), ss.end());
}

CAction UnserializeAction(const std::vector<unsigned char>& vch) {
    CDataStream ss(vch, SER_GETHASH, PROTOCOL_VERSION);
    int ty = 0;
    ss >> ty;
    switch (ty) {
    case 1:
    {
        CBindAction ba;
        ss >> ba;
        return std::move(CAction(ba));
    }
    case 2:
    {
        CUnbindAction uba;
        ss >> uba;
        return std::move(CAction(uba));
    }
    }
    return std::move(CAction(CNilAction{}));
}

CAction DecodeAction(const CTransactionRef tx, std::vector<unsigned char>& vchSig)
{
    do {
        if (tx->IsCoinBase() || tx->IsNull() || tx->vout.size() != 2) continue;
        auto outValue = tx->GetValueOut();
        CAmount nAmount{ 0 };
        for (auto vin : tx->vin) {
            CTransactionRef prevTx;
            uint256 hashBlock;
            GetTransaction(vin.prevout.hash, prevTx, Params().GetConsensus(), hashBlock);
            nAmount += prevTx->vout[vin.prevout.n].nValue;
        }
        if (nAmount - outValue != Params().GetConsensus().nActionFee) {
            continue;
        }
        for (auto vout : tx->vout) {
            if (vout.nValue != 0) continue;
            auto script = vout.scriptPubKey;
            CScriptBase::const_iterator pc = script.begin();
            opcodetype opcodeRet;
            std::vector<unsigned char> vchRet;
            if (!script.GetOp(pc, opcodeRet, vchRet) || opcodeRet != OP_RETURN) {
                continue;
            }
            script.GetOp(pc, opcodeRet, vchRet);
            auto action = UnserializeAction(vchRet);
            vchSig.clear();
            vchSig.insert(vchSig.end(), vchRet.end() - 65, vchRet.end());
            return std::move(action);
        }
    } while (false);
    return CAction(CNilAction{});
}

std::unique_ptr<CRelationDB> g_relationdb;

static const char DB_RELATION_KEY = 'K';
static const char DB_ACTIVE_ACTION_KEY = 'A';

CRelationDB::CRelationDB(size_t nCacheSize, bool fMemory, bool fWipe)
    : CDBWrapper(GetDataDir() / "index" / "relation", nCacheSize, fMemory, fWipe) 
{
}

bool CRelationDB::InsertRelation(const CKeyID& from, const CKeyID& to)
{
    return Write(std::make_pair(DB_RELATION_KEY, from), to);
}

CKeyID CRelationDB::To(const CKeyID& from) const
{
    CKeyID to;
    if (!Read(std::make_pair(DB_RELATION_KEY, from), to)) {
        //TODO: error catch
    }
    return std::move(to);
}

CKeyID CRelationDB::To(const uint64_t plotid) const
{
    std::unique_ptr<CDBIterator> iter(const_cast<CRelationDB&>(*this).NewIterator());
    iter->Seek(std::make_pair(DB_RELATION_KEY, CKeyID()));
    while (iter->Valid()) {
        auto key = std::make_pair(DB_RELATION_KEY, CKeyID());
        iter->GetKey(key);
        if (key.second.GetPlotID() == plotid) {
            CKeyID to;
            iter->GetValue(to);
            return std::move(to);
        }
        iter->Next();
    }
    return std::move(CKeyID());
}

typedef std::pair<CKeyID, CKeyID> CRelationActive;

bool CRelationDB::AcceptAction(const uint256& txid, const CAction& action)
{
    CDBBatch batch(*this);
    if (action.type() == typeid(CBindAction)) {
        auto ba = boost::get<CBindAction>(action);
        auto from = ba.first;
        auto nowTo = To(from);
        CRelationActive active{ std::make_pair(from, nowTo) };
        batch.Write(std::make_pair(DB_ACTIVE_ACTION_KEY, txid), active);
        batch.Write(std::make_pair(DB_RELATION_KEY, from), ba.second);
    } else if (action.type() == typeid(CUnbindAction)) {
        auto from = boost::get<CUnbindAction>(action);
        auto nowTo = To(from);
        CRelationActive active{ std::make_pair(from, nowTo) };
        batch.Write(std::make_pair(DB_ACTIVE_ACTION_KEY, txid), active);
        batch.Erase(std::make_pair(DB_RELATION_KEY, from));
    }
    return WriteBatch(batch);
}

bool CRelationDB::RollbackAction(const uint256& txid)
{
    CRelationActive active;
    auto activeKey = std::make_pair(DB_ACTIVE_ACTION_KEY, txid);
    if (!Read(activeKey, active))
        return false;
    CDBBatch batch(*this);
    batch.Erase(activeKey);
    auto relKey = std::make_pair(DB_RELATION_KEY, active.first);
    if (active.second != CKeyID()) {
        batch.Write(relKey, active.second);
    } else if (Exists(relKey)) {
        batch.Erase(relKey);
    }
    return WriteBatch(batch);
}