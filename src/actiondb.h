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

/** 
 * Sign the transaction, with outpoint inside.
 */
bool SignAction(const COutPoint& out, const CAction &action, const CKey& key, std::vector<unsigned char>& vch);

bool VerifyAction(const COutPoint& out, const CAction& action, std::vector<unsigned char>& vchSig);

CAction DecodeAction(const CTransactionRef& tx, std::vector<unsigned char>& vchSig);

typedef std::pair<CKeyID, CKeyID> CRelation;
typedef std::vector<CRelation> CRelationVector;
typedef std::map<uint64_t,uint64_t> RelationMap;
typedef std::map<CKeyID,CKeyID> RelationKeyIDMap;
typedef std::map<int,RelationMap> RelationMapIndex;
typedef std::map<int,RelationKeyIDMap> RelationMapKeyIDIndex;
typedef std::pair<CKeyID, CKeyID> CRelationActive;

/** 
 * Abstract view on the relation dataset. 
 */
class CRelationView : public CDBWrapper
{
public:
    explicit CRelationView(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    ~CRelationView() = default;

    /** 
     * Show the relation to.
     * @param[in]   from  The KeyID whose target relation we want to get.
     * @return      the target KeyID, which is bound by the "from".
     */
    CKeyID To(const uint160& from, uint64_t plotid, bool poc21) const;

    /** 
     * Push the relation(bind and unbind), which is at the height, into relation tip set.
     * @param[in]    height     the block height, at which this action appears.
     * @param[out]   txid       the txid, at which tx this action appears.
     * @param[out]   action     the action, which is format of bind(from, to) or unbind(from).
     * @param[out]   relations  the actions set.
     * @return      true if action is accepted.
     */
    bool AcceptAction(const int height, const uint256& txid, const CAction& action, std::vector<std::pair<uint256, CRelationActive>> &relations, bool poc21);
    
    /** 
     * ConnectBlock is an up-layer api, which calls AcceptAction and WriteRelationsToDisk, as well as be called by ConnectTip.
     * @param[in]    height  the block height, at which the connecttip function calls.
     * @param[in]    poc21   wether poc2+ is actived.
     * @param[out]   blk     the block.
     */
    void ConnectBlock(const int height, const CBlock &blk, bool poc21);

    void DisconnectBlock(const int height, const CBlock &blk, bool poc21);
    
    /** 
     * Write the relation tip set on disk.
     */
    bool WriteRelationsToDisk(const int height, const std::vector<std::pair<uint256, CRelationActive>> &relations);
    
    /** 
     * Init the relation tip set.
     * @param[in]   height  the block height, at which loading function calls.
     * @param[in]   poc21   wether poc2+ is actived.
     * @return      true if loaded.
     */
    bool LoadRelationFromDisk(const int height, bool poc21);

    /** 
    * An api call by wallet,
    * This api will show all the relation from the cache.
    * @return  all the relation set.
    */
    CRelationVector ListRelations() const;
private:
    /** Relation tip set which is push into relationMapIndex.*/
    RelationMap relationTip;
    /** Relation KEYID tip set which is for POC21.*/
    RelationKeyIDMap relationKeyIDTip;

    /** The map records new relation tip set into, when new block is coming.*/
    RelationMapIndex relationMapIndex; 
    /** The map records new relation KEYID tip, for POC21.*/
    RelationMapKeyIDIndex relationMapKeyIDIndex; 

    bool pushBackRelation(const int height, bool poc21);
};

#endif