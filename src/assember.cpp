#include <assember.h>
#include <chainparams.h>
#include <logging.h>
#include <miner.h>
#include <poc.h>
#include <util/time.h>
#include <validation.h>
#include <key_io.h>
#include <actiondb.h>
#include <timedata.h>
#include <fspool.h>
#include <wallet/rpcwallet.h>

#include <boost/bind.hpp>

CPOCBlockAssember::CPOCBlockAssember()
{
    SetNull();
}

bool CPOCBlockAssember::UpdateDeadline(const int height, const CKeyID& keyid, const uint64_t nonce, const uint64_t deadline, const CKey& key)
{
    auto prevIndex = chainActive.Tip();
    if (prevIndex->nHeight != (height - 1)) {
        LogPrintf("chainActive has been update, the new index is %uul, but the height to be produced is %uul\n", prevIndex->nHeight, height);
        return false;
    }
    auto params = Params();
    if (deadline / prevIndex->nBaseTarget > params.TargetDeadline()) {
        LogPrintf("Invalid deadline %uul\n", deadline);
        return false;
    }

    if (this->deadline != 0 && deadline >= this->deadline) {
        LogPrintf("Invalid deadline %uul\n", deadline);
        return false;
    }

    auto plotID = keyid.GetPlotID();
    auto generationSignature = CalcGenerationSignature(prevIndex->genSign, prevIndex->nPlotID);
    if (CalcDeadline(generationSignature, height, plotID, nonce) != deadline) {
        LogPrintf("Deadline inconformity %uul\n", deadline);
        return false;
    }
    auto ts = (deadline / prevIndex->nBaseTarget);
    LogPrintf("Update new deadline: %u, now: %u, target: %u\n", ts, GetTimeMillis() / 1000, prevIndex->nTime + ts);

    {
        boost::lock_guard<boost::mutex> lock(mtx);
        this->height = height;
        this->keyid = keyid;
        this->genSig = genSig;
        this->nonce = nonce;
        this->deadline = deadline;
        this->key = key;
        auto lastBlockTime = prevIndex->GetBlockHeader().GetBlockTime();
        auto ts = (deadline / chainActive.Tip()->nBaseTarget);
        this->dl = lastBlockTime + ts;
    }
    return true;
}

void CPOCBlockAssember::CreateNewBlock()
{
    int height{ 0 };
    CKeyID from;
    uint256 genSig;
    uint64_t deadline{ 0 };
    uint64_t nonce{ 0 };
    {
        boost::lock_guard<boost::mutex> lock(mtx);
        height = this->height;
        from = this->keyid;
        genSig = this->genSig;
        nonce = this->nonce;
        deadline = this->deadline;
    }
    
    auto plotid = from.GetPlotID();
    LogPrintf("CPOCBlockAssember CreateNewBlock, plotid: %u nonce:%u newheight:%u deadline:%u utc:%u\n", plotid, nonce, height, deadline, GetTimeMillis()/1000);
    auto params = Params();
    //plotid bind
    auto to = prelationview->To(from);
    auto target = to.IsNull() ? from : to;
    auto fstx = MakeTransactionRef();

    // get a fstx from fspool
    auto usableIndex = (height / pticketview->SlotLength());
    auto FstxRefSet = pfspool->GetFstxBySlotIndex(usableIndex);
    for (auto fstxRef:FstxRefSet){
        if (!pcoinsTip->AccessCoin(fstxRef->vin[0].prevout).IsSpent()){
            fstx = fstxRef;
            LogPrint(BCLog::FIRESTONE, "%s: get fstx:%s from fspool.\n", __func__,fstx->GetHash().ToString());
            break;
        }
    }

    if (fstx->IsNull()){
        // there is no fstx in fspool.
        // so, we use the wallet to sign a fstx.
        LOCK(cs_main);
        CTicketRef fs;
        auto fskey = firestoneKey.IsValid() ? firestoneKey : key;
        if (fskey.IsValid()) {
            auto index = (height / pticketview->SlotLength()) - 1;
            for (auto ticket : pticketview->GetTicketsBySlotIndex(index)) {
                if (fskey.GetPubKey().GetID() == ticket->KeyID() && !pcoinsTip->AccessCoin(*(ticket->out)).IsSpent()) {
                    fs = ticket;
                    LogPrint(BCLog::FIRESTONE, "%s: generate new block with firestone:%s:%d\n", __func__, fs->out->hash.ToString(), fs->out->n);
                    break;
                }
            }
        }

        if (fs && fskey.IsValid()) { //find firestone
            fstx = makeSpentTicketTx(fs, height, CTxDestination(fskey.GetPubKey().GetID()), fskey);
        }
    }
    
    auto scriptPubKeyIn = GetScriptForDestination(CTxDestination(target));
    auto blk = BlockAssembler(params).CreateNewBlock(scriptPubKeyIn, nonce, plotid, deadline, fstx);
    if (blk) {
        uint32_t extraNonce = 0;
        IncrementExtraNonce(&blk->block, chainActive.Tip(), extraNonce);
        auto pblk = std::make_shared<CBlock>(blk->block);
        if (ProcessNewBlock(params, pblk, true, NULL) == false) {
            LogPrintf("ProcessNewBlock failed\n");
        }
    } else {
        LogPrintf("CreateNewBlock failed\n");
    }
}

void CPOCBlockAssember::CheckDeadline()
{
    if (dl == 0)
        return;
    if (GetAdjustedTime() >= dl) {
        CreateNewBlock();
        SetNull();
    }
}

void CPOCBlockAssember::SetNull()
{
    height = 0;
    nonce = 0;
    deadline = 0;
    genSig = uint256();
    keyid.SetNull();
    dl = 0;
    //firestoneKey = CKey();
    //key = CKey();
}

void CPOCBlockAssember::SetFirestoneAt(const CKey& key)
{
    if (key.IsValid()) {
        LogPrint(BCLog::FIRESTONE, "%s: set firestone source, keyid:%s\n", __func__, EncodeDestination(CTxDestination( key.GetPubKey().GetID())));
        firestoneKey = key;
    } 
}
